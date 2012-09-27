
#ifndef _WINEING_H
#define _WINEING_H

#include "core/inxcore.h"
#include "logging/logging.h"


// Application defaults
#define DEFAULTS_CCHAN_IN_NAME            "tcp://*:9990"
#define DEFAULTS_CCHAN_OUT_NAME           "tcp://*:9991"
#define DEFAULTS_MCHAN_NAME               "tcp://*:9992"
#define DEFAULTS_TAPE_BASE_DIR            "C:\\md\\"
#define DEFAULTS_SHARED_DATA_SIZE        1024
#define DEFAULTS_SHARED_VERSION          -1
#define DEFAULTS_CCHAN_BUFFER_SIZE       2048

// Messaging
#define WINEING_SHARED_CMD_INIT          0
#define WINEING_SHARED_CMD_SHUTDOWN      1
#define WINEING_SHARED_CMD_MARKET_STOP   2
#define WINEING_SHARED_CMD_MARKET_RUN    3
#define WINEING_INPROC_CCHAN_OUT          "inproc://ctrl.out"

/**
 * \struct
 *
 * The configuration as modified by command line arguments passed to
 * program invocation.
 */
typedef struct
{
  const char *cchan_in_fqcn;
  const char *cchan_out_fqcn;
  const char *mchan_fqcn;
  const char *tape_basedir;
} w_conf;

/**
 * \struct
 *
 * Wineing context.
 */
typedef struct
{
  HINSTANCE nxCoreLib;
  w_conf *conf;
} w_ctx;


/**
 * Initializes Wineing.
 */
void wineing_init(w_ctx &);

/**
 * Runs Wineing by creating the three threads (see functions below)
 * - cchan_in_thread
 * - cchan_out_thread
 * - market_thread
 */
void wineing_run(w_ctx &);

/**
 * Frees any resources allocated by Wineing and does a clean shutdown.
 */
void wineing_shutdown(w_ctx &);

/**
 * Thread listening for incoming control requests. Responses to
 * control Requests are never sent directly to the client but instead
 * send to *cchan_out_thread*.
 */
void* cchan_in_thread(void*);

/**
 * Thread sending Responses to the client(s).
 */
void* cchan_out_thread(void*);

/**
 * Thread sending market data to clients. Any non market data related
 * messages, e.g. errors, are sent to *cchan_out_thread*.
 */
void* market_thread(void*);

#endif /* _WINEING_H */
