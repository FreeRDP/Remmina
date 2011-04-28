/*
 *  Copyright (C) 2009 Vic Lee.
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

#include <rfb/rfbclient.h>
#include <errno.h>
#ifdef WIN32
#undef SOCKET
#include <windows.h>           /* for Sleep() */
#define sleep(X) Sleep(1000*X) /* MinGW32 has no sleep() */
#include <winsock2.h>
#define read(sock,buf,len) recv(sock,buf,len,0)
#define write(sock,buf,len) send(sock,buf,len,0)
#endif
#include "tls.h"

#ifdef LIBVNCSERVER_WITH_CLIENT_TLS

static const char *rfbTLSPriority = "NORMAL:+DHE-DSS:+RSA:+DHE-RSA:+SRP";
static const char *rfbAnonTLSPriority= "NORMAL:+ANON-DH";

#define DH_BITS 1024
static gnutls_dh_params_t rfbDHParams;

static rfbBool rfbTLSInitialized = FALSE;

static rfbBool
InitializeTLS(void)
{
  int ret;

  if (rfbTLSInitialized) return TRUE;
  if ((ret = gnutls_global_init()) < 0 ||
      (ret = gnutls_dh_params_init(&rfbDHParams)) < 0 ||
      (ret = gnutls_dh_params_generate2(rfbDHParams, DH_BITS)) < 0)
  {
    rfbClientLog("Failed to initialized GnuTLS: %s.\n", gnutls_strerror(ret));
    return FALSE;
  }
  rfbClientLog("GnuTLS initialized.\n");
  rfbTLSInitialized = TRUE;
  return TRUE;
}

/*
 * On Windows, translate WSAGetLastError() to errno values as GNU TLS does it
 * internally too. This is necessary because send() and recv() on Windows
 * don't set errno when they fail but GNUTLS expects a proper errno value.
 *
 * Use gnutls_transport_set_global_errno() like the GNU TLS documentation
 * suggests to avoid problems with different errno variables when GNU TLS and
 * libvncclient are linked to different versions of msvcrt.dll.
 */
#ifdef WIN32
static void WSAtoTLSErrno()
{
  switch(WSAGetLastError()) {
  case WSAEWOULDBLOCK:
    gnutls_transport_set_global_errno(EAGAIN);
    break;
  case WSAEINTR:
    gnutls_transport_set_global_errno(EINTR);
    break;
  default:
    gnutls_transport_set_global_errno(EIO);
    break;
  }
}
#endif


static ssize_t
PushTLS(gnutls_transport_ptr_t transport, const void *data, size_t len)
{
  rfbClient *client = (rfbClient*)transport;
  int ret;

  while (1)
  {
    ret = write(client->sock, data, len);
    if (ret < 0)
    {
#ifdef WIN32
      WSAtoTLSErrno();
#endif
      if (errno == EINTR) continue;
      return -1;
    }
    return ret;
  }
}


static ssize_t
PullTLS(gnutls_transport_ptr_t transport, void *data, size_t len)
{
  rfbClient *client = (rfbClient*)transport;
  int ret;

  while (1)
  {
    ret = read(client->sock, data, len);
    if (ret < 0)
    {
#ifdef WIN32
      WSAtoTLSErrno();
#endif
      if (errno == EINTR) continue;
      return -1;
    }
    return ret;
  }
}

static rfbBool
InitializeTLSSession(rfbClient* client, rfbBool anonTLS)
{
  int ret;
  const char *p;

  if (client->tlsSession) return TRUE;

  if ((ret = gnutls_init(&client->tlsSession, GNUTLS_CLIENT)) < 0)
  {
    rfbClientLog("Failed to initialized TLS session: %s.\n", gnutls_strerror(ret));
    return FALSE;
  }

  if ((ret = gnutls_priority_set_direct(client->tlsSession,
    anonTLS ? rfbAnonTLSPriority : rfbTLSPriority, &p)) < 0)
  {
    rfbClientLog("Warning: Failed to set TLS priority: %s (%s).\n", gnutls_strerror(ret), p);
  }

  gnutls_transport_set_ptr(client->tlsSession, (gnutls_transport_ptr_t)client);
  gnutls_transport_set_push_function(client->tlsSession, PushTLS);
  gnutls_transport_set_pull_function(client->tlsSession, PullTLS);

  rfbClientLog("TLS session initialized.\n");

  return TRUE;
}

