/*
 *  Copyright (C) 2000-2002 Constantin Kaplinsky.  All Rights Reserved.
 *  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * rfbproto.c - functions to deal with client side of RFB protocol.
 */

#ifdef __STRICT_ANSI__
#define _BSD_SOURCE
#define _POSIX_SOURCE
#endif
#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#define strncasecmp _strnicmp
#endif
#include <errno.h>
#ifndef WIN32
#include <pwd.h>
#endif
#include <rfb/rfbclient.h>
#ifdef LIBVNCSERVER_HAVE_LIBZ
#include <zlib.h>
#ifdef __CHECKER__
#undef Z_NULL
#define Z_NULL NULL
#endif
#endif
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
#include <jpeglib.h>
#endif
#include <stdarg.h>
#include <time.h>

#include "minilzo.h"
#include "tls.h"

/*
 * rfbClientLog prints a time-stamped message to the log file (stderr).
 */

rfbBool rfbEnableClientLogging=TRUE;

static void
rfbDefaultClientLog(const char *format, ...)
{
    va_list args;
    char buf[256];
    time_t log_clock;

    if(!rfbEnableClientLogging)
      return;

    va_start(args, format);

    time(&log_clock);
    strftime(buf, 255, "%d/%m/%Y %X ", localtime(&log_clock));
    fprintf(stderr, "%s", buf);

    vfprintf(stderr, format, args);
    fflush(stderr);

    va_end(args);
}

rfbClientLogProc rfbClientLog=rfbDefaultClientLog;
rfbClientLogProc rfbClientErr=rfbDefaultClientLog;

/* extensions */

rfbClientProtocolExtension* rfbClientExtensions = NULL;

void rfbClientRegisterExtension(rfbClientProtocolExtension* e)
{
	e->next = rfbClientExtensions;
	rfbClientExtensions = e;
}

/* client data */

void rfbClientSetClientData(rfbClient* client, void* tag, void* data)
{
	rfbClientData* clientData = client->clientData;

	while(clientData && clientData->tag != tag)
		clientData = clientData->next;
	if(clientData == NULL) {
		clientData = calloc(sizeof(rfbClientData), 1);
		clientData->next = client->clientData;
		client->clientData = clientData;
		clientData->tag = tag;
	}

	clientData->data = data;
}

void* rfbClientGetClientData(rfbClient* client, void* tag)
{
	rfbClientData* clientData = client->clientData;

	while(clientData) {
		if(clientData->tag == tag)
			return clientData->data;
		clientData = clientData->next;
	}

	return NULL;
}

/* messages */

