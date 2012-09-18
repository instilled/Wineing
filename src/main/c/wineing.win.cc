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
 *   > http://stackoverflow.com/questions/375913/what-can-i-use-to-profile-c-code-in-linux
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

  I have yet to see if using these extensions/functions we achieve
  faster results that using cache 
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
#include <atomic>

#include "wineing.win.h"

// Google protobuf generated headers
#include "WineingCtrlProto.pb.h"
#include "WineingMarketDataProto.pb.h"

// Unsynchronized, potential threading issues with
// WineingMsg and WineingCtx

// Wineing uses these to communicate between threads.
static pthread_mutex_t sync_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond_var   = PTHREAD_COND_INITIALIZER;
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
static WineingCtx ctx;

int main(int argc, char** argv)
{
  // Interrupt ctrl-c and kill and do proper shutdown
  // http://www.cs.cf.ac.uk/Dave/C/node24.html
  //signal(SIGINT, sigproc);
  //signal(SIGQUIT, sigproc);

  WineingConf conf;

  conf.cchan_fqcn   = DEFAULTS_CCHAN_NAME;
  conf.mchan_fqcn   = DEFAULTS_MCHAN_NAME;
  conf.schan_fqcn   = DEFAULTS_SCHAN_NAME;
  conf.tape_basedir = DEFAULTS_TAPE_BASE_DIR;

  cmd_parse(argc, argv, conf);

  log(LOG_INFO, "Starting Wineing...");

  // Be nice and let Linux users know that we are running winelib!
  // HAHAHAHA!
  //MessageBox(NULL, "I'm Wineing!", "", MB_OK);

  ctx.conf = &conf;

  wineing_init(ctx);
  int rc = wineing_run(ctx);
  wineing_shutdown(ctx);

  return rc;
}

/**
 * Initiales Wineing. Includes (in that order)
 * - verifying Google Protocol generated header versions
 * - loading nxcore dll
 * - initializing ZMQ channels (cchan, wchan)
 *
 */
void wineing_init(WineingCtx &ctx)
{
  // Verify the version of the library we linked against is compatible
  // with the version of the headers generated.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Load dll.
  ctx.nxCoreLib = inxcore_load();
  if(!ctx.nxCoreLib) {
    log(LOG_ERROR, "Failed loading NxCore dll.");
    exit(1);
  }

  // Initialize the control channel. Only if the client requests
  // market data (by sending START over control channel) we will
  // create the market data channel.
  ctx.cchan = wchan_init(ctx.conf->cchan_fqcn, WCHAN_TYPE_CTRL);
  ctx.mchan = wchan_init(ctx.conf->mchan_fqcn, WCHAN_TYPE_MARKET);

  // Bind the channel to its endpoint.
  wchan_start(ctx.cchan);
  wchan_start(ctx.mchan);
}

/**
 *
 */
int wineing_run(WineingCtx &ctx)
{
  // Good tutorial on posix threads
  // http://www.yolinux.com/TUTORIALS/LinuxTutorialPosixThreads.html
  pthread_t ctrl_t;
  pthread_t market_t;

  // The control thread listens for incomming messages on cchan
  // managing the mchan (market data channel) as requested by the
  // client.
  pthread_create(&ctrl_t, NULL, ctrl_thread, (void*)&ctx);
  pthread_create(&market_t, NULL, market_thread, (void*)&ctx);

  // Wait for threads to finish
  pthread_join(ctrl_t, NULL);
  pthread_join(market_t, NULL);

  return 0;
}

/**
 * Frees allocated resources.
 */
