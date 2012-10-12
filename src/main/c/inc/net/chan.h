#ifndef _CHAN_H
#define _CHAN_H

#include <zmq.h>

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
typedef int (*chan_recvFn)(void *buffer, size_t size, void *obj);
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
 * \param c
 * \return
 *
 * \sa http://api.zeromq.org/2-1:zmq-ipc
 */
int chan_bind(chan *c);

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
 * After invocation of *fn* all ZMQ resources will be freed. The
 * application is therefore required to copy any data that should
 * persist the call to *fn*, e.g. by copying the buffer pointed by
 * **obj*.
 *
 * \param c     The chan (control channel) to receive a message from
 * \param fn    Function invoked to parse the message. The function
 *              is supposed to return -1 in case of error.
 * \param *obj  Pointer to a user provided value, e.g. ptr to a buffer
 * \return      If ZMQ call or message parsing fails -1, otherwise the
 *              number of bytes read
 */
inline int chan_recv(chan *c, chan_recvFn fn, void *obj)
{
  zmq_msg_t message;
  zmq_msg_init (&message);

  // Blocks until message is available. Abuse of zmq_recv's return
  // code. zmq_recv does not return the number of bytes read. Its
  // return value is either -1 in case of an error or 0 otherwise. In
  // case of success we set it to the number of bytes read.
  int read = zmq_recv(c->sock, &message, 0);
  if(read == 0) {
    read = zmq_msg_size (&message);
    void * data = zmq_msg_data(&message);
    if(0 > fn(data, read, obj)) {
      read = -1;
    }
    zmq_msg_close(&message);
  }
  return read;
}

inline int chan_send(chan *c,
                     void *buffer,
                     size_t size,
                     chan_sendFreeFn freeFn = NULL)
{
  zmq_msg_t out;
  zmq_msg_init_data(&out, buffer, size, freeFn, NULL);
  return zmq_send (c->sock, &out, 0);
}

inline const char * chan_error()
{
  return zmq_strerror(errno);
}

#endif /* _CHAN_H */
