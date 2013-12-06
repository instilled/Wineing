/*
 * wineing.win.cc
 *
 * Wining hides nxcore behind a protobuf based interface. Information
 * is exchanged over ZMQ.
 *
 * Wining provides two ZMQ channels for communication:
 *
 * - Control channel, a synchronous ZMQ socket(req/rep). The client
 *   application is supposed to connect to the channel with ZMQ's REQ
 *   socket option.
 *
 * - Market data channel: An asynchronous publish/subscribe
 *   channel. All the market data will be pushed to the client through
 *   this channel. The client is expected to connect with a ZMQ SUB
 *   socket. Client better be ready!
 */

/*
 * http://www.winehq.org/docs/winedev-guide/x2800
 */

#include "core/wineing.h"

#include "conc/conc.h"
#include "log/logging.h"
#include "net/chan.h"
#include "nx/nxinf.h"
#include "nx/nxtape.h"

#include "gen/WineingCtrlProto.pb.h"
#include "gen/WineingMarketDataProto.pb.h"

#include <unistd.h>
#include <pthread.h>
#include <sstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

/**
 * Global variables - declared extern in wineing.h.  These are shared
 * among multiple files, namely
 * - wineing.cc
 * - nxtape.win.cc
 */
w_ctrl g_data = {
  WINEING_CTRL_CMD_INIT,
  new char[WINEING_CTRL_DEFAULT_DATA_SIZE],
  0
};

/**
 * Used to make the *market_thread* sleep if the user has either not
 * started streaming or stopped it.
 */
static pthread_mutex_t g_market_sync_mutex;
static pthread_cond_t  g_market_sync_cond;

void wineing_init(w_ctx &ctx)
{
  log(LOG_INFO, "Initializing wineing");

  lazy_init(DEFAULTS_SHARED_VERSION_INIT);

  // Verify the version of the library we linked against is compatible
  // with the version of the headers generated.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Load the nxcore dll.
  if(0 > wininf_nxcore_load()) {
    log(LOG_ERROR, "Failed loading NxCore dll");
    exit(1);
  }

  pthread_mutex_init(&g_market_sync_mutex, NULL);
  pthread_cond_init(&g_market_sync_cond, NULL);
}

void wineing_run(w_ctx &ctx)
{
  // Good tutorial on posix threads
  // http://www.yolinux.com/TUTORIALS/LinuxTutorialPosixThreads.html
  pthread_t cchan_out_t;
  pthread_t cchan_in_t;
  pthread_t market_t;

  // The cchan_in_thread listens for incomming messages on cchan_in
  // managing the mchan_thread (market data channel) as requested by
  // the client.
  pthread_create(&cchan_out_t, NULL, cchan_out_thread, (void*)&ctx);
  pthread_create(&cchan_in_t, NULL, cchan_in_thread, (void*)&ctx);
  pthread_create(&market_t, NULL, market_thread, (void*)&ctx);

  // Wait for threads to finish
  pthread_join(market_t, NULL);
  pthread_join(cchan_out_t, NULL);
  pthread_join(cchan_in_t, NULL);
}

void wineing_shutdown(w_ctx &ctx)
{
  log(LOG_INFO, "Shutting down...");

  chan_shutdown();

  pthread_cond_destroy(&g_market_sync_cond);
  pthread_mutex_destroy(&g_market_sync_mutex);

  wininf_nxcore_free();

  // Free any protobuf specific resources
  google::protobuf::ShutdownProtobufLibrary();

  lazy_destroy();

  log(LOG_INFO, "Application shutdown successful.");
}

/**
 * Used by *chan_recv* to parse incoming data.
 *
 * \return 0 if successfull, -1 otherwise
 */
static inline int _recv_ctrl(void *data, size_t size, void *obj)
{
  WineingCtrlProto::Request *r =
    (WineingCtrlProto::Request*) obj;

  google::protobuf::io::ArrayInputStream is (data, size);

  bool p = r->ParseFromZeroCopyStream(&is);

  if(!p) {
    log(LOG_ERROR, "Failed parsing message");
    return -1;
  }

  return 0;
}

