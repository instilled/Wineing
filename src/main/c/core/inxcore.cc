

#include "inxcore.h"
#include "logging/logging.h"

#include <stdio.h>

HINSTANCE inxcore_load()
{
  log(LOG_DEBUG, "Loading NxCoreAPI64.dll");
  HINSTANCE hLib = ::LoadLibrary("lib/NxCoreAPI64.dll");

  if(!hLib) {
    log(LOG_ERROR, \
        "Failed loading NxCoreAPI64.dll. Reason %d", \
        GetLastError());
    return NULL;
  }

  return hLib;
}

void inxcore_run(HINSTANCE hLib,               \
                 inxcore_processTape callback, \
                 char *tape,
                 void *usrdata)
{
  NxCoreProcessTape pTapeProc =                                         \
    (NxCoreProcessTape) ::GetProcAddress(hLib,"sNxCoreProcessTape");
  if(!pTapeProc) {
    log(LOG_ERROR, "Failed loading NxCoreProcessTape function");
    return;
  }

  pTapeProc(tape, 0, 0, 0, callback);
  /*
    pfNxProcessTape("C:\\md\\201005\\20100506.DQ.nxc",  \
    0,                                                  \
    NxCF_EXCLUDE_CRC_CHECK,                             \
    123,                                                \
    nxCoreCallback);
  */
}

void inxcore_free(HINSTANCE nxCoreLib)
{
  ::FreeLibrary(nxCoreLib);
}

long msgCount = 0;

int __stdcall nxCoreCallback(const NxCoreSystem* pNxCoreSys, const NxCoreMessage* pNxCoreMsg)
{
  msgCount++;
  switch( pNxCoreMsg->MessageType ) {
  case NxMSG_STATUS:
    printf("msg %8ld: STATUS\n", msgCount);
    break;
  case NxMSG_EXGQUOTE:
    printf("msg %8ld: EXGQUOTE\n", msgCount);
    break;
  case NxMSG_MMQUOTE:
    printf("msg %8ld: MMQUOTE\n", msgCount);
    break;
  case NxMSG_TRADE:
    printf("msg %8ld: TRADE\n", msgCount);
    break;
  case NxMSG_CATEGORY:
    printf("msg %8ld: CATEGORY\n", msgCount);
    break;
  case NxMSG_SYMBOLCHANGE:
    printf("msg %8ld: SYMBOLCHANGE\n", msgCount);
    break;
  case NxMSG_SYMBOLSPIN:
    printf("msg %8ld: SYMBOLSPIN\n", msgCount);
    break;
  }
  return NxCALLBACKRETURN_CONTINUE;
}