static rfbBool
SetTLSAnonCredential(rfbClient* client)
{
  gnutls_anon_client_credentials anonCred;
  int ret;

  if ((ret = gnutls_anon_allocate_client_credentials(&anonCred)) < 0 ||
      (ret = gnutls_credentials_set(client->tlsSession, GNUTLS_CRD_ANON, anonCred)) < 0)
  {
    FreeTLS(client);
    rfbClientLog("Failed to create anonymous credentials: %s", gnutls_strerror(ret));
    return FALSE;
  }
  rfbClientLog("TLS anonymous credential created.\n");
  return TRUE;
}

static rfbBool
HandshakeTLS(rfbClient* client)
{
  int timeout = 15;
  int ret;

  while (timeout > 0 && (ret = gnutls_handshake(client->tlsSession)) < 0)
  {
    if (!gnutls_error_is_fatal(ret))
    {
      rfbClientLog("TLS handshake blocking.\n");
      sleep(1);
      timeout--;
      continue;
    }
    rfbClientLog("TLS handshake failed: %s.\n", gnutls_strerror(ret));
    FreeTLS(client);
    return FALSE;
  }

  if (timeout <= 0)
  {
    rfbClientLog("TLS handshake timeout.\n");
    FreeTLS(client);
    return FALSE;
  }

  rfbClientLog("TLS handshake done.\n");
  return TRUE;
}

/* VeNCrypt sub auth. 1 byte auth count, followed by count * 4 byte integers */
static rfbBool
ReadVeNCryptSecurityType(rfbClient* client, uint32_t *result)
{
    uint8_t count=0;
    uint8_t loop=0;
    uint8_t flag=0;
    uint32_t tAuth[256], t;
    char buf1[500],buf2[10];
    uint32_t authScheme;

    if (!ReadFromRFBServer(client, (char *)&count, 1)) return FALSE;

    if (count==0)
    {
        rfbClientLog("List of security types is ZERO. Giving up.\n");
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
        if (!ReadFromRFBServer(client, (char *)&tAuth[loop], 4)) return FALSE;
        t=rfbClientSwap32IfLE(tAuth[loop]);
        rfbClientLog("%d) Received security type %d\n", loop, t);
        if (flag) continue;
        if (t==rfbVeNCryptTLSNone ||
            t==rfbVeNCryptTLSVNC ||
            t==rfbVeNCryptTLSPlain ||
            t==rfbVeNCryptX509None ||
            t==rfbVeNCryptX509VNC ||
            t==rfbVeNCryptX509Plain)
        {
            flag++;
            authScheme=t;
            rfbClientLog("Selecting security type %d (%d/%d in the list)\n", authScheme, loop, count);
            /* send back 4 bytes (in original byte order!) indicating which security type to use */
            if (!WriteToRFBServer(client, (char *)&tAuth[loop], 4)) return FALSE;
        }
        tAuth[loop]=t;
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
        rfbClientLog("Unknown VeNCrypt authentication scheme from VNC server: %s\n",
               buf1);
        return FALSE;
    }
    *result = authScheme;
    return TRUE;
}

static void
FreeX509Credential(rfbCredential *cred)
{
  if (cred->x509Credential.x509CACertFile) free(cred->x509Credential.x509CACertFile);
  if (cred->x509Credential.x509CACrlFile) free(cred->x509Credential.x509CACrlFile);
  if (cred->x509Credential.x509ClientCertFile) free(cred->x509Credential.x509ClientCertFile);
  if (cred->x509Credential.x509ClientKeyFile) free(cred->x509Credential.x509ClientKeyFile);
  free(cred);
}

