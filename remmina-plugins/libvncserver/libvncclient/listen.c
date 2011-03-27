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
 * listen.c - listen for incoming connections
 */

#ifdef __STRICT_ANSI__
#define _BSD_SOURCE
#endif
#include <unistd.h>
#include <sys/types.h>
#ifdef __MINGW32__
#define close closesocket
#include <winsock2.h>
#undef max
#else
#include <sys/wait.h>
#include <sys/utsname.h>
#endif
#include <sys/time.h>
#include <rfb/rfbclient.h>

/*
 * listenForIncomingConnections() - listen for incoming connections from
 * servers, and fork a new process to deal with each connection.
 */

void
listenForIncomingConnections(rfbClient* client)
{
#ifdef __MINGW32__
  /* FIXME */
  rfbClientErr("listenForIncomingConnections on MinGW32 NOT IMPLEMENTED\n");
  return;
#else
  int listenSocket;
  fd_set fds;

  client->listenSpecified = TRUE;

  listenSocket = ListenAtTcpPort(client->listenPort);

  if ((listenSocket < 0))
    return;

  rfbClientLog("%s -listen: Listening on port %d\n",
	  client->programName,client->listenPort);
  rfbClientLog("%s -listen: Command line errors are not reported until "
	  "a connection comes in.\n", client->programName);

  while (TRUE) {

    /* reap any zombies */
    int status, pid;
    while ((pid= wait3(&status, WNOHANG, (struct rusage *)0))>0);

    /* TODO: callback for discard any events (like X11 events) */

    FD_ZERO(&fds); 

    FD_SET(listenSocket, &fds);

    select(listenSocket+1, &fds, NULL, NULL, NULL);

    if (FD_ISSET(listenSocket, &fds)) {
      client->sock = AcceptTcpConnection(listenSocket);
      if (client->sock < 0)
	return;
      if (!SetNonBlocking(client->sock))
	return;

      /* Now fork off a new process to deal with it... */

      switch (fork()) {

      case -1: 
	rfbClientErr("fork\n"); 
	return;

      case 0:
	/* child - return to caller */
	close(listenSocket);
	return;

      default:
	/* parent - go round and listen again */
	close(client->sock); 
	break;
      }
    }
  }
#endif
}



/*
 * listenForIncomingConnectionsNoFork() - listen for incoming connections
 * from servers, but DON'T fork, instead just wait timeout microseconds.
 * If timeout is negative, block indefinitly.
 * Returns 1 on success (there was an incoming connection on the listen socket
 * and we accepted it successfully), -1 on error, 0 on timeout.
 */

int
listenForIncomingConnectionsNoFork(rfbClient* client, int timeout)
{
  fd_set fds;
  struct timeval to;
  int r;

  to.tv_sec= timeout / 1000000;
  to.tv_usec= timeout % 1000000;

  client->listenSpecified = TRUE;

  if (client->listenSock < 0)
    {
      client->listenSock = ListenAtTcpPort(client->listenPort);

      if (client->listenSock < 0)
	return -1;

      rfbClientLog("%s -listennofork: Listening on port %d\n",
		   client->programName,client->listenPort);
      rfbClientLog("%s -listennofork: Command line errors are not reported until "
		   "a connection comes in.\n", client->programName);
    }

  FD_ZERO(&fds);

  FD_SET(client->listenSock, &fds);

  if (timeout < 0)
    r = select(client->listenSock+1, &fds, NULL, NULL, NULL);
  else
    r = select(client->listenSock+1, &fds, NULL, NULL, &to);

  if (r > 0)
    {
      client->sock = AcceptTcpConnection(client->listenSock);
      if (client->sock < 0)
	return -1;
      if (!SetNonBlocking(client->sock))
	return -1;

      close(client->listenSock);
      return r;
    }

  /* r is now either 0 (timeout) or -1 (error) */
  return r;
}