static void FillRectangle(rfbClient* client, int x, int y, int w, int h, uint32_t colour) {
  int i,j;

#define FILL_RECT(BPP) \
    for(j=y*client->width;j<(y+h)*client->width;j+=client->width) \
      for(i=x;i<x+w;i++) \
	((uint##BPP##_t*)client->frameBuffer)[j+i]=colour;

  switch(client->format.bitsPerPixel) {
  case  8: FILL_RECT(8);  break;
  case 16: FILL_RECT(16); break;
  case 32: FILL_RECT(32); break;
  default:
    rfbClientLog("Unsupported bitsPerPixel: %d\n",client->format.bitsPerPixel);
  }
}

static void CopyRectangle(rfbClient* client, uint8_t* buffer, int x, int y, int w, int h) {
  int j;

#define COPY_RECT(BPP) \
  { \
    int rs = w * BPP / 8, rs2 = client->width * BPP / 8; \
    for (j = ((x * (BPP / 8)) + (y * rs2)); j < (y + h) * rs2; j += rs2) { \
      memcpy(client->frameBuffer + j, buffer, rs); \
      buffer += rs; \
    } \
  }

  switch(client->format.bitsPerPixel) {
  case  8: COPY_RECT(8);  break;
  case 16: COPY_RECT(16); break;
  case 32: COPY_RECT(32); break;
  default:
    rfbClientLog("Unsupported bitsPerPixel: %d\n",client->format.bitsPerPixel);
  }
}

/* TODO: test */
static void CopyRectangleFromRectangle(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y) {
  int i,j;

#define COPY_RECT_FROM_RECT(BPP) \
  { \
    uint##BPP##_t* _buffer=((uint##BPP##_t*)client->frameBuffer)+(src_y-dest_y)*client->width+src_x-dest_x; \
    if (dest_y < src_y) { \
      for(j = dest_y*client->width; j < (dest_y+h)*client->width; j += client->width) { \
        if (dest_x < src_x) { \
          for(i = dest_x; i < dest_x+w; i++) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } else { \
          for(i = dest_x+w-1; i >= dest_x; i--) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } \
      } \
    } else { \
      for(j = (dest_y+h-1)*client->width; j >= dest_y*client->width; j-=client->width) { \
        if (dest_x < src_x) { \
          for(i = dest_x; i < dest_x+w; i++) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } else { \
          for(i = dest_x+w-1; i >= dest_x; i--) { \
            ((uint##BPP##_t*)client->frameBuffer)[j+i]=_buffer[j+i]; \
          } \
        } \
      } \
    } \
  }

  switch(client->format.bitsPerPixel) {
  case  8: COPY_RECT_FROM_RECT(8);  break;
  case 16: COPY_RECT_FROM_RECT(16); break;
  case 32: COPY_RECT_FROM_RECT(32); break;
  default:
    rfbClientLog("Unsupported bitsPerPixel: %d\n",client->format.bitsPerPixel);
  }
}

static rfbBool HandleRRE8(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleRRE16(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleRRE32(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleCoRRE8(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleCoRRE16(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleCoRRE32(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleHextile8(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleHextile16(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleHextile32(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleUltra8(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleUltra16(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleUltra32(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleUltraZip8(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleUltraZip16(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleUltraZip32(rfbClient* client, int rx, int ry, int rw, int rh);
#ifdef LIBVNCSERVER_HAVE_LIBZ
static rfbBool HandleZlib8(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleZlib16(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleZlib32(rfbClient* client, int rx, int ry, int rw, int rh);
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
static rfbBool HandleTight8(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleTight16(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleTight32(rfbClient* client, int rx, int ry, int rw, int rh);

static long ReadCompactLen (rfbClient* client);

static void JpegInitSource(j_decompress_ptr cinfo);
static boolean JpegFillInputBuffer(j_decompress_ptr cinfo);
static void JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes);
static void JpegTermSource(j_decompress_ptr cinfo);
static void JpegSetSrcManager(j_decompress_ptr cinfo, uint8_t *compressedData,
                              int compressedLen);
#endif
static rfbBool HandleZRLE8(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleZRLE15(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleZRLE16(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleZRLE24(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleZRLE24Up(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleZRLE24Down(rfbClient* client, int rx, int ry, int rw, int rh);
static rfbBool HandleZRLE32(rfbClient* client, int rx, int ry, int rw, int rh);
#endif

/*
 * Server Capability Functions
 */
rfbBool
SupportsClient2Server(rfbClient* client, int messageType)
{
    return (client->supportedMessages.client2server[((messageType & 0xFF)/8)] & (1<<(messageType % 8)) ? TRUE : FALSE);
}

rfbBool
SupportsServer2Client(rfbClient* client, int messageType)
{
    return (client->supportedMessages.server2client[((messageType & 0xFF)/8)] & (1<<(messageType % 8)) ? TRUE : FALSE);
}

void
SetClient2Server(rfbClient* client, int messageType)
{
  client->supportedMessages.client2server[((messageType & 0xFF)/8)] |= (1<<(messageType % 8));
}

void
SetServer2Client(rfbClient* client, int messageType)
{
  client->supportedMessages.server2client[((messageType & 0xFF)/8)] |= (1<<(messageType % 8));
}

void
ClearClient2Server(rfbClient* client, int messageType)
{
  client->supportedMessages.client2server[((messageType & 0xFF)/8)] &= (!(1<<(messageType % 8)));
}

void
ClearServer2Client(rfbClient* client, int messageType)
{
  client->supportedMessages.server2client[((messageType & 0xFF)/8)] &= (!(1<<(messageType % 8)));
}


void
DefaultSupportedMessages(rfbClient* client)
{
    memset((char *)&client->supportedMessages,0,sizeof(client->supportedMessages));

    /* Default client supported messages (universal RFB 3.3 protocol) */
    SetClient2Server(client, rfbSetPixelFormat);
    /* SetClient2Server(client, rfbFixColourMapEntries); Not currently supported */
    SetClient2Server(client, rfbSetEncodings);
    SetClient2Server(client, rfbFramebufferUpdateRequest);
    SetClient2Server(client, rfbKeyEvent);
    SetClient2Server(client, rfbPointerEvent);
    SetClient2Server(client, rfbClientCutText);
    /* technically, we only care what we can *send* to the server
     * but, we set Server2Client Just in case it ever becomes useful
     */
    SetServer2Client(client, rfbFramebufferUpdate);
    SetServer2Client(client, rfbSetColourMapEntries);
    SetServer2Client(client, rfbBell);
    SetServer2Client(client, rfbServerCutText);
}

void
DefaultSupportedMessagesUltraVNC(rfbClient* client)
{
    DefaultSupportedMessages(client);
    SetClient2Server(client, rfbFileTransfer);
    SetClient2Server(client, rfbSetScale);
    SetClient2Server(client, rfbSetServerInput);
    SetClient2Server(client, rfbSetSW);
    SetClient2Server(client, rfbTextChat);
    SetClient2Server(client, rfbPalmVNCSetScaleFactor);
    /* technically, we only care what we can *send* to the server */
    SetServer2Client(client, rfbResizeFrameBuffer);
    SetServer2Client(client, rfbPalmVNCReSizeFrameBuffer);
    SetServer2Client(client, rfbFileTransfer);
    SetServer2Client(client, rfbTextChat);
}


void
DefaultSupportedMessagesTightVNC(rfbClient* client)
{
    DefaultSupportedMessages(client);
    SetClient2Server(client, rfbFileTransfer);
    SetClient2Server(client, rfbSetServerInput);
    SetClient2Server(client, rfbSetSW);
    /* SetClient2Server(client, rfbTextChat); */
    /* technically, we only care what we can *send* to the server */
    SetServer2Client(client, rfbFileTransfer);
    SetServer2Client(client, rfbTextChat);
}

#ifndef WIN32
static rfbBool
IsUnixSocket(const char *name)
{
  struct stat sb;
  if(stat(name, &sb) == 0 && (sb.st_mode & S_IFMT) == S_IFSOCK)
    return TRUE;
  return FALSE;
}
#endif

/*
 * ConnectToRFBServer.
 */

rfbBool
ConnectToRFBServer(rfbClient* client,const char *hostname, int port)
{
  unsigned int host;

  if (client->serverPort==-1) {
    /* serverHost is a file recorded by vncrec. */
    const char* magic="vncLog0.0";
    char buffer[10];
    rfbVNCRec* rec = (rfbVNCRec*)malloc(sizeof(rfbVNCRec));
    client->vncRec = rec;

    rec->file = fopen(client->serverHost,"rb");
    rec->tv.tv_sec = 0;
    rec->readTimestamp = FALSE;
    rec->doNotSleep = FALSE;
    
    if (!rec->file) {
      rfbClientLog("Could not open %s.\n",client->serverHost);
      return FALSE;
    }
    setbuf(rec->file,NULL);
    fread(buffer,1,strlen(magic),rec->file);
    if (strncmp(buffer,magic,strlen(magic))) {
      rfbClientLog("File %s was not recorded by vncrec.\n",client->serverHost);
      fclose(rec->file);
      return FALSE;
    }
    client->sock = -1;
    return TRUE;
  }

#ifndef WIN32
  if(IsUnixSocket(hostname))
    /* serverHost is a UNIX socket. */
    client->sock = ConnectClientToUnixSock(hostname);
  else
#endif
  {
    /* serverHost is a hostname */
    if (!StringToIPAddr(hostname, &host)) {
      rfbClientLog("Couldn't convert '%s' to host address\n", hostname);
      return FALSE;
    }
    client->sock = ConnectClientToTcpAddr(host, port);
  }

  if (client->sock < 0) {
    rfbClientLog("Unable to connect to VNC server\n");
    return FALSE;
  }

  return SetNonBlocking(client->sock);
}

extern void rfbClientEncryptBytes(unsigned char* bytes, char* passwd);
extern void rfbClientEncryptBytes2(unsigned char *where, const int length, unsigned char *key);

rfbBool
rfbHandleAuthResult(rfbClient* client)
{
    uint32_t authResult=0, reasonLen=0;
    char *reason=NULL;

    if (!ReadFromRFBServer(client, (char *)&authResult, 4)) return FALSE;

    authResult = rfbClientSwap32IfLE(authResult);

    switch (authResult) {
    case rfbVncAuthOK:
      rfbClientLog("VNC authentication succeeded\n");
      return TRUE;
      break;
    case rfbVncAuthFailed:
      if (client->major==3 && client->minor>7)
      {
        /* we have an error following */
        if (!ReadFromRFBServer(client, (char *)&reasonLen, 4)) return FALSE;
        reasonLen = rfbClientSwap32IfLE(reasonLen);
        reason = malloc(reasonLen+1);
        if (!ReadFromRFBServer(client, reason, reasonLen)) { free(reason); return FALSE; }
        reason[reasonLen]=0;
        rfbClientLog("VNC connection failed: %s\n",reason);
        free(reason);
        return FALSE;
      }
      rfbClientLog("VNC authentication failed\n");
      return FALSE;
    case rfbVncAuthTooMany:
      rfbClientLog("VNC authentication failed - too many tries\n");
      return FALSE;
    }

    rfbClientLog("Unknown VNC authentication result: %d\n",
                 (int)authResult);
    return FALSE;
}

static void
ReadReason(rfbClient* client)
{
    uint32_t reasonLen;
    char *reason;

    /* we have an error following */
    if (!ReadFromRFBServer(client, (char *)&reasonLen, 4)) return;
    reasonLen = rfbClientSwap32IfLE(reasonLen);
    reason = malloc(reasonLen+1);
    if (!ReadFromRFBServer(client, reason, reasonLen)) { free(reason); return; }
    reason[reasonLen]=0;
    rfbClientLog("VNC connection failed: %s\n",reason);
    free(reason);
}

static rfbBool
ReadSupportedSecurityType(rfbClient* client, uint32_t *result, rfbBool subAuth)
{
    uint8_t count=0;
    uint8_t loop=0;
    uint8_t flag=0;
    uint8_t tAuth[256];
    char buf1[500],buf2[10];
    uint32_t authScheme;

    if (!ReadFromRFBServer(client, (char *)&count, 1)) return FALSE;

    if (count==0)
    {
        rfbClientLog("List of security types is ZERO, expecting an error to follow\n");
        ReadReason(client);
        return FALSE;
    }
    if (count>sizeof(tAuth))
    {
        rfbClientLog("%d security types are too many; maximum is %d\n", count, sizeof(tAuth));
        return FALSE;
    }

    rfbClientLog("We have %d security types to read\n", count);
    authScheme=0;
    /* now, we have a list of available security types to read ( uint8_t[] ) */
    for (loop=0;loop<count;loop++)
    {
        if (!ReadFromRFBServer(client, (char *)&tAuth[loop], 1)) return FALSE;
        rfbClientLog("%d) Received security type %d\n", loop, tAuth[loop]);
        if (flag) continue;
        if (tAuth[loop]==rfbVncAuth || tAuth[loop]==rfbNoAuth || tAuth[loop]==rfbMSLogon ||
            (!subAuth && (tAuth[loop]==rfbTLS || tAuth[loop]==rfbVeNCrypt)))
        {
            flag++;
            authScheme=tAuth[loop];
            rfbClientLog("Selecting security type %d (%d/%d in the list)\n", authScheme, loop, count);
            /* send back a single byte indicating which security type to use */
            if (!WriteToRFBServer(client, (char *)&tAuth[loop], 1)) return FALSE;

        }
    }
    if (authScheme==0)
    {
        memset(buf1, 0, sizeof(buf1));
        for (loop=0;loop<count;loop++)
        {
            if (strlen(buf1)>=sizeof(buf1)-1) break;
            snprintf(buf2, sizeof(buf2), (loop>0 ? ", %d" : "%d"), (int)tAuth[loop]);
            strncat(buf1, buf2, sizeof(buf1)-strlen(buf1)-1);
        }
        rfbClientLog("Unknown authentication scheme from VNC server: %s\n",
               buf1);
        return FALSE;
    }
    *result = authScheme;
    return TRUE;
}

static rfbBool
HandleVncAuth(rfbClient *client)
{
    uint8_t challenge[CHALLENGESIZE];
    char *passwd=NULL;
    int i;

    if (!ReadFromRFBServer(client, (char *)challenge, CHALLENGESIZE)) return FALSE;

    if (client->serverPort!=-1) { /* if not playing a vncrec file */
      if (client->GetPassword)
        passwd = client->GetPassword(client);

      if ((!passwd) || (strlen(passwd) == 0)) {
        rfbClientLog("Reading password failed\n");
        return FALSE;
      }
      if (strlen(passwd) > 8) {
        passwd[8] = '\0';
      }

      rfbClientEncryptBytes(challenge, passwd);

      /* Lose the password from memory */
      for (i = strlen(passwd); i >= 0; i--) {
        passwd[i] = '\0';
      }
      free(passwd);

      if (!WriteToRFBServer(client, (char *)challenge, CHALLENGESIZE)) return FALSE;
    }

    /* Handle the SecurityResult message */
    if (!rfbHandleAuthResult(client)) return FALSE;

    return TRUE;
}

static void
FreeUserCredential(rfbCredential *cred)
{
  if (cred->userCredential.username) free(cred->userCredential.username);
  if (cred->userCredential.password) free(cred->userCredential.password);
  free(cred);
}

static rfbBool
HandlePlainAuth(rfbClient *client)
{
  uint32_t ulen, ulensw;
  uint32_t plen, plensw;
  rfbCredential *cred;

  if (!client->GetCredential)
  {
    rfbClientLog("GetCredential callback is not set.\n");
    return FALSE;
  }
  cred = client->GetCredential(client, rfbCredentialTypeUser);
  if (!cred)
  {
    rfbClientLog("Reading credential failed\n");
    return FALSE;
  }

  ulen = (cred->userCredential.username ? strlen(cred->userCredential.username) : 0);
  ulensw = rfbClientSwap32IfLE(ulen);
  plen = (cred->userCredential.password ? strlen(cred->userCredential.password) : 0);
  plensw = rfbClientSwap32IfLE(plen);
  if (!WriteToRFBServer(client, (char *)&ulensw, 4) ||
      !WriteToRFBServer(client, (char *)&plensw, 4))
  {
    FreeUserCredential(cred);
    return FALSE;
  }
  if (ulen > 0)
  {
    if (!WriteToRFBServer(client, cred->userCredential.username, ulen))
    {
      FreeUserCredential(cred);
      return FALSE;
    }
  }
  if (plen > 0)
  {
    if (!WriteToRFBServer(client, cred->userCredential.password, plen))
    {
      FreeUserCredential(cred);
      return FALSE;
    }
  }

  FreeUserCredential(cred);

  /* Handle the SecurityResult message */
  if (!rfbHandleAuthResult(client)) return FALSE;

  return TRUE;
}

/* Simple 64bit big integer arithmetic implementation */
/* (x + y) % m, works even if (x + y) > 64bit */
#define rfbAddM64(x,y,m) ((x+y)%m+(x+y<x?(((uint64_t)-1)%m+1)%m:0))
/* (x * y) % m */
static uint64_t
rfbMulM64(uint64_t x, uint64_t y, uint64_t m)
{
  uint64_t r;
  for(r=0;x>0;x>>=1)
  {
    if (x&1) r=rfbAddM64(r,y,m);
    y=rfbAddM64(y,y,m);
  }
  return r;
}
/* (x ^ y) % m */
static uint64_t
rfbPowM64(uint64_t b, uint64_t e, uint64_t m)
{
  uint64_t r;
  for(r=1;e>0;e>>=1)
  {
    if(e&1) r=rfbMulM64(r,b,m);
    b=rfbMulM64(b,b,m);
  }
  return r;
}

static rfbBool
HandleMSLogonAuth(rfbClient *client)
{
  uint64_t gen, mod, resp, priv, pub, key;
  uint8_t username[256], password[64];
  rfbCredential *cred;

  if (!ReadFromRFBServer(client, (char *)&gen, 8)) return FALSE;
  if (!ReadFromRFBServer(client, (char *)&mod, 8)) return FALSE;
  if (!ReadFromRFBServer(client, (char *)&resp, 8)) return FALSE;
  gen = rfbClientSwap64IfLE(gen);
  mod = rfbClientSwap64IfLE(mod);
  resp = rfbClientSwap64IfLE(resp);

  if (!client->GetCredential)
  {
    rfbClientLog("GetCredential callback is not set.\n");
    return FALSE;
  }
  rfbClientLog("WARNING! MSLogon security type has very low password encryption! "\
    "Use it only with SSH tunnel or trusted network.\n");
  cred = client->GetCredential(client, rfbCredentialTypeUser);
  if (!cred)
  {
    rfbClientLog("Reading credential failed\n");
    return FALSE;
  }

  memset(username, 0, sizeof(username));
  strncpy((char *)username, cred->userCredential.username, sizeof(username));
  memset(password, 0, sizeof(password));
  strncpy((char *)password, cred->userCredential.password, sizeof(password));
  FreeUserCredential(cred);

  srand(time(NULL));
  priv = ((uint64_t)rand())<<32;
  priv |= (uint64_t)rand();

  pub = rfbPowM64(gen, priv, mod);
  key = rfbPowM64(resp, priv, mod);
  pub = rfbClientSwap64IfLE(pub);
  key = rfbClientSwap64IfLE(key);

  rfbClientEncryptBytes2(username, sizeof(username), (unsigned char *)&key);
  rfbClientEncryptBytes2(password, sizeof(password), (unsigned char *)&key);

  if (!WriteToRFBServer(client, (char *)&pub, 8)) return FALSE;
  if (!WriteToRFBServer(client, (char *)username, sizeof(username))) return FALSE;
  if (!WriteToRFBServer(client, (char *)password, sizeof(password))) return FALSE;

  /* Handle the SecurityResult message */
  if (!rfbHandleAuthResult(client)) return FALSE;

  return TRUE;
}

/*
 * InitialiseRFBConnection.
 */

rfbBool
InitialiseRFBConnection(rfbClient* client)
{
  rfbProtocolVersionMsg pv;
  int major,minor;
  uint32_t authScheme;
  uint32_t subAuthScheme;
  rfbClientInitMsg ci;

  /* if the connection is immediately closed, don't report anything, so
       that pmw's monitor can make test connections */

  if (client->listenSpecified)
    errorMessageOnReadFailure = FALSE;

  if (!ReadFromRFBServer(client, pv, sz_rfbProtocolVersionMsg)) return FALSE;
  pv[sz_rfbProtocolVersionMsg]=0;

  errorMessageOnReadFailure = TRUE;

  pv[sz_rfbProtocolVersionMsg] = 0;

  if (sscanf(pv,rfbProtocolVersionFormat,&major,&minor) != 2) {
    rfbClientLog("Not a valid VNC server (%s)\n",pv);
    return FALSE;
  }


  DefaultSupportedMessages(client);
  client->major = major;
  client->minor = minor;

  /* fall back to viewer supported version */
  if ((major==rfbProtocolMajorVersion) && (minor>rfbProtocolMinorVersion))
    client->minor = rfbProtocolMinorVersion;

  /* UltraVNC uses minor codes 4 and 6 for the server */
  if (major==3 && (minor==4 || minor==6)) {
      rfbClientLog("UltraVNC server detected, enabling UltraVNC specific messages\n",pv);
      DefaultSupportedMessagesUltraVNC(client);
  }

  /* TightVNC uses minor codes 5 for the server */
  if (major==3 && minor==5) {
      rfbClientLog("TightVNC server detected, enabling TightVNC specific messages\n",pv);
      DefaultSupportedMessagesTightVNC(client);
  }

  /* we do not support > RFB3.8 */
  if ((major==3 && minor>8) || major>3)
  {
    client->major=3;
    client->minor=8;
  }

  rfbClientLog("VNC server supports protocol version %d.%d (viewer %d.%d)\n",
	  major, minor, rfbProtocolMajorVersion, rfbProtocolMinorVersion);

  sprintf(pv,rfbProtocolVersionFormat,client->major,client->minor);

  if (!WriteToRFBServer(client, pv, sz_rfbProtocolVersionMsg)) return FALSE;


  /* 3.7 and onwards sends a # of security types first */
  if (client->major==3 && client->minor > 6)
  {
    if (!ReadSupportedSecurityType(client, &authScheme, FALSE)) return FALSE;
  }
  else
  {
    if (!ReadFromRFBServer(client, (char *)&authScheme, 4)) return FALSE;
    authScheme = rfbClientSwap32IfLE(authScheme);
  }
  
  rfbClientLog("Selected Security Scheme %d\n", authScheme);
  client->authScheme = authScheme;
  
  switch (authScheme) {

  case rfbConnFailed:
    ReadReason(client);
    return FALSE;

  case rfbNoAuth:
    rfbClientLog("No authentication needed\n");

    /* 3.8 and upwards sends a Security Result for rfbNoAuth */
    if ((client->major==3 && client->minor > 7) || client->major>3)
        if (!rfbHandleAuthResult(client)) return FALSE;        

    break;

  case rfbVncAuth:
    if (!HandleVncAuth(client)) return FALSE;
    break;

  case rfbMSLogon:
    if (!HandleMSLogonAuth(client)) return FALSE;
    break;

  case rfbTLS:
    if (!HandleAnonTLSAuth(client)) return FALSE;
    /* After the TLS session is established, sub auth types are expected.
     * Note that all following reading/writing are through the TLS session from here.
     */
    if (!ReadSupportedSecurityType(client, &subAuthScheme, TRUE)) return FALSE;
    client->subAuthScheme = subAuthScheme;

    switch (subAuthScheme) {

      case rfbConnFailed:
        ReadReason(client);
        return FALSE;

      case rfbNoAuth:
        rfbClientLog("No sub authentication needed\n");
        /* 3.8 and upwards sends a Security Result for rfbNoAuth */
        if ((client->major==3 && client->minor > 7) || client->major>3)
            if (!rfbHandleAuthResult(client)) return FALSE;
        break;

      case rfbVncAuth:
        if (!HandleVncAuth(client)) return FALSE;
        break;

      default:
        rfbClientLog("Unknown sub authentication scheme from VNC server: %d\n",
            (int)subAuthScheme);
        return FALSE;
    }

    break;

  case rfbVeNCrypt:
    if (!HandleVeNCryptAuth(client)) return FALSE;

    switch (client->subAuthScheme) {

      case rfbVeNCryptTLSNone:
      case rfbVeNCryptX509None:
        rfbClientLog("No sub authentication needed\n");
        if (!rfbHandleAuthResult(client)) return FALSE;
        break;

      case rfbVeNCryptTLSVNC:
      case rfbVeNCryptX509VNC:
        if (!HandleVncAuth(client)) return FALSE;
        break;

      case rfbVeNCryptTLSPlain:
      case rfbVeNCryptX509Plain:
        if (!HandlePlainAuth(client)) return FALSE;
        break;

      default:
        rfbClientLog("Unknown sub authentication scheme from VNC server: %d\n",
            client->subAuthScheme);
        return FALSE;
    }

    break;

  default:
    rfbClientLog("Unknown authentication scheme from VNC server: %d\n",
	    (int)authScheme);
    return FALSE;
  }

  ci.shared = (client->appData.shareDesktop ? 1 : 0);

  if (!WriteToRFBServer(client,  (char *)&ci, sz_rfbClientInitMsg)) return FALSE;

  if (!ReadFromRFBServer(client, (char *)&client->si, sz_rfbServerInitMsg)) return FALSE;

  client->si.framebufferWidth = rfbClientSwap16IfLE(client->si.framebufferWidth);
  client->si.framebufferHeight = rfbClientSwap16IfLE(client->si.framebufferHeight);
  client->si.format.redMax = rfbClientSwap16IfLE(client->si.format.redMax);
  client->si.format.greenMax = rfbClientSwap16IfLE(client->si.format.greenMax);
  client->si.format.blueMax = rfbClientSwap16IfLE(client->si.format.blueMax);
  client->si.nameLength = rfbClientSwap32IfLE(client->si.nameLength);

  client->desktopName = malloc(client->si.nameLength + 1);
  if (!client->desktopName) {
    rfbClientLog("Error allocating memory for desktop name, %lu bytes\n",
            (unsigned long)client->si.nameLength);
    return FALSE;
  }

  if (!ReadFromRFBServer(client, client->desktopName, client->si.nameLength)) return FALSE;

  client->desktopName[client->si.nameLength] = 0;

  rfbClientLog("Desktop name \"%s\"\n",client->desktopName);

  rfbClientLog("Connected to VNC server, using protocol version %d.%d\n",
	  client->major, client->minor);

  rfbClientLog("VNC server default format:\n");
  PrintPixelFormat(&client->si.format);

  return TRUE;
}


/*
 * SetFormatAndEncodings.
 */

rfbBool
SetFormatAndEncodings(rfbClient* client)
{
  rfbSetPixelFormatMsg spf;
  char buf[sz_rfbSetEncodingsMsg + MAX_ENCODINGS * 4];

  rfbSetEncodingsMsg *se = (rfbSetEncodingsMsg *)buf;
  uint32_t *encs = (uint32_t *)(&buf[sz_rfbSetEncodingsMsg]);
  int len = 0;
  rfbBool requestCompressLevel = FALSE;
  rfbBool requestQualityLevel = FALSE;
  rfbBool requestLastRectEncoding = FALSE;
  rfbClientProtocolExtension* e;

  if (!SupportsClient2Server(client, rfbSetPixelFormat)) return TRUE;

  spf.type = rfbSetPixelFormat;
  spf.format = client->format;
  spf.format.redMax = rfbClientSwap16IfLE(spf.format.redMax);
  spf.format.greenMax = rfbClientSwap16IfLE(spf.format.greenMax);
  spf.format.blueMax = rfbClientSwap16IfLE(spf.format.blueMax);

  if (!WriteToRFBServer(client, (char *)&spf, sz_rfbSetPixelFormatMsg))
    return FALSE;


  if (!SupportsClient2Server(client, rfbSetEncodings)) return TRUE;

  se->type = rfbSetEncodings;
  se->nEncodings = 0;

  if (client->appData.encodingsString) {
    const char *encStr = client->appData.encodingsString;
    int encStrLen;
    do {
      const char *nextEncStr = strchr(encStr, ' ');
      if (nextEncStr) {
	encStrLen = nextEncStr - encStr;
	nextEncStr++;
      } else {
	encStrLen = strlen(encStr);
      }

      if (strncasecmp(encStr,"raw",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingRaw);
      } else if (strncasecmp(encStr,"copyrect",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingCopyRect);
#ifdef LIBVNCSERVER_HAVE_LIBZ
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
      } else if (strncasecmp(encStr,"tight",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingTight);
	requestLastRectEncoding = TRUE;
	if (client->appData.compressLevel >= 0 && client->appData.compressLevel <= 9)
	  requestCompressLevel = TRUE;
	if (client->appData.enableJPEG)
	  requestQualityLevel = TRUE;
#endif
#endif
      } else if (strncasecmp(encStr,"hextile",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingHextile);
#ifdef LIBVNCSERVER_HAVE_LIBZ
      } else if (strncasecmp(encStr,"zlib",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZlib);
	if (client->appData.compressLevel >= 0 && client->appData.compressLevel <= 9)
	  requestCompressLevel = TRUE;
      } else if (strncasecmp(encStr,"zlibhex",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZlibHex);
	if (client->appData.compressLevel >= 0 && client->appData.compressLevel <= 9)
	  requestCompressLevel = TRUE;
      } else if (strncasecmp(encStr,"zrle",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZRLE);
      } else if (strncasecmp(encStr,"zywrle",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZYWRLE);
	requestQualityLevel = TRUE;
#endif
      } else if ((strncasecmp(encStr,"ultra",encStrLen) == 0) || (strncasecmp(encStr,"ultrazip",encStrLen) == 0)) {
        /* There are 2 encodings used in 'ultra' */
        encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingUltra);
        encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingUltraZip);
      } else if (strncasecmp(encStr,"corre",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingCoRRE);
      } else if (strncasecmp(encStr,"rre",encStrLen) == 0) {
	encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingRRE);
      } else {
	rfbClientLog("Unknown encoding '%.*s'\n",encStrLen,encStr);
      }

      encStr = nextEncStr;
    } while (encStr && se->nEncodings < MAX_ENCODINGS);

    if (se->nEncodings < MAX_ENCODINGS && requestCompressLevel) {
      encs[se->nEncodings++] = rfbClientSwap32IfLE(client->appData.compressLevel +
					  rfbEncodingCompressLevel0);
    }

    if (se->nEncodings < MAX_ENCODINGS && requestQualityLevel) {
      if (client->appData.qualityLevel < 0 || client->appData.qualityLevel > 9)
        client->appData.qualityLevel = 5;
      encs[se->nEncodings++] = rfbClientSwap32IfLE(client->appData.qualityLevel +
					  rfbEncodingQualityLevel0);
    }
  }
  else {
    if (SameMachine(client->sock)) {
      /* TODO:
      if (!tunnelSpecified) {
      */
      rfbClientLog("Same machine: preferring raw encoding\n");
      encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingRaw);
      /*
      } else {
	rfbClientLog("Tunneling active: preferring tight encoding\n");
      }
      */
    }

    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingCopyRect);
#ifdef LIBVNCSERVER_HAVE_LIBZ
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingTight);
    requestLastRectEncoding = TRUE;
#endif
#endif
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingHextile);
#ifdef LIBVNCSERVER_HAVE_LIBZ
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZlib);
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZRLE);
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingZYWRLE);
#endif
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingUltra);
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingUltraZip);
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingCoRRE);
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingRRE);

    if (client->appData.compressLevel >= 0 && client->appData.compressLevel <= 9) {
      encs[se->nEncodings++] = rfbClientSwap32IfLE(client->appData.compressLevel +
					  rfbEncodingCompressLevel0);
    } else /* if (!tunnelSpecified) */ {
      /* If -tunnel option was provided, we assume that server machine is
	 not in the local network so we use default compression level for
	 tight encoding instead of fast compression. Thus we are
	 requesting level 1 compression only if tunneling is not used. */
      encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingCompressLevel1);
    }

    if (client->appData.enableJPEG) {
      if (client->appData.qualityLevel < 0 || client->appData.qualityLevel > 9)
	client->appData.qualityLevel = 5;
      encs[se->nEncodings++] = rfbClientSwap32IfLE(client->appData.qualityLevel +
					  rfbEncodingQualityLevel0);
    }
  }



  /* Remote Cursor Support (local to viewer) */
  if (client->appData.useRemoteCursor) {
    if (se->nEncodings < MAX_ENCODINGS)
      encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingXCursor);
    if (se->nEncodings < MAX_ENCODINGS)
      encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingRichCursor);
    if (se->nEncodings < MAX_ENCODINGS)
      encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingPointerPos);
  }

  /* Keyboard State Encodings */
  if (se->nEncodings < MAX_ENCODINGS)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingKeyboardLedState);

  /* New Frame Buffer Size */
  if (se->nEncodings < MAX_ENCODINGS && client->canHandleNewFBSize)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingNewFBSize);

  /* Last Rect */
  if (se->nEncodings < MAX_ENCODINGS && requestLastRectEncoding)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingLastRect);

  /* Server Capabilities */
  if (se->nEncodings < MAX_ENCODINGS)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingSupportedMessages);
  if (se->nEncodings < MAX_ENCODINGS)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingSupportedEncodings);
  if (se->nEncodings < MAX_ENCODINGS)
    encs[se->nEncodings++] = rfbClientSwap32IfLE(rfbEncodingServerIdentity);


  /* client extensions */
  for(e = rfbClientExtensions; e; e = e->next)
    if(e->encodings) {
      int* enc;
      for(enc = e->encodings; *enc; enc++)
	encs[se->nEncodings++] = rfbClientSwap32IfLE(*enc);
    }

  len = sz_rfbSetEncodingsMsg + se->nEncodings * 4;

  se->nEncodings = rfbClientSwap16IfLE(se->nEncodings);

  if (!WriteToRFBServer(client, buf, len)) return FALSE;

  return TRUE;
}


/*
 * SendIncrementalFramebufferUpdateRequest.
 */

rfbBool
SendIncrementalFramebufferUpdateRequest(rfbClient* client)
{
	return SendFramebufferUpdateRequest(client,
			client->updateRect.x, client->updateRect.y,
			client->updateRect.w, client->updateRect.h, TRUE);
}


/*
 * SendFramebufferUpdateRequest.
 */

rfbBool
SendFramebufferUpdateRequest(rfbClient* client, int x, int y, int w, int h, rfbBool incremental)
{
  rfbFramebufferUpdateRequestMsg fur;

  if (!SupportsClient2Server(client, rfbFramebufferUpdateRequest)) return TRUE;
  
  fur.type = rfbFramebufferUpdateRequest;
  fur.incremental = incremental ? 1 : 0;
  fur.x = rfbClientSwap16IfLE(x);
  fur.y = rfbClientSwap16IfLE(y);
  fur.w = rfbClientSwap16IfLE(w);
  fur.h = rfbClientSwap16IfLE(h);

  if (!WriteToRFBServer(client, (char *)&fur, sz_rfbFramebufferUpdateRequestMsg))
    return FALSE;

  return TRUE;
}


/*
 * SendScaleSetting.
 */
rfbBool
SendScaleSetting(rfbClient* client,int scaleSetting)
{
  rfbSetScaleMsg ssm;

  ssm.scale = scaleSetting;
  ssm.pad = 0;
  
  /* favor UltraVNC SetScale if both are supported */
  if (SupportsClient2Server(client, rfbSetScale)) {
      ssm.type = rfbSetScale;
      if (!WriteToRFBServer(client, (char *)&ssm, sz_rfbSetScaleMsg))
          return FALSE;
  }
  
  if (SupportsClient2Server(client, rfbPalmVNCSetScaleFactor)) {
      ssm.type = rfbPalmVNCSetScaleFactor;
      if (!WriteToRFBServer(client, (char *)&ssm, sz_rfbSetScaleMsg))
          return FALSE;
  }

  return TRUE;
}

/*
 * TextChatFunctions (UltraVNC)
 * Extremely bandwidth friendly method of communicating with a user
 * (Think HelpDesk type applications)
 */

rfbBool TextChatSend(rfbClient* client, char *text)
{
    rfbTextChatMsg chat;
    int count = strlen(text);

    if (!SupportsClient2Server(client, rfbTextChat)) return TRUE;
    chat.type = rfbTextChat;
    chat.pad1 = 0;
    chat.pad2 = 0;
    chat.length = (uint32_t)count;
    chat.length = rfbClientSwap32IfLE(chat.length);

    if (!WriteToRFBServer(client, (char *)&chat, sz_rfbTextChatMsg))
        return FALSE;

    if (count>0) {
        if (!WriteToRFBServer(client, text, count))
            return FALSE;
    }
    return TRUE;
}

rfbBool TextChatOpen(rfbClient* client)
{
    rfbTextChatMsg chat;

    if (!SupportsClient2Server(client, rfbTextChat)) return TRUE;
    chat.type = rfbTextChat;
    chat.pad1 = 0;
    chat.pad2 = 0;
    chat.length = rfbClientSwap32IfLE(rfbTextChatOpen);
    return  (WriteToRFBServer(client, (char *)&chat, sz_rfbTextChatMsg) ? TRUE : FALSE);
}

rfbBool TextChatClose(rfbClient* client)
{
    rfbTextChatMsg chat;
    if (!SupportsClient2Server(client, rfbTextChat)) return TRUE;
    chat.type = rfbTextChat;
    chat.pad1 = 0;
    chat.pad2 = 0;
    chat.length = rfbClientSwap32IfLE(rfbTextChatClose);
    return  (WriteToRFBServer(client, (char *)&chat, sz_rfbTextChatMsg) ? TRUE : FALSE);
}

rfbBool TextChatFinish(rfbClient* client)
{
    rfbTextChatMsg chat;
    if (!SupportsClient2Server(client, rfbTextChat)) return TRUE;
    chat.type = rfbTextChat;
    chat.pad1 = 0;
    chat.pad2 = 0;
    chat.length = rfbClientSwap32IfLE(rfbTextChatFinished);
    return  (WriteToRFBServer(client, (char *)&chat, sz_rfbTextChatMsg) ? TRUE : FALSE);
}

/*
 * UltraVNC Server Input Disable
 * Apparently, the remote client can *prevent* the local user from interacting with the display
 * I would think this is extremely helpful when used in a HelpDesk situation
 */
rfbBool PermitServerInput(rfbClient* client, int enabled)
{
    rfbSetServerInputMsg msg;

    if (!SupportsClient2Server(client, rfbSetServerInput)) return TRUE;
    /* enabled==1, then server input from local keyboard is disabled */
    msg.type = rfbSetServerInput;
    msg.status = (enabled ? 1 : 0);
    msg.pad = 0;
    return  (WriteToRFBServer(client, (char *)&msg, sz_rfbSetServerInputMsg) ? TRUE : FALSE);
}


/*
 * SendPointerEvent.
 */

rfbBool
SendPointerEvent(rfbClient* client,int x, int y, int buttonMask)
{
  rfbPointerEventMsg pe;

  if (!SupportsClient2Server(client, rfbPointerEvent)) return TRUE;

  pe.type = rfbPointerEvent;
  pe.buttonMask = buttonMask;
  if (x < 0) x = 0;
  if (y < 0) y = 0;

  pe.x = rfbClientSwap16IfLE(x);
  pe.y = rfbClientSwap16IfLE(y);
  return WriteToRFBServer(client, (char *)&pe, sz_rfbPointerEventMsg);
}


/*
 * SendKeyEvent.
 */

rfbBool
SendKeyEvent(rfbClient* client, uint32_t key, rfbBool down)
{
  rfbKeyEventMsg ke;

  if (!SupportsClient2Server(client, rfbKeyEvent)) return TRUE;

  ke.type = rfbKeyEvent;
  ke.down = down ? 1 : 0;
  ke.key = rfbClientSwap32IfLE(key);
  return WriteToRFBServer(client, (char *)&ke, sz_rfbKeyEventMsg);
}


/*
 * SendClientCutText.
 */

rfbBool
SendClientCutText(rfbClient* client, char *str, int len)
{
  rfbClientCutTextMsg cct;

  if (!SupportsClient2Server(client, rfbClientCutText)) return TRUE;

  cct.type = rfbClientCutText;
  cct.length = rfbClientSwap32IfLE(len);
  return  (WriteToRFBServer(client, (char *)&cct, sz_rfbClientCutTextMsg) &&
	   WriteToRFBServer(client, str, len));
}



/*
 * HandleRFBServerMessage.
 */

rfbBool
HandleRFBServerMessage(rfbClient* client)
{
  rfbServerToClientMsg msg;

  if (client->serverPort==-1)
    client->vncRec->readTimestamp = TRUE;
  if (!ReadFromRFBServer(client, (char *)&msg, 1))
    return FALSE;

  switch (msg.type) {

  case rfbSetColourMapEntries:
  {
    /* TODO:
    int i;
    uint16_t rgb[3];
    XColor xc;

    if (!ReadFromRFBServer(client, ((char *)&msg) + 1,
			   sz_rfbSetColourMapEntriesMsg - 1))
      return FALSE;

    msg.scme.firstColour = rfbClientSwap16IfLE(msg.scme.firstColour);
    msg.scme.nColours = rfbClientSwap16IfLE(msg.scme.nColours);

    for (i = 0; i < msg.scme.nColours; i++) {
      if (!ReadFromRFBServer(client, (char *)rgb, 6))
	return FALSE;
      xc.pixel = msg.scme.firstColour + i;
      xc.red = rfbClientSwap16IfLE(rgb[0]);
      xc.green = rfbClientSwap16IfLE(rgb[1]);
      xc.blue = rfbClientSwap16IfLE(rgb[2]);
      xc.flags = DoRed|DoGreen|DoBlue;
      XStoreColor(dpy, cmap, &xc);
    }
    */

    break;
  }

  case rfbFramebufferUpdate:
  {
    rfbFramebufferUpdateRectHeader rect;
    int linesToRead;
    int bytesPerLine;
    int i;

    if (!ReadFromRFBServer(client, ((char *)&msg.fu) + 1,
			   sz_rfbFramebufferUpdateMsg - 1))
      return FALSE;

    msg.fu.nRects = rfbClientSwap16IfLE(msg.fu.nRects);

    for (i = 0; i < msg.fu.nRects; i++) {
      if (!ReadFromRFBServer(client, (char *)&rect, sz_rfbFramebufferUpdateRectHeader))
	return FALSE;

      rect.encoding = rfbClientSwap32IfLE(rect.encoding);
      if (rect.encoding == rfbEncodingLastRect)
	break;

      rect.r.x = rfbClientSwap16IfLE(rect.r.x);
      rect.r.y = rfbClientSwap16IfLE(rect.r.y);
      rect.r.w = rfbClientSwap16IfLE(rect.r.w);
      rect.r.h = rfbClientSwap16IfLE(rect.r.h);


      if (rect.encoding == rfbEncodingXCursor ||
	  rect.encoding == rfbEncodingRichCursor) {

	if (!HandleCursorShape(client,
			       rect.r.x, rect.r.y, rect.r.w, rect.r.h,
			       rect.encoding)) {
	  return FALSE;
	}
	continue;
      }

      if (rect.encoding == rfbEncodingPointerPos) {
	if (!client->HandleCursorPos(client,rect.r.x, rect.r.y)) {
	  return FALSE;
	}
	continue;
      }
      
      if (rect.encoding == rfbEncodingKeyboardLedState) {
          /* OK! We have received a keyboard state message!!! */
          client->KeyboardLedStateEnabled = 1;
          if (client->HandleKeyboardLedState!=NULL)
              client->HandleKeyboardLedState(client, rect.r.x, 0);
          /* stash it for the future */
          client->CurrentKeyboardLedState = rect.r.x;
          continue;
      }

      if (rect.encoding == rfbEncodingNewFBSize) {
	client->width = rect.r.w;
	client->height = rect.r.h;
	client->MallocFrameBuffer(client);
	SendFramebufferUpdateRequest(client, 0, 0, rect.r.w, rect.r.h, FALSE);
	rfbClientLog("Got new framebuffer size: %dx%d\n", rect.r.w, rect.r.h);
	continue;
      }

      /* rect.r.w=byte count */
      if (rect.encoding == rfbEncodingSupportedMessages) {
          int loop;
          if (!ReadFromRFBServer(client, (char *)&client->supportedMessages, sz_rfbSupportedMessages))
              return FALSE;

          /* msgs is two sets of bit flags of supported messages client2server[] and server2client[] */
          /* currently ignored by this library */

          rfbClientLog("client2server supported messages (bit flags)\n");
          for (loop=0;loop<32;loop+=8)
            rfbClientLog("%02X: %04x %04x %04x %04x - %04x %04x %04x %04x\n", loop,
                client->supportedMessages.client2server[loop],   client->supportedMessages.client2server[loop+1],
                client->supportedMessages.client2server[loop+2], client->supportedMessages.client2server[loop+3],
                client->supportedMessages.client2server[loop+4], client->supportedMessages.client2server[loop+5],
                client->supportedMessages.client2server[loop+6], client->supportedMessages.client2server[loop+7]);

          rfbClientLog("server2client supported messages (bit flags)\n");
          for (loop=0;loop<32;loop+=8)
            rfbClientLog("%02X: %04x %04x %04x %04x - %04x %04x %04x %04x\n", loop,
                client->supportedMessages.server2client[loop],   client->supportedMessages.server2client[loop+1],
                client->supportedMessages.server2client[loop+2], client->supportedMessages.server2client[loop+3],
                client->supportedMessages.server2client[loop+4], client->supportedMessages.server2client[loop+5],
                client->supportedMessages.server2client[loop+6], client->supportedMessages.server2client[loop+7]);
          continue;
      }

      /* rect.r.w=byte count, rect.r.h=# of encodings */
      if (rect.encoding == rfbEncodingSupportedEncodings) {
          char *buffer;
          buffer = malloc(rect.r.w);
          if (!ReadFromRFBServer(client, buffer, rect.r.w))
          {
              free(buffer);
              return FALSE;
          }

          /* buffer now contains rect.r.h # of uint32_t encodings that the server supports */
          /* currently ignored by this library */
          free(buffer);
          continue;
      }

      /* rect.r.w=byte count */
      if (rect.encoding == rfbEncodingServerIdentity) {
          char *buffer;
          buffer = malloc(rect.r.w+1);
          if (!ReadFromRFBServer(client, buffer, rect.r.w))
          {
              free(buffer);
              return FALSE;
          }
          buffer[rect.r.w]=0; /* null terminate, just in case */
          rfbClientLog("Connected to Server \"%s\"\n", buffer);
          free(buffer);
          continue;
      }

      /* rfbEncodingUltraZip is a collection of subrects.   x = # of subrects, and h is always 0 */
      if (rect.encoding != rfbEncodingUltraZip)
      {
        if ((rect.r.x + rect.r.w > client->width) ||
	    (rect.r.y + rect.r.h > client->height))
	    {
	      rfbClientLog("Rect too large: %dx%d at (%d, %d)\n",
	  	  rect.r.w, rect.r.h, rect.r.x, rect.r.y);
	      return FALSE;
            }

        /* UltraVNC with scaling, will send rectangles with a zero W or H
         *
        if ((rect.encoding != rfbEncodingTight) && 
            (rect.r.h * rect.r.w == 0))
        {
	  rfbClientLog("Zero size rect - ignoring (encoding=%d (0x%08x) %dx, %dy, %dw, %dh)\n", rect.encoding, rect.encoding, rect.r.x, rect.r.y, rect.r.w, rect.r.h);
	  continue;
        }
        */
        
        /* If RichCursor encoding is used, we should prevent collisions
	   between framebuffer updates and cursor drawing operations. */
        client->SoftCursorLockArea(client, rect.r.x, rect.r.y, rect.r.w, rect.r.h);
      }

      switch (rect.encoding) {

      case rfbEncodingRaw: {
	int y=rect.r.y, h=rect.r.h;

	bytesPerLine = rect.r.w * client->format.bitsPerPixel / 8;
	linesToRead = RFB_BUFFER_SIZE / bytesPerLine;

	while (h > 0) {
	  if (linesToRead > h)
	    linesToRead = h;

	  if (!ReadFromRFBServer(client, client->buffer,bytesPerLine * linesToRead))
	    return FALSE;

	  CopyRectangle(client, (uint8_t *)client->buffer,
			   rect.r.x, y, rect.r.w,linesToRead);

	  h -= linesToRead;
	  y += linesToRead;

	}
      } break;

      case rfbEncodingCopyRect:
      {
	rfbCopyRect cr;

	if (!ReadFromRFBServer(client, (char *)&cr, sz_rfbCopyRect))
	  return FALSE;

	cr.srcX = rfbClientSwap16IfLE(cr.srcX);
	cr.srcY = rfbClientSwap16IfLE(cr.srcY);

	/* If RichCursor encoding is used, we should extend our
	   "cursor lock area" (previously set to destination
	   rectangle) to the source rectangle as well. */
	client->SoftCursorLockArea(client,
				   cr.srcX, cr.srcY, rect.r.w, rect.r.h);

        if (client->GotCopyRect != NULL) {
          client->GotCopyRect(client, cr.srcX, cr.srcY, rect.r.w, rect.r.h,
              rect.r.x, rect.r.y);
        } else
		CopyRectangleFromRectangle(client,
				   cr.srcX, cr.srcY, rect.r.w, rect.r.h,
				   rect.r.x, rect.r.y);

	break;
      }

      case rfbEncodingRRE:
      {
	switch (client->format.bitsPerPixel) {
	case 8:
	  if (!HandleRRE8(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 16:
	  if (!HandleRRE16(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 32:
	  if (!HandleRRE32(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	}
	break;
      }

      case rfbEncodingCoRRE:
      {
	switch (client->format.bitsPerPixel) {
	case 8:
	  if (!HandleCoRRE8(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 16:
	  if (!HandleCoRRE16(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 32:
	  if (!HandleCoRRE32(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	}
	break;
      }

      case rfbEncodingHextile:
      {
	switch (client->format.bitsPerPixel) {
	case 8:
	  if (!HandleHextile8(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 16:
	  if (!HandleHextile16(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 32:
	  if (!HandleHextile32(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	}
	break;
      }

      case rfbEncodingUltra:
      {
        switch (client->format.bitsPerPixel) {
        case 8:
          if (!HandleUltra8(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return FALSE;
          break;
        case 16:
          if (!HandleUltra16(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return FALSE;
          break;
        case 32:
          if (!HandleUltra32(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return FALSE;
          break;
        }
        break;
      }
      case rfbEncodingUltraZip:
      {
        switch (client->format.bitsPerPixel) {
        case 8:
          if (!HandleUltraZip8(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return FALSE;
          break;
        case 16:
          if (!HandleUltraZip16(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return FALSE;
          break;
        case 32:
          if (!HandleUltraZip32(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
            return FALSE;
          break;
        }
        break;
      }

#ifdef LIBVNCSERVER_HAVE_LIBZ
      case rfbEncodingZlib:
      {
	switch (client->format.bitsPerPixel) {
	case 8:
	  if (!HandleZlib8(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 16:
	  if (!HandleZlib16(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 32:
	  if (!HandleZlib32(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	}
	break;
     }

#ifdef LIBVNCSERVER_HAVE_LIBJPEG
      case rfbEncodingTight:
      {
	switch (client->format.bitsPerPixel) {
	case 8:
	  if (!HandleTight8(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 16:
	  if (!HandleTight16(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 32:
	  if (!HandleTight32(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	}
	break;
      }
#endif
      case rfbEncodingZRLE:
	/* Fail safe for ZYWRLE unsupport VNC server. */
	client->appData.qualityLevel = 9;
	/* fall through */
      case rfbEncodingZYWRLE:
      {
	switch (client->format.bitsPerPixel) {
	case 8:
	  if (!HandleZRLE8(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	case 16:
	  if (client->si.format.greenMax > 0x1F) {
	    if (!HandleZRLE16(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	      return FALSE;
	  } else {
	    if (!HandleZRLE15(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	      return FALSE;
	  }
	  break;
	case 32:
	{
	  uint32_t maxColor=(client->format.redMax<<client->format.redShift)|
		(client->format.greenMax<<client->format.greenShift)|
		(client->format.blueMax<<client->format.blueShift);
	  if ((client->format.bigEndian && (maxColor&0xff)==0) ||
	      (!client->format.bigEndian && (maxColor&0xff000000)==0)) {
	    if (!HandleZRLE24(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	      return FALSE;
	  } else if (!client->format.bigEndian && (maxColor&0xff)==0) {
	    if (!HandleZRLE24Up(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	      return FALSE;
	  } else if (client->format.bigEndian && (maxColor&0xff000000)==0) {
	    if (!HandleZRLE24Down(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	      return FALSE;
	  } else if (!HandleZRLE32(client, rect.r.x,rect.r.y,rect.r.w,rect.r.h))
	    return FALSE;
	  break;
	}
	}
	break;
     }

#endif

      default:
	 {
	   rfbBool handled = FALSE;
	   rfbClientProtocolExtension* e;

	   for(e = rfbClientExtensions; !handled && e; e = e->next)
	     if(e->handleEncoding && e->handleEncoding(client, &rect))
	       handled = TRUE;

	   if(!handled) {
	     rfbClientLog("Unknown rect encoding %d\n",
		 (int)rect.encoding);
	     return FALSE;
	   }
	 }
      }

      /* Now we may discard "soft cursor locks". */
      client->SoftCursorUnlockScreen(client);

      client->GotFrameBufferUpdate(client, rect.r.x, rect.r.y, rect.r.w, rect.r.h);
    }

    if (!SendIncrementalFramebufferUpdateRequest(client))
      return FALSE;

    if (client->FinishedFrameBufferUpdate)
      client->FinishedFrameBufferUpdate(client);

    break;
  }

  case rfbBell:
  {
    client->Bell(client);

    break;
  }

  case rfbServerCutText:
  {
    char *buffer;

    if (!ReadFromRFBServer(client, ((char *)&msg) + 1,
			   sz_rfbServerCutTextMsg - 1))
      return FALSE;

    msg.sct.length = rfbClientSwap32IfLE(msg.sct.length);

    buffer = malloc(msg.sct.length+1);

    if (!ReadFromRFBServer(client, buffer, msg.sct.length))
      return FALSE;

    buffer[msg.sct.length] = 0;

    if (client->GotXCutText)
      client->GotXCutText(client, buffer, msg.sct.length);

    free(buffer);

    break;
  }

  case rfbTextChat:
  {
      char *buffer=NULL;
      if (!ReadFromRFBServer(client, ((char *)&msg) + 1,
                             sz_rfbTextChatMsg- 1))
        return FALSE;
      msg.tc.length = rfbClientSwap32IfLE(msg.sct.length);
      switch(msg.tc.length) {
      case rfbTextChatOpen:
          rfbClientLog("Received TextChat Open\n");
          if (client->HandleTextChat!=NULL)
              client->HandleTextChat(client, (int)rfbTextChatOpen, NULL);
          break;
      case rfbTextChatClose:
          rfbClientLog("Received TextChat Close\n");
         if (client->HandleTextChat!=NULL)
              client->HandleTextChat(client, (int)rfbTextChatClose, NULL);
          break;
      case rfbTextChatFinished:
          rfbClientLog("Received TextChat Finished\n");
         if (client->HandleTextChat!=NULL)
              client->HandleTextChat(client, (int)rfbTextChatFinished, NULL);
          break;
      default:
          buffer=malloc(msg.tc.length+1);
          if (!ReadFromRFBServer(client, buffer, msg.tc.length))
          {
              free(buffer);
              return FALSE;
          }
          /* Null Terminate <just in case> */
          buffer[msg.tc.length]=0;
          rfbClientLog("Received TextChat \"%s\"\n", buffer);
          if (client->HandleTextChat!=NULL)
              client->HandleTextChat(client, (int)msg.tc.length, buffer);
          free(buffer);
          break;
      }
      break;
  }

  case rfbResizeFrameBuffer:
  {
    if (!ReadFromRFBServer(client, ((char *)&msg) + 1,
                           sz_rfbResizeFrameBufferMsg -1))
      return FALSE;
    client->width = rfbClientSwap16IfLE(msg.rsfb.framebufferWidth);
    client->height = rfbClientSwap16IfLE(msg.rsfb.framebufferHeigth);
    client->MallocFrameBuffer(client);
    SendFramebufferUpdateRequest(client, 0, 0, client->width, client->height, FALSE);
    rfbClientLog("Got new framebuffer size: %dx%d\n", client->width, client->height);
    break;
  }

  case rfbPalmVNCReSizeFrameBuffer:
  {
    if (!ReadFromRFBServer(client, ((char *)&msg) + 1,
                           sz_rfbPalmVNCReSizeFrameBufferMsg -1))
      return FALSE;
    client->width = rfbClientSwap16IfLE(msg.prsfb.buffer_w);
    client->height = rfbClientSwap16IfLE(msg.prsfb.buffer_h);
    client->MallocFrameBuffer(client);
    SendFramebufferUpdateRequest(client, 0, 0, client->width, client->height, FALSE);
    rfbClientLog("Got new framebuffer size: %dx%d\n", client->width, client->height);
    break;
  }

  default:
    {
      rfbBool handled = FALSE;
      rfbClientProtocolExtension* e;

      for(e = rfbClientExtensions; !handled && e; e = e->next)
	if(e->handleMessage && e->handleMessage(client, &msg))
	  handled = TRUE;

      if(!handled) {
	char buffer[256];
	rfbClientLog("Unknown message type %d from VNC server\n",msg.type);
	ReadFromRFBServer(client, buffer, 256);
	return FALSE;
      }
    }
  }

  return TRUE;
}


#define GET_PIXEL8(pix, ptr) ((pix) = *(ptr)++)

#define GET_PIXEL16(pix, ptr) (((uint8_t*)&(pix))[0] = *(ptr)++, \
			       ((uint8_t*)&(pix))[1] = *(ptr)++)

#define GET_PIXEL32(pix, ptr) (((uint8_t*)&(pix))[0] = *(ptr)++, \
			       ((uint8_t*)&(pix))[1] = *(ptr)++, \
			       ((uint8_t*)&(pix))[2] = *(ptr)++, \
			       ((uint8_t*)&(pix))[3] = *(ptr)++)

/* CONCAT2 concatenates its two arguments.  CONCAT2E does the same but also
   expands its arguments if they are macros */

#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#define CONCAT3(a,b,c) a##b##c
#define CONCAT3E(a,b,c) CONCAT3(a,b,c)

#define BPP 8
#include "rre.c"
#include "corre.c"
#include "hextile.c"
#include "ultra.c"
#include "zlib.c"
#include "tight.c"
#include "zrle.c"
#undef BPP
#define BPP 16
#include "rre.c"
#include "corre.c"
#include "hextile.c"
#include "ultra.c"
#include "zlib.c"
#include "tight.c"
#include "zrle.c"
#define REALBPP 15
#include "zrle.c"
#undef BPP
#define BPP 32
#include "rre.c"
#include "corre.c"
#include "hextile.c"
#include "ultra.c"
#include "zlib.c"
#include "tight.c"
#include "zrle.c"
#define REALBPP 24
#include "zrle.c"
#define REALBPP 24
#define UNCOMP 8
#include "zrle.c"
#define REALBPP 24
#define UNCOMP -8
#include "zrle.c"
#undef BPP


/*
 * PrintPixelFormat.
 */

void
PrintPixelFormat(rfbPixelFormat *format)
{
  if (format->bitsPerPixel == 1) {
    rfbClientLog("  Single bit per pixel.\n");
    rfbClientLog(
	    "  %s significant bit in each byte is leftmost on the screen.\n",
	    (format->bigEndian ? "Most" : "Least"));
  } else {
    rfbClientLog("  %d bits per pixel.\n",format->bitsPerPixel);
    if (format->bitsPerPixel != 8) {
      rfbClientLog("  %s significant byte first in each pixel.\n",
	      (format->bigEndian ? "Most" : "Least"));
    }
    if (format->trueColour) {
      rfbClientLog("  TRUE colour: max red %d green %d blue %d"
		   ", shift red %d green %d blue %d\n",
		   format->redMax, format->greenMax, format->blueMax,
		   format->redShift, format->greenShift, format->blueShift);
    } else {
      rfbClientLog("  Colour map (not true colour).\n");
    }
  }
}

/* avoid name clashes with LibVNCServer */

#define rfbEncryptBytes rfbClientEncryptBytes
#define rfbEncryptBytes2 rfbClientEncryptBytes2
#define rfbDes rfbClientDes
#define rfbDesKey rfbClientDesKey
#define rfbUseKey rfbClientUseKey
#define rfbCPKey rfbClientCPKey

#include "../libvncserver/vncauth.c"
#include "../libvncserver/d3des.c"