/**
 * Called by *chan_send* if it is safe to delete the data, that is it
 * has been successfully transferred to the socket.
 */
static inline void _send_free(void *buffer, void *hint)
{
  delete [] (char*)buffer;
}

/**
 * The controlling thread. It waits for the client to send control
 * messages to Wineing.
 */
void* cchan_in_thread(void *_ctx)
{
  using namespace WineingCtrlProto;

  w_ctx *ctx = (w_ctx *)_ctx;
  chan *cchan_out_inmem;
  chan *cchan_in;
  char *buffer;
  // Statically allocate variables to improve runtime performance
  static Request req;
  static Response res;
    static int t_version = DEFAULTS_SHARED_VERSION_INIT;
  static w_ctrl t_data = {
    WINEING_CTRL_CMD_INIT,
    new char[WINEING_CTRL_DEFAULT_DATA_SIZE],
    0
  };

  log(LOG_INFO, "Initializing control_in thread (%s)",
      ctx->conf->cchan_in_fqcn);

  // Initialize inbound channel
  cchan_in = chan_init(ctx->conf->cchan_in_fqcn, CHAN_TYPE_PULL_BIND);
  if(chan_bind(cchan_in) < 0) {
    log(LOG_ERROR, "Failed binding to cchan_in (%s). Error [%s]",
        ctx->conf->cchan_in_fqcn,
        chan_error());
    return NULL;
  }

  // We can not bind to the inproc channel unless it's been created.
  cchan_out_inmem = chan_init(DEFAULTS_ICHAN_NAME, CHAN_TYPE_PUSH_CONNECT);
  while(0 > chan_bind(cchan_out_inmem)) {
    sleep(1);
  }

  log(LOG_DEBUG, "Ready to accept client requests");

  while (WINEING_CTRL_CMD_SHUTDOWN < t_data.cmd) {
    std::stringstream err;
    std::stringstream tape;

    // Clear all objects (they're statically allocated and the data
    // needs to be cleared)
    req.Clear();
    res.Clear();

    // Receive data
    int rc = chan_recv(cchan_in, _recv_ctrl, &req);
    //log(LOG_DEBUG,
    //    "Received control request [id: %li, type: %d]",
    //    req.requestid(),
    //    req.type());

    if(0 < rc) {
      // Assume we will respond with an OK. Response::ERR is only set
      // in case one happens
      res.set_requestid(req.requestid());

      // Process control message
      switch(req.type())
        {
        case Request::MARKET_START:
          res.set_type(Response::MARKET_START_OK);

          // Check that the market data thread is not already
          // started. If so send an error back to the client.
          if(t_data.cmd == WINEING_CTRL_CMD_MARKET_RUN) {
            err << "Already in RUNNING state.";
            res.set_type(Response::MARKET_START_ERR_RUNNING);
            res.set_err_text(err.str());
            break;
          }

          t_data.size = 0;

          // If the tape file is empty or NULL NnXcore will start
          // streaming real-time data. Make sure NxCoreAccess is
          // running and connected to the NxCore servers. See
          // http://nxcoreapi.com/doc/concept_Introduction.html.
          if(req.has_tape_file()) {
            tape << ctx->conf->tape_basedir \
                 << req.tape_file();

            // Checks whether a file exists the windows way. Remember
            // we are loading the file with NxCore which is, well,
            // Windows.
            if(0 > wininf_file_exists(tape.str().c_str())) {
              err << "File '" << tape.str() << "' not found.";
              res.set_type(Response::ERR);
              res.set_err_text(err.str());
              log(LOG_DEBUG, err.str().c_str());
              break;
            }

            // String length plus NULL byte
            t_data.size = tape.str().length() + 1;
            memcpy(t_data.data,
                   tape.str().c_str(),
                   t_data.size);
          }

          // Update g_data
          t_data.cmd = WINEING_CTRL_CMD_MARKET_RUN;
          t_version = lazy_update_global_if_owner(t_version,
                                                  &t_data,
                                                  &g_data,
                                                  _copy_local_to_shared);

          pthread_mutex_lock( &g_market_sync_mutex );
          pthread_cond_signal( &g_market_sync_cond );
          pthread_mutex_unlock( &g_market_sync_mutex );
          break;

        case Request::MARKET_STOP:
          res.set_type(Response::MARKET_STOP_OK);
          t_data.cmd = WINEING_CTRL_CMD_MARKET_STOP;
          t_version = lazy_update_global_if_owner(t_version,
                                                  &t_data,
                                                  &g_data,
                                                  _copy_local_to_shared);
          break;

        case Request::SHUTDOWN:
          res.set_type(Response::SHUTDOWN_OK);
          t_data.cmd = WINEING_CTRL_CMD_SHUTDOWN;
          t_version = lazy_update_global_if_owner(t_version,
                                                  &t_data,
                                                  &g_data,
                                                  _copy_local_to_shared);

          // In case no START message was successfully processed by the
          // control thread notifing the market thread is still necessary.
          // Do that.
          pthread_mutex_lock( &g_market_sync_mutex );
          pthread_cond_signal( &g_market_sync_cond );
          pthread_mutex_unlock( &g_market_sync_mutex );
          break;
        }


      int buf_size = res.ByteSize();
      buffer = new char[buf_size];
      google::protobuf::io::ArrayOutputStream os (buffer, buf_size);
      res.SerializeToZeroCopyStream(&os);
      // log(LOG_DEBUG, "Sending Response [id: %li, type: %i]",
      //    res.requestid(),
      //    res.type()
      //    );
      if(0 > chan_send(cchan_out_inmem, buffer, buf_size, _send_free)) {
        log(LOG_WARN,
            "Failed sending message to inproc channel (%s). Error %s",
            DEFAULTS_ICHAN_NAME,
            chan_error());
      }
    }
  }

  chan_destroy(cchan_in);
  chan_destroy(cchan_out_inmem);

  log(LOG_INFO, "Shutting down control_in thread");

  return NULL;
}

