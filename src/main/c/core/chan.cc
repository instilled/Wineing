
#include "chan.h"

using namespace std;

/*
  Notes on C/C++

  C++ requires explicit casting of void pointers, C doesn't. There's
  more to the story. See
  http://stackoverflow.com/questions/3477741/why-does-c-require-a-cast-for-malloc-but-c-doesnt

  To remembered: Use the correct memory management constructs. When
  using C use malloc/free and with C++ new/delete. Also note that you
  should never mix them (malloc/delete and/or new/free) as this can
  lead to subtle memory errors.
*/

/**
 * The thread safe context instance shared among the application.
 */
void *my_zmq_ctx = zmq_init(2);

chan* chan_init(const char *fqcn, int type)
{
  chan *c = new chan;
  c->fqcn = fqcn;
  c->type = type;

  return c;
}


int _to_zmq_type(int type)
{
  int t = 0;
  switch(type)
    {
    case CHAN_TYPE_PUB:
      t = ZMQ_PUB;
      break;
    case CHAN_TYPE_SUB:
      t = ZMQ_SUB;
      break;
    case CHAN_TYPE_PUSH_BIND:
    case CHAN_TYPE_PUSH_CONNECT:
      t = ZMQ_PUSH;
      break;
    case CHAN_TYPE_PULL_BIND:
    case CHAN_TYPE_PULL_CONNECT:
      t = ZMQ_PULL;
      break;
    case CHAN_TYPE_REQ:
      t = ZMQ_REQ;
      break;
    case CHAN_TYPE_REP:
      t = ZMQ_REP;
      break;
    }

  return t;
}

int chan_bind(chan *c)
{
  c->sock = zmq_socket(my_zmq_ctx, _to_zmq_type(c->type));

  int rc = c->sock == NULL ? -1 : 0;

  if(rc == 0) {
    switch(c->type)
      {
      case CHAN_TYPE_PUB:
      case CHAN_TYPE_REP:
      case CHAN_TYPE_PUSH_BIND:
      case CHAN_TYPE_PULL_BIND:
        rc = zmq_bind(c->sock, c->fqcn);
        break;

      case CHAN_TYPE_SUB:
      case CHAN_TYPE_REQ:
      case CHAN_TYPE_PUSH_CONNECT:
      case CHAN_TYPE_PULL_CONNECT:
        rc = zmq_connect(c->sock, c->fqcn);
        break;

      default:
        log(LOG_ERROR, "Invalid CHAN type (%i)", c->type);
      }

    if(rc == 0 && c->type == CHAN_TYPE_SUB) {
      // subscribe to all messages
      zmq_setsockopt (c->sock, ZMQ_SUBSCRIBE, "", 0);
    }
  }
  return rc;
}

void chan_destroy(chan *c)
{
  zmq_close(c->sock);

  // TODO: delete...
  // delete c;
}

void chan_shutdown()
{
  zmq_term(my_zmq_ctx);
}

