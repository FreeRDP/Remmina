/*
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
 * vncviewer.c - the Xt-based VNC viewer.
 */

#ifdef __STRICT_ANSI__
#define _BSD_SOURCE
#define _POSIX_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <rfb/rfbclient.h>
#include "tls.h"

static void Dummy(rfbClient* client) {
}
static rfbBool DummyPoint(rfbClient* client, int x, int y) {
  return TRUE;
}
static void DummyRect(rfbClient* client, int x, int y, int w, int h) {
}

#ifdef __MINGW32__
static char* NoPassword(rfbClient* client) {
  return strdup("");
}
#undef SOCKET
#include <winsock2.h>
#define close closesocket
#else
#include <stdio.h>
#include <termios.h>
#endif

static char* ReadPassword(rfbClient* client) {
#ifdef __MINGW32__
	/* FIXME */
	rfbClientErr("ReadPassword on MinGW32 NOT IMPLEMENTED\n");
	return NoPassword(client);
#else
	int i;
	char* p=malloc(9);
	struct termios save,noecho;
	p[0]=0;
	if(tcgetattr(fileno(stdin),&save)!=0) return p;
	noecho=save; noecho.c_lflag &= ~ECHO;
	if(tcsetattr(fileno(stdin),TCSAFLUSH,&noecho)!=0) return p;
	fprintf(stderr,"Password: ");
	i=0;
	while(1) {
		int c=fgetc(stdin);
		if(c=='\n')
			break;
		if(i<8) {
			p[i]=c;
			i++;
			p[i]=0;
		}
	}
	tcsetattr(fileno(stdin),TCSAFLUSH,&save);
	return p;
#endif
}
static rfbBool MallocFrameBuffer(rfbClient* client) {
  if(client->frameBuffer)
    free(client->frameBuffer);
  client->frameBuffer=malloc(client->width*client->height*client->format.bitsPerPixel/8);
  return client->frameBuffer?TRUE:FALSE;
}

static void initAppData(AppData* data) {
	data->shareDesktop=TRUE;
	data->viewOnly=FALSE;
	data->encodingsString="tight zrle ultra copyrect hextile zlib corre rre raw";
	data->useBGR233=FALSE;
	data->nColours=0;
	data->forceOwnCmap=FALSE;
	data->forceTrueColour=FALSE;
	data->requestedDepth=0;
	data->compressLevel=3;
	data->qualityLevel=5;
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
	data->enableJPEG=TRUE;
#else
	data->enableJPEG=FALSE;
#endif
	data->useRemoteCursor=FALSE;
}