/**
 * The thread processing NxCore messages.
 */
void* market_thread(void *_ctx)
{
  w_ctx *ctx = (w_ctx *)_ctx;

  static timespec timeout {
    0,     // seconds
    500000 // nanoseconds
  };
  // Thread local version of the shared state
  static int t_version = DEFAULTS_SHARED_VERSION_READ_INIT;
  static w_ctrl t_data = {
    WINEING_CTRL_CMD_INIT,
    new char[WINEING_CTRL_DEFAULT_DATA_SIZE],
    0
  };

  chan *mchan;
  chan *cchan_out_inmem;

  log(LOG_INFO, "Initializing market data thread (%s)",
      ctx->conf->mchan_fqcn);

  mchan = chan_init(ctx->conf->mchan_fqcn, CHAN_TYPE_PUB);
  if(0 > chan_bind(mchan)) {
    log(LOG_ERROR, "Failed binding mchan (%s). Error [%s]",
        ctx->conf->mchan_fqcn,
        chan_error());
    return NULL;
  }

  // We can not bind to the inproc channel unless it's been created.
  cchan_out_inmem = chan_init(DEFAULTS_ICHAN_NAME, CHAN_TYPE_PUSH_CONNECT);
  while(0 > chan_bind(cchan_out_inmem)) {
    sleep(1);
  }

  nxtape_init(cchan_out_inmem, mchan);

  while(1) {
    // NxCore callback will return upon successfully completing a tape
    // (day) but is ready to start again immediately thus the inner
    // while loop. Again the read on g_msg.ctrl is unsynchronized.
    while(1) {
      // Read the global state
      t_version = lazy_update_local_if_changed(t_version,
                                               &t_data,
                                               &g_data,
                                               _copy_shared_to_local);
      // Loop as long as no shutdown is requested (g_msg.ctrl == 0)
      if(t_data.cmd == WINEING_CTRL_CMD_MARKET_STOP) {
        log(LOG_INFO, "Stopping nxcore");
        break;
      } else if(t_data.cmd == WINEING_CTRL_CMD_SHUTDOWN) {
        log(LOG_INFO, "Shutting down market data thread");
        goto shutdown;
      } else if(t_data.cmd == WINEING_CTRL_CMD_MARKET_RUN) {
        log(LOG_DEBUG, "Running nxcore [tape: %s]",
            t_data.size == 0 ? "real-time" : t_data.data);
        wininf_nxcore_run(t_data.data, nxtape_process);
      } else {
        // Be nice to the cpu and sleep for a bit if no data was
        // requested.
        nanosleep(&timeout, 0);
      }
    }
  }

  // Do a proper shutdown freeing all resources.
 shutdown:
  chan_destroy(mchan);
  chan_destroy(cchan_out_inmem);
  return NULL;
}

