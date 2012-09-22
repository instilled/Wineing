
#ifndef __WINEING_H
#define __WINEING_H

#include "core/inxcore.h"
#include "logging/logging.h"


// Application defaults
#define DEFAULTS_SCHAN_NAME           "tcp://*:9999"
#define DEFAULTS_CCHAN_NAME           "tcp://*:9990"
#define DEFAULTS_MCHAN_NAME           "tcp://*:9992"
#define DEFAULTS_TAPE_BASE_DIR        "C:\\md\\"


// Messaging
#define WINEING_MSG_MARKET_SHUTDOWN   0
#define WINEING_MSG_MARKET_STOP       1
#define WINEING_MSG_MARKET_RUN        2

/**
 * \struct
 *
 * TBD
 */
typedef struct
{
  int ctrl;    // One of WINEING_MSG_*
  void *data;  // Anything
} WineingMsg;

/**
 * \struct
 *
 * TBD
 */
typedef struct
{
  const char *schan_fqcn;
  const char *cchan_fqcn;
  const char *mchan_fqcn;
  const char *tape_basedir;
} WineingConf;

/**
 * \struct
 *
 * TBD
 */
typedef struct
{
  HINSTANCE nxCoreLib;
  const char *cchan_fqcn_out;
  WineingConf *conf;
} WineingCtx;


/**
 *
 */
void wineing_init(WineingCtx &);

/**
 *
 */
void wineing_run(WineingCtx &);

/**
 *
 */
void wineing_shutdown(WineingCtx &);

/**
 *
 */
int wineing_wait_for_client(WineingCtx &);

/**
 *
 */
void* ctrl_thread(void*);

/**
 *
 */
void* market_thread(void*);


//void sigproc(int);

#endif /* __WINEING_H */