rfbClient* rfbGetClient(int bitsPerSample,int samplesPerPixel,
			int bytesPerPixel) {
  rfbClient* client=(rfbClient*)calloc(sizeof(rfbClient),1);
  if(!client) {
    rfbClientErr("Couldn't allocate client structure!\n");
    return NULL;
  }
  initAppData(&client->appData);
  client->endianTest = 1;
  client->programName="";
  client->serverHost=strdup("");
  client->serverPort=5900;

  client->destHost = NULL;
  client->destPort = 5900;
  
  client->CurrentKeyboardLedState = 0;
  client->HandleKeyboardLedState = (HandleKeyboardLedStateProc)DummyPoint;

  /* default: use complete frame buffer */ 
  client->updateRect.x = -1;
 
  client->format.bitsPerPixel = bytesPerPixel*8;
  client->format.depth = bitsPerSample*samplesPerPixel;
  client->appData.requestedDepth=client->format.depth;
  client->format.bigEndian = *(char *)&client->endianTest?FALSE:TRUE;
  client->format.trueColour = TRUE;

  if (client->format.bitsPerPixel == 8) {
    client->format.redMax = 7;
    client->format.greenMax = 7;
    client->format.blueMax = 3;
    client->format.redShift = 0;
    client->format.greenShift = 3;
    client->format.blueShift = 6;
  } else {
    client->format.redMax = (1 << bitsPerSample) - 1;
    client->format.greenMax = (1 << bitsPerSample) - 1;
    client->format.blueMax = (1 << bitsPerSample) - 1;
    if(!client->format.bigEndian) {
      client->format.redShift = 0;
      client->format.greenShift = bitsPerSample;
      client->format.blueShift = bitsPerSample * 2;
    } else {
      if(client->format.bitsPerPixel==8*3) {
	client->format.redShift = bitsPerSample*2;
	client->format.greenShift = bitsPerSample*1;
	client->format.blueShift = 0;
      } else {
	client->format.redShift = bitsPerSample*3;
	client->format.greenShift = bitsPerSample*2;
	client->format.blueShift = bitsPerSample;
      }
    }
  }

  client->bufoutptr=client->buf;
  client->buffered=0;

#ifdef LIBVNCSERVER_HAVE_LIBZ
  client->raw_buffer_size = -1;
  client->decompStreamInited = FALSE;

#ifdef LIBVNCSERVER_HAVE_LIBJPEG
  memset(client->zlibStreamActive,0,sizeof(rfbBool)*4);
  client->jpegSrcManager = NULL;
#endif
#endif

  client->HandleCursorPos = DummyPoint;
  client->SoftCursorLockArea = DummyRect;
  client->SoftCursorUnlockScreen = Dummy;
  client->GotFrameBufferUpdate = DummyRect;
  client->FinishedFrameBufferUpdate = NULL;
  client->GetPassword = ReadPassword;
  client->MallocFrameBuffer = MallocFrameBuffer;
  client->Bell = Dummy;
  client->CurrentKeyboardLedState = 0;
  client->HandleKeyboardLedState = (HandleKeyboardLedStateProc)DummyPoint;
  client->QoS_DSCP = 0;

  client->authScheme = 0;
  client->subAuthScheme = 0;
  client->GetCredential = NULL;
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
  client->tlsSession = NULL;
#endif
  client->sock = -1;
  client->listenSock = -1;
  client->clientAuthSchemes = NULL;
  return client;
}

static rfbBool rfbInitConnection(rfbClient* client)
{
  /* Unless we accepted an incoming connection, make a TCP connection to the
     given VNC server */

  if (!client->listenSpecified) {
    if (!client->serverHost)
      return FALSE;
    if (client->destHost) {
      if (!ConnectToRFBRepeater(client,client->serverHost,client->serverPort,client->destHost,client->destPort))
        return FALSE;
    } else {
      if (!ConnectToRFBServer(client,client->serverHost,client->serverPort))
        return FALSE;
    }
  }

  /* Initialise the VNC connection, including reading the password */

  if (!InitialiseRFBConnection(client))
    return FALSE;

  client->width=client->si.framebufferWidth;
  client->height=client->si.framebufferHeight;
  client->MallocFrameBuffer(client);

  if (!SetFormatAndEncodings(client))
    return FALSE;

  if (client->updateRect.x < 0) {
    client->updateRect.x = client->updateRect.y = 0;
    client->updateRect.w = client->width;
    client->updateRect.h = client->height;
  }

  if (client->appData.scaleSetting>1)
  {
      if (!SendScaleSetting(client, client->appData.scaleSetting))
          return FALSE;
      if (!SendFramebufferUpdateRequest(client,
			      client->updateRect.x / client->appData.scaleSetting,
			      client->updateRect.y / client->appData.scaleSetting,
			      client->updateRect.w / client->appData.scaleSetting,
			      client->updateRect.h / client->appData.scaleSetting,
			      FALSE))
	      return FALSE;
  }
  else
  {
      if (!SendFramebufferUpdateRequest(client,
			      client->updateRect.x, client->updateRect.y,
			      client->updateRect.w, client->updateRect.h,
			      FALSE))
      return FALSE;
  }

  return TRUE;
}