/**
 * Used by cchan_out_thread. Allocates a buffer of size *size*.
 */
int _cchan_in_mem_copy(void *buffer, size_t size, void *out_buffer)
{
  memcpy(out_buffer, buffer, size);
  return 0;
}

void* cchan_out_thread(void *_ctx)
{
  using namespace WineingCtrlProto;

  w_ctx *ctx = (w_ctx *)_ctx;
  chan *cchan_out;
  chan *cchan_in_mem;
  int read;
  int t_version = DEFAULTS_SHARED_VERSION_READ_INIT;
  static w_ctrl t_data = {
    WINEING_CTRL_CMD_INIT,
    new char[WINEING_CTRL_DEFAULT_DATA_SIZE],
    0
  };
  char *buffer;

  log(LOG_INFO, "Initializing control_out thread (%s)",
      ctx->conf->cchan_out_fqcn);


  // This is where we send Response messages to the
  // client(s). ZMQ_PUSH is a fan-out type socket.
  cchan_out = chan_init(ctx->conf->cchan_out_fqcn, CHAN_TYPE_PUB);
  if(0 > chan_bind(cchan_out)) {
    log(LOG_ERROR, "Failed binding cchan_out (%s). Error [%s]",
        ctx->conf->cchan_out_fqcn,
        chan_error());
    return NULL;
  }

  // This initializes the inbound memory channel where we get Response
  // messages required to be sent to the client.
  cchan_in_mem = chan_init(DEFAULTS_ICHAN_NAME, CHAN_TYPE_PULL_BIND);
  if(0 > chan_bind(cchan_in_mem)) {
    log(LOG_ERROR, "Failed binding to cchan_in_mem (%s). Error [%s]",
        DEFAULTS_ICHAN_NAME,
        chan_error());
    return NULL;
  }

  while(1) {
    // Read the global state
    t_version = lazy_update_local_if_changed(t_version,
                                             &t_data,
                                             &g_data,
                                             _copy_shared_to_local);

    if(WINEING_CTRL_CMD_SHUTDOWN == t_data.cmd) {
      break;
    }

    // Allocate buffer for the data we're expecting. It is freed by as
    // soon as the data is on the wire by _send_free function provided
    // to chan_send below.
    buffer = new char[1024];

    // Receive the data
    read = chan_recv(cchan_in_mem,
                     _cchan_in_mem_copy,
                     buffer);
    if(0 > read) {
      log(LOG_WARN,
          "Failed reading message from inproc channel (%s). Error %s",
          DEFAULTS_ICHAN_NAME,
          chan_error());
      continue;
    }

    // Send the data and free the buffer.
    read = chan_send(cchan_out,
                      buffer,
                      read,
                     _send_free);
    if(0 > read) {
      log(LOG_WARN, "Sending control message failed. Error %s",
          chan_error());
    }
  }

  // Free all resources
  chan_destroy(cchan_in_mem);
  chan_destroy(cchan_out);
  log(LOG_INFO, "Shutting down control_out thread");

  return NULL;
}