static gnutls_certificate_credentials_t
CreateX509CertCredential(rfbCredential *cred)
{
  gnutls_certificate_credentials_t x509_cred;
  int ret;

  if (!cred->x509Credential.x509CACertFile)
  {
    rfbClientLog("No CA certificate provided.\n");
    return NULL;
  }

  if ((ret = gnutls_certificate_allocate_credentials(&x509_cred)) < 0)
  {
    rfbClientLog("Cannot allocate credentials: %s.\n", gnutls_strerror(ret));
    return NULL;
  }
  if ((ret = gnutls_certificate_set_x509_trust_file(x509_cred,
    cred->x509Credential.x509CACertFile, GNUTLS_X509_FMT_PEM)) < 0)
  {
    rfbClientLog("Cannot load CA credentials: %s.\n", gnutls_strerror(ret));
    gnutls_certificate_free_credentials (x509_cred);
    return NULL;
  }
  if (cred->x509Credential.x509ClientCertFile && cred->x509Credential.x509ClientKeyFile)
  {
    if ((ret = gnutls_certificate_set_x509_key_file(x509_cred,
      cred->x509Credential.x509ClientCertFile, cred->x509Credential.x509ClientKeyFile,
      GNUTLS_X509_FMT_PEM)) < 0)
    {
      rfbClientLog("Cannot load client certificate or key: %s.\n", gnutls_strerror(ret));
      gnutls_certificate_free_credentials (x509_cred);
      return NULL;
    }
  } else
  {
    rfbClientLog("No client certificate or key provided.\n");
  }
  if (cred->x509Credential.x509CACrlFile)
  {
    if ((ret = gnutls_certificate_set_x509_crl_file(x509_cred,
      cred->x509Credential.x509CACrlFile, GNUTLS_X509_FMT_PEM)) < 0)
    {
      rfbClientLog("Cannot load CRL: %s.\n", gnutls_strerror(ret));
      gnutls_certificate_free_credentials (x509_cred);
      return NULL;
    }
  } else
  {
    rfbClientLog("No CRL provided.\n");
  }
  gnutls_certificate_set_dh_params (x509_cred, rfbDHParams);
  return x509_cred;
}

#endif

rfbBool
HandleAnonTLSAuth(rfbClient* client)
{
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS

  if (!InitializeTLS() || !InitializeTLSSession(client, TRUE)) return FALSE;

  if (!SetTLSAnonCredential(client)) return FALSE;

  if (!HandshakeTLS(client)) return FALSE;

  return TRUE;

#else
  rfbClientLog("TLS is not supported.\n");
  return FALSE;
#endif
}