rfbBool rfbInitClient(rfbClient* client,int* argc,char** argv) {
  int i,j;

  if(argv && argc && *argc) {
    if(client->programName==0)
      client->programName=argv[0];

    for (i = 1; i < *argc; i++) {
      j = i;
      if (strcmp(argv[i], "-listen") == 0) {
	listenForIncomingConnections(client);
	break;
      } else if (strcmp(argv[i], "-listennofork") == 0) {
	listenForIncomingConnectionsNoFork(client, -1);
	break;
      } else if (strcmp(argv[i], "-play") == 0) {
	client->serverPort = -1;
	j++;
      } else if (i+1<*argc && strcmp(argv[i], "-encodings") == 0) {
	client->appData.encodingsString = argv[i+1];
	j+=2;
      } else if (i+1<*argc && strcmp(argv[i], "-compress") == 0) {
	client->appData.compressLevel = atoi(argv[i+1]);
	j+=2;
      } else if (i+1<*argc && strcmp(argv[i], "-quality") == 0) {
	client->appData.qualityLevel = atoi(argv[i+1]);
	j+=2;
      } else if (i+1<*argc && strcmp(argv[i], "-scale") == 0) {
        client->appData.scaleSetting = atoi(argv[i+1]);
        j+=2;
      } else if (i+1<*argc && strcmp(argv[i], "-qosdscp") == 0) {
        client->QoS_DSCP = atoi(argv[i+1]);
        j+=2;
      } else if (i+1<*argc && strcmp(argv[i], "-repeaterdest") == 0) {
	char* colon=strchr(argv[i+1],':');

	if(client->destHost)
	  free(client->destHost);
        client->destPort = 5900;

	client->destHost = strdup(argv[i+1]);
	if(colon) {
	  client->destHost[(int)(colon-argv[i+1])] = '\0';
	  client->destPort = atoi(colon+1);
	}
        j+=2;
      } else {
	char* colon=strchr(argv[i],':');

	if(client->serverHost)
	  free(client->serverHost);

	if(colon) {
	  client->serverHost = strdup(argv[i]);
	  client->serverHost[(int)(colon-argv[i])] = '\0';
	  client->serverPort = atoi(colon+1);
	} else {
	  client->serverHost = strdup(argv[i]);
	}
	if(client->serverPort >= 0 && client->serverPort < 5900)
	  client->serverPort += 5900;
      }
      /* purge arguments */
      if (j>i) {
	*argc-=j-i;
	memmove(argv+i,argv+j,(*argc-i)*sizeof(char*));
	i--;
      }
    }
  }

  if(!rfbInitConnection(client)) {
    rfbClientCleanup(client);
    return FALSE;
  }

  return TRUE;
}

void rfbClientCleanup(rfbClient* client) {
#ifdef LIBVNCSERVER_HAVE_LIBZ
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
  int i;

  for ( i = 0; i < 4; i++ ) {
    if (client->zlibStreamActive[i] == TRUE ) {
      if (inflateEnd (&client->zlibStream[i]) != Z_OK &&
	  client->zlibStream[i].msg != NULL)
	rfbClientLog("inflateEnd: %s\n", client->zlibStream[i].msg);
    }
  }

  if ( client->decompStreamInited == TRUE ) {
    if (inflateEnd (&client->decompStream) != Z_OK &&
	client->decompStream.msg != NULL)
      rfbClientLog("inflateEnd: %s\n", client->decompStream.msg );
  }

  if (client->jpegSrcManager)
    free(client->jpegSrcManager);
#endif
#endif

#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
  FreeTLS(client);
#endif
  if (client->sock >= 0)
    close(client->sock);
  if (client->listenSock >= 0)
    close(client->listenSock);
  free(client->desktopName);
  free(client->serverHost);
  if (client->destHost)
    free(client->destHost);
  if (client->clientAuthSchemes)
    free(client->clientAuthSchemes);
  free(client);
}
