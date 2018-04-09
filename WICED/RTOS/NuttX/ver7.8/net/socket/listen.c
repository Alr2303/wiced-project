/****************************************************************************
 * net/socket/listen.c
 *
 *   Copyright (C) 2007-2009, 201-2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#if defined(CONFIG_NET) && CONFIG_NSOCKET_DESCRIPTORS > 0

#include <sys/socket.h>
#include <errno.h>
#include <assert.h>
#include <debug.h>

#include "tcp/tcp.h"
#include "local/local.h"
#include "socket/socket.h"

#include "syslog.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Function: psock_listen
 *
 * Description:
 *   To accept connections, a socket is first created with psock_socket(), a
 *   willingness to accept incoming connections and a queue limit for
 *   incoming connections are specified with psock_listen(), and then the
 *   connections are accepted with psock_accept(). The psock_listen() call
 *   applies only to sockets of type SOCK_STREAM or SOCK_SEQPACKET.
 *
 * Parameters:
 *   psock    Reference to an internal, boound socket structure.
 *   backlog  The maximum length the queue of pending connections may grow.
 *            If a connection request arrives with the queue full, the client
 *            may receive an error with an indication of ECONNREFUSED or,
 *            if the underlying protocol supports retransmission, the request
 *            may be ignored so that retries succeed.
 *
 * Returned Value:
 *   On success, zero is returned. On error, -1 is returned, and errno is set
 *   appropriately.
 *
 *   EADDRINUSE
 *     Another socket is already listening on the same port.
 *   EOPNOTSUPP
 *     The socket is not of a type that supports the listen operation.
 *
 ****************************************************************************/

int psock_listen(FAR struct socket *psock, int backlog)
{
  int err;

  DEBUGASSERT(psock != NULL);

  /* Verify that the sockfd corresponds to a connected SOCK_STREAM */

  if (psock->s_type != SOCK_STREAM || !psock->s_conn)
    {
      err = EOPNOTSUPP;
      goto errout;
    }

#ifdef CONFIG_NET_LOCAL
#ifdef CONFIG_NET_TCP
  if (psock->s_domain == PF_LOCAL)
#endif
    {
      FAR struct local_conn_s *conn =
        (FAR struct local_conn_s *)psock->s_conn;

      err = local_listen(conn, backlog);
      if (err < 0)
        {
          err = -err;
          goto errout;
        }
    }
#endif /* CONFIG_NET_LOCAL */

#ifdef CONFIG_NET_TCP
#ifdef CONFIG_NET_LOCAL
  else
#endif
    {
      FAR struct tcp_conn_s *conn =
        (FAR struct tcp_conn_s *)psock->s_conn;

      if (conn->lport <= 0)
        {
          err = EOPNOTSUPP;
          goto errout;
        }

      /* Set up the backlog for this connection */

#ifdef CONFIG_NET_TCPBACKLOG
      err = tcp_backlogcreate(conn, backlog);
      if (err < 0)
        {
          err = -err;
          goto errout;
        }
#endif

      /* Start listening to the bound port.  This enables callbacks when
       * accept() is called and enables poll()/select() logic.
       */

      err = tcp_listen(conn);
      if (err < 0)
        {
          if (err == -EADDRINUSE)
            {
              struct tcp_conn_s * oldconn = tcp_findlistener(conn->lport);
              tcp_unlisten(oldconn);
              err = tcp_listen(conn);
              syslog(LOG_ERR, "setup tcp_listen port again ret=%d\n", err);
            }
          if(err < 0)
            {
              err = -err;
              goto errout;              
            }
        }
    }
#endif /* CONFIG_NET_TCP */

  psock->s_flags |= _SF_LISTENING;
  return OK;

errout:
  set_errno(err);
  return ERROR;
}

/****************************************************************************
 * Function: listen
 *
 * Description:
 *   To accept connections, a socket is first created with socket(), a
 *   willingness to accept incoming connections and a queue limit for incoming
 *   connections are specified with listen(), and then the connections are
 *   accepted with accept(). The listen() call applies only to sockets of
 *   type SOCK_STREAM or SOCK_SEQPACKET.
 *
 * Parameters:
 *   sockfd   Socket descriptor of the bound socket
 *   backlog  The maximum length the queue of pending connections may grow.
 *            If a connection request arrives with the queue full, the client
 *            may receive an error with an indication of ECONNREFUSED or,
 *            if the underlying protocol supports retransmission, the request
 *            may be ignored so that retries succeed.
 *
 * Returned Value:
 *   On success, zero is returned. On error, -1 is returned, and errno is set
 *   appropriately.
 *
 *   EADDRINUSE
 *     Another socket is already listening on the same port.
 *   EBADF
 *     The argument 'sockfd' is not a valid descriptor.
 *   ENOTSOCK
 *     The argument 'sockfd' is not a socket.
 *   EOPNOTSUPP
 *     The socket is not of a type that supports the listen operation.
 *
 ****************************************************************************/

int listen(int sockfd, int backlog)
{
  FAR struct socket *psock = sockfd_socket(sockfd);
  int err;

  /* Verify that the sockfd corresponds to valid, allocated socket */

  if (psock == NULL || psock->s_crefs <= 0)
    {
      /* It is not a valid socket description.  Distinguish between the
       * cases where sockfd is a just invalid and when it is a valid file
       * descriptor used in the wrong context.
       */

#if CONFIG_NFILE_DESCRIPTORS > 0
      if ((unsigned int)sockfd < CONFIG_NFILE_DESCRIPTORS)
        {
          err = ENOTSOCK;
        }
      else
#endif
        {
          err = EBADF;
        }

      set_errno(err);
      return ERROR;
    }

  /* The let psock_listen to the work. If psock_listen() fails, it will have
   * set the errno variable.
   */

  return psock_listen(psock, backlog);
}

#endif /* CONFIG_NET && CONFIG_NSOCKET_DESCRIPTORS */