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
 * TODO:
 *
 * - better error handling
 * - command line argument parsing
 * - add comments
 * - use tcmalloc (googles perftools library [provided by AUR on arch].
 *   > provides heapcheck, heapprofile, and cpuprofile (nicE!!!)
 *   > http://code.google.com/p/gperftools/wiki/GooglePerformanceTools
 * - efficient (space, time) market data structure (get familiar with
 *   NxCore message structure)
 * - logging facility uses its own thread
 * - interrupt CTRL-C and KILL signals (kind of a shutdown hook)
 *   > clean shutdown to free resources, e.g. memory, sockets, ...
 * - switch to asynchronous control channel
 * - profiling
 *   > gprof: http://www.cs.utah.edu/dept/old/texinfo/as/gprof.html
 *   > http://stackoverflow.com/questions/375913/w
 hat-can-i-use-to-profile-c-code-in-linux
 * - Traffic/Resource monitoring
 *   > list of good tools
 *     http://www.cyberciti.biz/tips/top-linux-monitoring-tools.html
 *   > ntop seems to be the best (rt, many features, web-server, ...),
 *     iptraf
 */

/*
  Notes on sharing state between threads in C/C++.

  Atomic operations are not supported by C/C++ (before c++11
  standard). Some compilers such as GCC or Intel C Compiler support
  compiler extensions that will make use of the cpu's instruction for
  atomic operations (memory barriers aka fence, cas,
  ...). Alternatives are: boost library (not preferred), cache line
  alignment, mutex (too slow for certain), use counter and check (poll
  state) periodically.

  I have yet to see if whether we achieve better performance using
  these extensions/functions or by cache line alignment.

  // This aligns code on 64 byte boundaries. The struct will thus
  // be padded with 56 bytes.
  typedef struct
  {
    int a __attribute__((aligned(64)));
    int b;
  } my_type;

*/

/*
  Notes on debugging.

  I have tried to debug the final executable. Unfortunately without
  success. This is (most probably) due to the fact that we are linking
  C, C++ and Windows code together in one executable.

  Unfortunately I did not figure out how to get winedbg to work with
  that language/platform mix. Maybe I'm completely wrong and things
  fails somewhere else.

  An alternative would be to get a zmq.dll and a protobuf.dll and do
  everything in the windows world. I doupt that IPC calls will be
  possible with that approach but again, I might just be wrong.

  There's a rather brief blogpost about building a zmq.dll with
  winelib
  http://wine.1045685.n5.nabble.com/building-a-winelib-dll-td3271834.html
*/
#include <windows.h>

#include <unistd.h>
#include <pthread.h>
#include <sstream>

#include "wineing.h"
#include "core/chan.h"

// Google protobuf generated headers
#include "WineingCtrlProto.pb.h"
#include "WineingMarketDataProto.pb.h"

#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

// TODO: Unsynchronized, potential threading issues with
// WineingMsg and WineingCtx

// Wineing uses these to communicate between threads.
static pthread_mutex_t sync_mutex;
static pthread_cond_t  cond_var;
static WineingMsg msg;

// The application context. Its access is unsyncronized. The variable
// is being written only while initializing Wiening (wineing_init). I
// have to use a global variable because of NxCoreSystem.UserData's
// design - its an INT! The documentation says its ok to share user
// provided value and also pointers with the callback. Indeed it
// is. But as its size is of type int and pointers on 64-bit
// architectures use, well, 64 instead of 32 bits the pointer is
// invalid! They should have used size_t instead, which would point to
// int on 32-bit and to long on 64-bit machines (actually I don't know
// if windows has size_t or something else.
// IMTPORTANT: ctx is not thread safe! especially wchan (wchan.sock)
// structs can (should) not be shared between threads!
//WineingCtx *ctx;

/**
 * Initiales Wineing. Includes (in that order)
 * - verifying Google Protocol generated header versions
 * - loading nxcore dll
 * - initializing ZMQ channels (cchan, chan)
 *
 */
void wineing_init(WineingCtx &ctx)
{
  log(LOG_INFO, "Initializing wineing");
  // Verify the version of the library we linked against is compatible
  // with the version of the headers generated.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Load dll.
  ctx.nxCoreLib = inxcore_load();
  if(!ctx.nxCoreLib) {
    log(LOG_ERROR, "Failed loading NxCore dll");
    exit(1);
  }

  pthread_mutex_init(&sync_mutex, NULL);
  pthread_cond_init(&cond_var, NULL);
}

/**
 *
 */
void wineing_run(WineingCtx &ctx)
{
  // Good tutorial on posix threads
  // http://www.yolinux.com/TUTORIALS/LinuxTutorialPosixThreads.html
  pthread_t ctrl_t;
  pthread_t market_t;

  // Wait for client to connect
  wineing_wait_for_client(ctx);

  // The control thread listens for incomming messages on cchan
  // managing the mchan (market data channel) as requested by the
  // client.
  pthread_create(&ctrl_t, NULL, ctrl_thread, (void*)&ctx);
  pthread_create(&market_t, NULL, market_thread, (void*)&ctx);

  // Wait for threads to finish
  pthread_join(ctrl_t, NULL);
  pthread_join(market_t, NULL);
}

/**
 * Frees allocated resources.
 */
void wineing_shutdown(WineingCtx &ctx)
{
  log(LOG_INFO, "Shutting down...");

  chan_shutdown();

  pthread_cond_destroy(&cond_var);
  pthread_mutex_destroy(&sync_mutex);

  inxcore_free(ctx.nxCoreLib);

  // Free any protobuf specific resources
  google::protobuf::ShutdownProtobufLibrary();
}


static int _recv_ctrl(int size, void *data, void *obj)
{
  WineingCtrlProto::Request *r =
    (WineingCtrlProto::Request*) obj;

  google::protobuf::io::ArrayInputStream is (data, size);

  bool p = r->ParseFromZeroCopyStream(&is);

  if(!p) {
    log(LOG_ERROR, "Failed parsing message");
    return -2;
  }

  return 0;
}

static void _send_free(void *buffer, void *hint)
{
  delete (char*)buffer;
}

int wineing_wait_for_client(WineingCtx &ctx)
{
  using namespace WineingCtrlProto;

  char *buffer = new char[2048];

  chan *sync = chan_init(ctx.conf->schan_fqcn, CHAN_TYPE_REP);
  chan_bind(sync);

  // Wait for client
  Request req;
  Response res;

  int rc = chan_recv(sync, _recv_ctrl, &req);

  if(rc == 0) {
    res.set_requestid(req.requestid());

    if(req.type() != Request::INIT &&
       !(req.has_cchan_fqcn())) {
      std::stringstream err;
      err << "Wrong Request type (INIT expected) or "
          << "missing control channel fqcn.";
      res.set_code(Response::ERR);
      res.set_text(err.str());
      rc = -1;

    } else {
      // Get the control channel endpoint names
      char *cchan_out = new char[req.cchan_fqcn().length()+1];
      strcpy(cchan_out, req.cchan_fqcn().c_str());
      ctx.cchan_fqcn_out = cchan_out;

      res.set_code(Response::INIT);
      res.set_cchan_fqcn(ctx.conf->cchan_fqcn);
      res.set_mchan_fqcn(ctx.conf->mchan_fqcn);
      rc = 0;

      log(LOG_DEBUG,
          "Exchanged ctrl [%s] endpoint",
          ctx.cchan_fqcn_out);
    }


    google::protobuf::io::ArrayOutputStream os (buffer, 2048);
    res.SerializeToZeroCopyStream(&os);
    chan_send(sync, buffer, res.ByteSize(), _send_free);

  } else {
    log(LOG_ERROR, "Failed parsing Request");
  }

  // Sync channel you're done!
  chan_destroy(sync);

  return rc;
}

/**
 * Prcesses each market data update from NxCore sends it through a ZMQ
 * channel to the client.
 */
int __stdcall processTape(const NxCoreSystem *pNxCoreSys, \
                          const NxCoreMessage *pNxCoreMsg)
{
  using namespace WineingMarketDataProto;

  // Use static vars to avoid reinitialization. This is potentially
  // risky if the UserData (mchan) changes. We know that Winieng never
  // reallocates a new chan structure during its lifetime.
  static MarketData m;

  // Because we reuse protobuf objects we have to clear them.
  m.Clear();
  /*
  switch( pNxCoreMsg->MessageType )
    {
    case NxMSG_STATUS:
      m.set_type(MarketData::STATUS);
      chan_send_market(ctx.mchan, m);
      break;

    case NxMSG_EXGQUOTE:
      m.set_type(MarketData::QUOTE_EX);
      chan_send_market(ctx.mchan, m);
      break;

    case NxMSG_MMQUOTE:
      m.set_type(MarketData::QUOTE_EX);
      chan_send_market(ctx.mchan, m);
      break;

    case NxMSG_TRADE:
      m.set_type(MarketData::QUOTE_EX);
      chan_send_market(ctx.mchan, m);
      break;

    case NxMSG_CATEGORY:
      m.set_type(MarketData::QUOTE_EX);
      chan_send_market(ctx.mchan, m);
      break;
      //case NxMSG_SYMBOLCHANGE:
      //break;
      //case NxMSG_SYMBOLSPIN:
      //break;
    }
*/

  // An unsynchronized read on a shared variable. We don't care about
  // the value being delayed. The cost of synchronization is too high.
  return msg.ctrl == WINEING_MSG_MARKET_RUN ? \
    NxCALLBACKRETURN_CONTINUE :               \
    NxCALLBACKRETURN_STOP;
}

/**
 * The thread processing NxCore messages.
 */
void* market_thread(void *_ctx)
{
  WineingCtx *ctx = (WineingCtx *)_ctx;

  // TODO: Access to ctx->cchan_fqcn_out is not thread-safe. Do we
  // need to access the variable through a memory barrier e.g. gcc's
  // __sync_* extensions? I guess it's safe because we created the
  // thread ways after we set the variable.
  chan *cchan_out = chan_init(ctx->cchan_fqcn_out, CHAN_TYPE_PUSH_CONNECT);
  chan *mchan  = chan_init(ctx->conf->cchan_fqcn, CHAN_TYPE_PUB);

  chan_bind(cchan_out);
  chan_bind(mchan);
  /*
  while(1) {
    pthread_mutex_lock( &sync_mutex );
    pthread_cond_wait( &cond_var, &sync_mutex );
    pthread_mutex_unlock( &sync_mutex );

    char * tape = (char*)msg.data;

    log(LOG_DEBUG, "Starting to stream market data [%s]",
        tape == NULL ? "real-time" : tape);

    // NxCore callback will return upon successfully completing a tape
    // (day) but is ready to start again immediately thus the inner
    // while loop. Again the read on msg.ctrl is unsynchronized.
    while(msg.ctrl == WINEING_MSG_MARKET_RUN) {
      inxcore_run(_ctx->nxCoreLib, processTape, tape, NULL);
    }

    // Loop as long as no shutdown is requested (msg.ctrl == 0)
    if(msg.ctrl == WINEING_MSG_MARKET_SHUTDOWN) {
      break;
    }
  }
*/
  chan_destroy(mchan);
  chan_destroy(cchan_out);

  return NULL;
}

/**
 * The controlling thread. It waits for the client to send control
 * messages to Wineing.
 */
void* ctrl_thread(void *_ctx)
{
  WineingCtx *ctx = (WineingCtx *)_ctx;

  // TODO: Access to ctx->cchan_fqcn_out is not thread-safe. Do we
  // need to access the variable through a memory barrier e.g. gcc's
  // __sync_* extensions? I guess it's safe because we created the
  // thread ways after we set the variable.
   chan *cchan_out = chan_init(ctx->cchan_fqcn_out, CHAN_TYPE_PUSH_CONNECT);
  chan *cchan_in  = chan_init(ctx->conf->cchan_fqcn, CHAN_TYPE_PULL_BIND);

  chan_bind(cchan_out);
  chan_bind(cchan_in);

  while(1) {


  }
  /*  WineingCtrlProto::Request req;
  WineingCtrlProto::Response res;
  std::stringstream tape;

  bool running = true;
  while (running) {

    // Clear all objects
    req.Clear();
    res.Clear();
    tape.clear();

    int rc = chan_recv_ctrl(ctx->cchan, req);
    log(LOG_DEBUG,                                          \
        "Received control request [id: %li, type: %d]",     \
        req.requestid(),                                    \
        req.type());

    if(rc == 1) {
      std::stringstream err;

      res.set_requestid(req.requestid());
      res.set_code(WineingCtrlProto::Response::OK);

      // Process control message
      switch(req.type())
        {
        case WineingCtrlProto::Request::START:
          // Error handling (such as checking if tape file exists) is
          // done here because it's less work to do so than sending
          // messages between two threads.

          // Check that the market data thread is not already
          // started. If so send an error back to the client.
          if(msg.ctrl == WINEING_MSG_MARKET_RUN) {
            err << "Already streaming market data! Send a stop first.";
            res.set_code(WineingCtrlProto::Response::ERR);
            res.set_text(err.str());
            break;
          }

          msg.ctrl = WINEING_MSG_MARKET_RUN;
          msg.data = NULL;

          // If the tape file was not provided NxCore will start
          // streaming real-time data. Make sure NxCoreAccess is
          // running and connected to the NxCore servers. See
          // http://nxcoreapi.com/doc/concept_Introduction.html.
          if(req.has_tape_file()) {
            tape << ctx->conf->tape_basedir \
                 << req.tape_file();

            // This checks whether a file exists or not the windows
            // way. Remember we are loading the file with NxCore which
            // is, well, Windows.
            DWORD attrs = GetFileAttributes(tape.str().c_str());
            if((attrs == INVALID_FILE_ATTRIBUTES)       \
               && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
              err << "File '" << tape.str() << "' not found.";
              res.set_code(WineingCtrlProto::Response::ERR);
              res.set_text(err.str());
              log(LOG_DEBUG, err.str().c_str());
              break;
            }

            msg.data = (void*)tape.str().c_str();
          }

          pthread_mutex_lock( &sync_mutex );
          pthread_cond_signal( &cond_var );
          pthread_mutex_unlock( &sync_mutex );
          break;

        case WineingCtrlProto::Request::STOP:
          msg.ctrl = WINEING_MSG_MARKET_STOP;
          break;

        case WineingCtrlProto::Request::SHUTDOWN:
          msg.ctrl = WINEING_MSG_MARKET_SHUTDOWN;
          running = false;
          break;
        }

     chan_send_ctrl(ctx->cchan, res);

    }
  }
*/
  chan_destroy(cchan_in);
  chan_destroy(cchan_out);

  return NULL;
}

// void sigproc(int ptr)
// {
//   wineing_shutdown(ctx);
// }