void wineing_shutdown(WineingCtx &ctx)
{
  log(LOG_INFO, "Shutting down...");

  inxcore_free(ctx.nxCoreLib);

  wchan_free(ctx.mchan);
  wchan_free(ctx.cchan);

  // This frees any protobuf specific stuff.
  google::protobuf::ShutdownProtobufLibrary();
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
  // reallocates a new wchan structure during its lifetime.
  static MarketData m;

  // Because we reuse protobuf objects we have to clear them.
  m.Clear();

  switch( pNxCoreMsg->MessageType )
    {
    case NxMSG_STATUS:
      m.set_type(MarketData::STATUS);
      wchan_send_market(ctx.mchan, m);
      break;

    case NxMSG_EXGQUOTE:
      m.set_type(MarketData::QUOTE_EX);
      wchan_send_market(ctx.mchan, m);
      break;

    case NxMSG_MMQUOTE:
      m.set_type(MarketData::QUOTE_EX);
      wchan_send_market(ctx.mchan, m);
      break;

    case NxMSG_TRADE:
      m.set_type(MarketData::QUOTE_EX);
      wchan_send_market(ctx.mchan, m);
      break;

    case NxMSG_CATEGORY:
      m.set_type(MarketData::QUOTE_EX);
      wchan_send_market(ctx.mchan, m);
      break;
      //case NxMSG_SYMBOLCHANGE:
      //break;
      //case NxMSG_SYMBOLSPIN:
      //break;
    }


  // An unsynchronized read on a shared variable. We don't care about
  // the value being delayed. The cost of synchronization is too high.
  return msg.ctrl == WINEING_MSG_MARKET_RUN ? \
    NxCALLBACKRETURN_CONTINUE :               \
    NxCALLBACKRETURN_STOP;
}

/**
 * The thread processing NxCore messages.
 */
void* market_thread(void *ctx)
{
  WineingCtx *_ctx = (WineingCtx *)ctx;

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

  return NULL;
}

/**
 * The controlling thread. It waits for the client to send control
 * messages to Wineing.
 */
void* ctrl_thread(void *_ctx)
{
  WineingCtx *ctx = (WineingCtx *)_ctx;

  WineingCtrlProto::Request req;
  WineingCtrlProto::Response res;
  std::stringstream tape;

  bool running = true;
  while (running) {

    // Clear all objects
    req.Clear();
    res.Clear();
    tape.clear();

    int rc = wchan_recv_ctrl(ctx->cchan, req);
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

     wchan_send_ctrl(ctx->cchan, res);

    }
  }

  return NULL;
}

// void sigproc(int ptr)
// {
//   wineing_shutdown(ctx);
// }

void cmd_print_usage()
{
  printf("Usage: wineing.exe "
         "--cchan=<fqcn> "
         "--mchan=<fqcn> "
         "[--schan=<fqcn>] "
         "[--tape-root=<dir>]\n\n");

  printf("Wineing TBD.\n\n");
  printf("ZMQ channels:\n");
  printf("  --cchan          Control channel (synchronous ZMQ REQ/REP socket)\n");
  printf("  --mchan          Market data channel (asynchronous ZMQ PUB/SUB\n");
  printf("                   socket)\n");
  printf("  [--schan]        Optional. Sync channel (synchronous ZMQ REQ/REP "
                                                                    "socket)\n");
  printf("                   Defaults to 'tcp://localhost:9991'\n\n");
  printf("NxCore related options:\n");
  printf("  [--tape-root]    The directory from which to serve the tape files\n");
  printf("                   Defaults to 'C:\\md\\'. The path has to end "
                             "in '\\'\n");
}

char * cmd_parse_opt(char *argv)
{
  int eq = strcspn (argv, "=");
  return &argv[eq+1];
}

void cmd_parse(int argc, char** argv, WineingConf &conf)
{
  int allOpts = 0;
  for(int i = 1; i < argc; i++) {
    switch(argv[i][2])
      {
      case 'c':
        conf.cchan_fqcn = cmd_parse_opt(argv[i]);
        allOpts |= 0x1;
        break;

      case 'm':
        conf.mchan_fqcn = cmd_parse_opt(argv[i]);
        allOpts |= 0x2;
        break;

      case 't':
        conf.tape_basedir = cmd_parse_opt(argv[i]);
        break;

      case 's':
        conf.schan_fqcn = cmd_parse_opt(argv[i]);
        break;
      }
  }

  if(allOpts != 3) {
    cmd_print_usage();
    exit(1);
  }
}
