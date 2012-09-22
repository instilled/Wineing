
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
void *my_zmq_ctx = zmq_init(1);

chan* chan_init(const char *fqcn, int type)
{
  log(LOG_DEBUG, "Initializing channel [%s]", fqcn);

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

void chan_bind(chan *c)
{
  log(LOG_DEBUG, "Binding channel [%s]", c->fqcn);

  c->sock = zmq_socket(my_zmq_ctx, _to_zmq_type(c->type));

  if(c->sock == NULL) {
    log(LOG_ERROR,
        "Failed creating socket. Error '%s'",
        zmq_strerror(errno));
    return;
  }

  int rc = 0;
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

  if(rc < 0) {
    log(LOG_ERROR,
        "Failed connecting to socket. Error '%s'",
        zmq_strerror(errno));
    return;
  }

  if(c->type == CHAN_TYPE_SUB) {
    // subscribe to all messages
    zmq_setsockopt (c->sock, ZMQ_SUBSCRIBE, "", 0);
  }
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


// int chan_recv_ctrl(chan *c, WineingCtrlProto::Request &r)
// {
//   zmq_msg_t message;
//   zmq_msg_init (&message);

//   // Blocking until message is available.
//   int rc = zmq_recv (c->sock, &message, 0);
//   if(rc < 0) {
//     log(LOG_ERROR,                                                      \
//         "Failed receiving data on control channel [%s]. Error %s.",     \
//         c->fqcn,                                                        \
//         zmq_strerror(errno));
//     // zmq_msg_close(&message);
//     return rc;
//   }

//   // Copy the content of the zmq message to a string
//   int size = zmq_msg_size (&message);
//   void * data = zmq_msg_data(&message);

//   google::protobuf::io::ArrayInputStream is (data, size);
//   //  istringstream is (str, istringstream::in);

//   bool p = r.ParseFromZeroCopyStream(&is);
//   zmq_msg_close(&message);

//   if(!p) {
//     log(LOG_ERROR, "Failed parsing message");
//     return -2;
//   }

//   return 0;
// }
// void chan_send_market(chan *c)//, WineingMarketDataProto::MarketData &msg)
// {
//   // Statically allocate memory will be freed by the runtime
//   // itself. No need to call free/delete.
//   // TODO: Check how much memory is really needed at max!
//   // Use msg.ByteSize()
//   static char buffer[2048];

//   // // This is where we tell protobuf to serialize the message without
//   // // using memcopy but instead reference the memory directly.
//   // google::protobuf::io::ArrayOutputStream os (buffer, 2048);
//   // msg.SerializeToZeroCopyStream(&os);

//   // Sends the data referenced by BUFFER (zero-copy).
//   zmq_msg_t out;
//   void *hint = NULL;
//   //  zmq_msg_init_data (&out, buffer, msg.ByteSize(), _chan_send_free, hint);
//   zmq_send (c->sock, &out, 0);
// }
