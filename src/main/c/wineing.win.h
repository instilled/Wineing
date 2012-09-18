
#ifndef __WINEING_H
#define __WINEING_H

#include "core/wchan.h"
#include "core/inxcore.win.h"


#include "logging/logging.h"

// Application defaults
#define DEFAULTS_SCHAN_NAME           "tcp://*:9990"
#define DEFAULTS_CCHAN_NAME           "tcp://*:9991"
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
  const char *cchan_fqcn;
  const char *mchan_fqcn;
  const char *schan_fqcn;
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
  wchan *cchan;
  wchan *mchan;
  WineingConf *conf;
} WineingCtx;

//int __stdcall nxCoreCallback(const NxCoreSystem*, const NxCoreMessage*);

/**
 *
 */
static void wineing_init(WineingCtx &);

/**
 *
 */
static int wineing_run(WineingCtx &);

/**
 *
 */
static void wineing_shutdown(WineingCtx &);

/**
 *
 */
static void* ctrl_thread(void*);

/**
 *
 */
static void* market_thread(void*);

void cmd_parse(int, char**, WineingConf &);

void sigproc(int);

#endif /* __WINEING_H */