rfbBool
HandleVeNCryptAuth(rfbClient* client)
{
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
  uint8_t major, minor, status;
  uint32_t authScheme;
  rfbBool anonTLS;
  gnutls_certificate_credentials_t x509_cred = NULL;
  int ret;

  if (!InitializeTLS()) return FALSE;

  /* Read VeNCrypt version */
  if (!ReadFromRFBServer(client, (char *)&major, 1) ||
      !ReadFromRFBServer(client, (char *)&minor, 1))
  {
    return FALSE;
  }
  rfbClientLog("Got VeNCrypt version %d.%d from server.\n", (int)major, (int)minor);

  if (major != 0 && minor != 2)
  {
    rfbClientLog("Unsupported VeNCrypt version.\n");
    return FALSE;
  }

  if (!WriteToRFBServer(client, (char *)&major, 1) ||
      !WriteToRFBServer(client, (char *)&minor, 1) ||
      !ReadFromRFBServer(client, (char *)&status, 1))
  {
    return FALSE;
  }

  if (status != 0)
  {
    rfbClientLog("Server refused VeNCrypt version %d.%d.\n", (int)major, (int)minor);
    return FALSE;
  }

  if (!ReadVeNCryptSecurityType(client, &authScheme)) return FALSE;
  if (!ReadFromRFBServer(client, (char *)&status, 1) || status != 1)
  {
    rfbClientLog("Server refused VeNCrypt authentication %d (%d).\n", authScheme, (int)status);
    return FALSE;
  }
  client->subAuthScheme = authScheme;

  /* Some VeNCrypt security types are anonymous TLS, others are X509 */
  switch (authScheme)
  {
    case rfbVeNCryptTLSNone:
    case rfbVeNCryptTLSVNC:
    case rfbVeNCryptTLSPlain:
      anonTLS = TRUE;
      break;
    default:
      anonTLS = FALSE;
      break;
  }

  /* Get X509 Credentials if it's not anonymous */
  if (!anonTLS)
  {
    rfbCredential *cred;

    if (!client->GetCredential)
    {
      rfbClientLog("GetCredential callback is not set.\n");
      return FALSE;
    }
    cred = client->GetCredential(client, rfbCredentialTypeX509);
    if (!cred)
    {
      rfbClientLog("Reading credential failed\n");
      return FALSE;
    }

    x509_cred = CreateX509CertCredential(cred);
    FreeX509Credential(cred);
    if (!x509_cred) return FALSE;
  }

  /* Start up the TLS session */
  if (!InitializeTLSSession(client, anonTLS)) return FALSE;

  if (anonTLS)
  {
    if (!SetTLSAnonCredential(client)) return FALSE;
  }
  else
  {
    if ((ret = gnutls_credentials_set(client->tlsSession, GNUTLS_CRD_CERTIFICATE, x509_cred)) < 0)
    {
      rfbClientLog("Cannot set x509 credential: %s.\n", gnutls_strerror(ret));
      FreeTLS(client);
      return FALSE;
    }
  }

  if (!HandshakeTLS(client)) return FALSE;

  /* TODO: validate certificate */

  /* We are done here. The caller should continue with client->subAuthScheme
   * to do actual sub authentication.
   */
  return TRUE;

#else
  rfbClientLog("TLS is not supported.\n");
  return FALSE;
#endif
}

int
ReadFromTLS(rfbClient* client, char *out, unsigned int n)
{
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
  ssize_t ret;

  ret = gnutls_record_recv(client->tlsSession, out, n);
  if (ret >= 0) return ret;
  if (ret == GNUTLS_E_REHANDSHAKE || ret == GNUTLS_E_AGAIN)
  {
    errno = EAGAIN;
  } else
  {
    rfbClientLog("Error reading from TLS: %s.\n", gnutls_strerror(ret));
    errno = EINTR;
  }
  return -1;
#else
  rfbClientLog("TLS is not supported.\n");
  errno = EINTR;
  return -1;
#endif
}

int
WriteToTLS(rfbClient* client, char *buf, unsigned int n)
{
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
  unsigned int offset = 0;
  ssize_t ret;

  while (offset < n)
  {
    ret = gnutls_record_send(client->tlsSession, buf+offset, (size_t)(n-offset));
    if (ret == 0) continue;
    if (ret < 0)
    {
      if (ret == GNUTLS_E_AGAIN || ret == GNUTLS_E_INTERRUPTED) continue;
      rfbClientLog("Error writing to TLS: %s.\n", gnutls_strerror(ret));
      return -1;
    }
    offset += (unsigned int)ret;
  }
  return offset;
#else
  rfbClientLog("TLS is not supported.\n");
  errno = EINTR;
  return -1;
#endif
}

void FreeTLS(rfbClient* client)
{
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
  if (client->tlsSession)
  {
    gnutls_deinit(client->tlsSession);
    client->tlsSession = NULL;
  }
#endif
}
