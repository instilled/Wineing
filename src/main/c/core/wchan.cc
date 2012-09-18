
#include <iostream>
#include <sstream>
#include <string>

#include <zmq.h>

#include "wchan.h"
#include "logging/logging.h"

//#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

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
 * \struct wchan
 */
struct wchan {
  const char *fqcn;
  void *ctx;
  void *sock;
  int type;
};

/**
 * Logs zmq errors printing a user defined error message followed by
 * ZMQ's error text.
 */
void log_zmq_error(wchan *c, const char * msg)
{
  std::ostringstream out;

  out << "[" << c->fqcn << "]: "
      << msg
      << " Error " << errno << " [" << zmq_strerror(errno) << "]"
      << endl;

  log(LOG_ERROR, out.str().c_str());
}

wchan* wchan_init(const char *fqcn, int type)
{
  log(LOG_DEBUG, "Initializind channel [%s]", fqcn);

  wchan *c = new wchan;
  c->fqcn = fqcn;
  c->type = type;
  c->ctx = zmq_init(1);

  switch(type) {
  case WCHAN_TYPE_CTRL:
    c->sock = zmq_socket(c->ctx, ZMQ_REP);
    break;

  case WCHAN_TYPE_MARKET:
    c->sock = zmq_socket(c->ctx, ZMQ_PUB);
    break;

  default:
    log(LOG_ERROR, "Invalid type, not one of WCHAN_TYPE_CTRL, "
                   "or WCHAN_TYPE_MARKET.");
  }

  return c;
}

/**
 * TODO: delete function
 */
void wchan_wait_for_client(const char * fqcn)
{
  log(LOG_DEBUG, "Waiting for client connection");

  // Initialization
  void *ctx = zmq_init(1);
  void *srv = zmq_socket(ctx, ZMQ_REP);
  zmq_bind(srv, fqcn);

  //  Wait for client
  zmq_msg_t request;
  zmq_msg_init (&request);
  zmq_recv (srv, &request, 0);
  zmq_msg_close (&request);

  //  Send reply
  zmq_msg_t reply;
  zmq_msg_init_size (&reply, 3);
  memcpy (zmq_msg_data (&reply), "OK!", 5);
  zmq_send (srv, &reply, 0);
  zmq_msg_close (&reply);

  //  Free resources
  zmq_close (srv);
  zmq_term (ctx);
}

void wchan_start(wchan *c)
{
  int r;

  log(LOG_DEBUG, "Binding channel [%s]", c->fqcn);

  switch(c->type)
    {
    case WCHAN_TYPE_MARKET:
    case WCHAN_TYPE_CTRL:
      // Connect outbound channel
      r = zmq_bind(c->sock, c->fqcn);
      if(r < 0) {
        log(LOG_ERROR,                                          \
            "Failed binding to ZMQ socket [%s]. Error %s.",     \
            c->fqcn,                                            \
            zmq_strerror(errno));
      }
      break;

    default:
      log(LOG_ERROR, "Invalid type, not one of WCHAN_TYPE_CTRL or "
                      "WCHAN_TYPE_MARKET");
  }
}

void wchan_stop(wchan *c)
{
  // does nothing currently
}

void wchan_free(wchan *c)
{
  zmq_close(c->sock);
  zmq_term(c->ctx);

  // TODO: delete...
  // delete c;
}

int wchan_recv_ctrl(wchan *c, WineingCtrlProto::Request &r)
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

  // Copy the content of the zmq message to a string
  int size = zmq_msg_size (&message);
  void * data = zmq_msg_data(&message);

  google::protobuf::io::ArrayInputStream is (data, size);
  //  istringstream is (str, istringstream::in);

  bool p = r.ParseFromZeroCopyStream(&is);
  zmq_msg_close(&message);

  if(!p) {
    log(LOG_ERROR, "Failed parsing message");
    return 0;
  }

  return 1;
}

/**
 * Invoked from ZMQ upon completing the send operation.
 */
void _wchan_send_free (void *buffer, void *hint)
{
  // We use statically allocated buffers so we don't really do
  // anything here... In a dynamically allocated buffer world we would
  // free any memory here.

  //free((char*) buffer);
}

void wchan_send_ctrl(wchan *c, WineingCtrlProto::Response &msg)
{
  // See wchan_send_market below for comments
  static char buffer[2048];

  int size = msg.ByteSize();

  google::protobuf::io::ArrayOutputStream os (buffer, 2048);
  msg.SerializeToZeroCopyStream(&os);

  zmq_msg_t out;
  void *hint = NULL;
  zmq_msg_init_data (&out, buffer, msg.ByteSize(), _wchan_send_free, hint);
  zmq_send (c->sock, &out, 0);
}

void wchan_send_market(wchan *c, WineingMarketDataProto::MarketData &msg)
{
  // Statically allocate memory will be freed by the runtime
  // itself. No need to call free/delete.
  // TODO: Check how much memory is really needed at max!
  // Use msg.ByteSize()
  static char buffer[2048];

  // This is where we tell protobuf to serialize the message without
  // using memcopy but instead reference the memory directly.
  google::protobuf::io::ArrayOutputStream os (buffer, 2048);
  msg.SerializeToZeroCopyStream(&os);

  // Sends the data referenced by BUFFER (zero-copy).
  zmq_msg_t out;
  void *hint = NULL;
  zmq_msg_init_data (&out, buffer, msg.ByteSize(), _wchan_send_free, hint);
  zmq_send (c->sock, &out, 0);
}
