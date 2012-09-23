#ifndef _CHAN_H
#define _CHAN_H

#include <zmq.h>
#include "logging/logging.h"

#define CHAN_RECV_BLOCK     0
#define CHAN_RECV_NOBLOCK   ZMQ_NOBLOCK

#define CHAN_TYPE_PUB            0
#define CHAN_TYPE_SUB            1
#define CHAN_TYPE_REQ            2
#define CHAN_TYPE_REP            3
#define CHAN_TYPE_PULL_BIND      4
#define CHAN_TYPE_PULL_CONNECT   5
#define CHAN_TYPE_PUSH_BIND      6
#define CHAN_TYPE_PUSH_CONNECT   7


/**
 * A channel definition. The current implementation uses ZMQ as the
 * underlying communication layer. It hides implementation details.
 */
typedef struct {
  const char *fqcn;
  void *ctx;
  void *sock;
  int type;
} chan;

/**
 * A user provide receive function. The function shall transform the
 * data received from the channel (pointed by *data*). The function is
 * invoked with the *size* of the message, the *data* from the
 * channel, and finally the a void pointer *obj* provided by the user
 * when calling *chan_recv* function.
 */
typedef int (*chan_recvFn)(void *data, size_t size, void *obj);
typedef void (*chan_sendFreeFn)(void *buffer, void *hint);

/**
 * Initializes a zmq channel. Allocates a new chan struct, invokes
 * zmq_init and finally zmq_socket. Clients should choose IPC as the
 * transport protocol, i.e. *ipc:///tmp/wineing/chan/in*. *chan_init*
 * WILL NOT create directories to match the *fqcn*, the client is
 * required to do so.
 *
 * \param fqcn
 * \param type One of CHAN_TYPE_*
 *
 * \sa http://api.zeromq.org/2-1:zmq-socket#toc8
 * \sa http://api.zeromq.org/2-1:zmq-socket
 */
chan* chan_init(const char *fqcn, int type);

/**
 * Binds chan to its endpoint. After a call to *chan_bind* the socket
 * is ready for use. ZMQ sockets are not thread-safe. If the
 * application uses multiple threads it is recommended to invoke
 * *chan_bind* in each thread separately.
 *
 * \sa http://api.zeromq.org/2-1:zmq-ipc
 */
void chan_bind(chan *c);

/**
 * Closes a chan. Closing involves (in that order):
 * - closing the zmq socket
 *
 * Obviously any reference to chan * will be invalid after calling
 * *chan_free*.
 *
 * \param c Pointer to the chan to be closed.
 */
void chan_destroy(chan *c);

/**
 * Terminates the zmq socket. The use of any sockets associated to the
 * ZMQ context will fail. Usually invoked when the application is
 * shutdown. An application should invoke *chan_free* first.
 */
void chan_shutdown();

/**
 * Receives a message from the control channel. If no message is
 * available *chan_recv_ctrl* will block until a message is available.
 *
 * \param c     The chan (control channel) to receive a message from.
 * \param msg   Request object (valid only if return == 1).
 * \return      If ZMQ call failed -1, -2 if parsing of message failed,
 *              or 0 in case of success.
 */
inline int chan_recv(chan *c, chan_recvFn fn, void *obj)
{
  zmq_msg_t message;
  zmq_msg_init (&message);

  // Blocking until message is available.
  int rc = zmq_recv (c->sock, &message, 0);
  if(rc < 0) {
    log(LOG_ERROR,                                                      \
        "Failed receiving data on control channel [%s]. Error %s.",     \
        c->fqcn,                                                        \
        zmq_strerror(errno));
    // zmq_msg_close(&message);
    return rc;
  }

  size_t size = zmq_msg_size (&message);
  void * data = zmq_msg_data(&message);
  rc = fn(data, size, obj);
  zmq_msg_close(&message);
  return rc;
}

inline int chan_send(chan *c,                              \
                     void *buffer,                         \
                     size_t size,                          \
                     chan_sendFreeFn freeFn = 0)
{
  zmq_msg_t out;
  void *hint = 0;
  zmq_msg_init_data (&out, buffer, size, freeFn, hint);
  return zmq_send (c->sock, &out, 0);
}


#endif /* _CHAN_H */
