
#ifndef _WINEING_H
#define _WINEING_H

#include <string.h>
#include "logging/logging.h"


// Application defaults
#define DEFAULTS_CCHAN_IN_NAME            "tcp://*:9990"
#define DEFAULTS_CCHAN_OUT_NAME           "tcp://*:9991"
#define DEFAULTS_MCHAN_NAME               "tcp://*:9992"
#define DEFAULTS_ICHAN_NAME               "inproc://ctrl.out"
#define DEFAULTS_TAPE_BASE_DIR            "C:\\md\\"
#define DEFAULTS_SHARED_VERSION_INIT      0
#define DEFAULTS_SHARED_VERSION_READ_INIT -1
#define DEFAULTS_CCHAN_BUFFER_SIZE        2048

// Values for w_ctrl.cmd
#define WINEING_CTRL_CMD_INIT             3
#define WINEING_CTRL_CMD_MARKET_RUN       2
#define WINEING_CTRL_CMD_MARKET_STOP      1
#define WINEING_CTRL_CMD_SHUTDOWN         0
#define WINEING_CTRL_DEFAULT_DATA_SIZE    1024

// The channel response/notification messages
// are sent to cchan_out_thread

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
  void* nxCoreLib;
  w_conf *conf;
} w_ctx;

// http://www.drdobbs.com/parallel/volatile-vs-volatile/212701484
// see http://stackoverflow.com/questions/2044565/volatile-struct-semantics
typedef struct
{
  int volatile cmd;       // the command
  char * volatile data;    // data buffer
  size_t volatile size;   // data buffer's size
} w_ctrl;

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


inline void _copy_local_to_shared (const void *t, void *g)
{
  const w_ctrl *local = (const w_ctrl*)t;
  w_ctrl *shared = (w_ctrl*)g;

  shared->cmd  = local->cmd;
  shared->size = local->size;
  if(0 < local->size) {
    memcpy(shared->data, local->data, local->size);
  }
}

inline void _copy_shared_to_local(void *t, const void *g)
{
  w_ctrl *local = (w_ctrl*)t;
  const w_ctrl *shared = (const w_ctrl*)g;

  local->cmd  = shared->cmd;
  local->size = shared->size;
  if(0 < shared->size) {
    memcpy(local->data, shared->data, shared->size);
  }
}

#endif /* _WINEING_H */
