#ifndef __INXCORE_H
#define __INXCORE_H

#include <windows.h> //rename("timeval", "wtimeval") // this doesn't work

/* we could use nxcoreapi_class. in fact that would simplify
   interacting with nxcore greatly at the cost of having c-string
   conversion warnings. i prefer a clean compile over comfortness. */
//#include "nxcoreapi_class.h"
#include "NxCoreAPI.h"

/**
 * The callback invoked on each market data message received by
 * NxCore.
 */
typedef int __stdcall (*inxcore_processTape)(const NxCoreSystem* pNxCoreSys, \
                                             const NxCoreMessage* pNxCoreMsg);

HINSTANCE inxcore_load();

void inxcore_run(HINSTANCE hLib,                                \
                 inxcore_processTape callback,                  \
                 char *tape,                                    \
                 void *usrdata);

void inxcore_free(HINSTANCE hLib);

/* int __stdcall nxCoreCallback(const NxCoreSystem* pNxCoreSys, const NxCoreMessage* pNxCoreMsg) */
/* { */
/*   msgCount++; */
/*   switch( pNxCoreMsg->MessageType ) { */
/*   case NxMSG_STATUS: */
/*     printf("msg %8ld: STATUS\n", msgCount); */
/*     break; */
/*   case NxMSG_EXGQUOTE: */
/*     printf("msg %8ld: EXGQUOTE\n", msgCount); */
/*     break; */
/*   case NxMSG_MMQUOTE: */
/*     printf("msg %8ld: MMQUOTE\n", msgCount); */
/*     break; */
/*   case NxMSG_TRADE: */
/*     printf("msg %8ld: TRADE\n", msgCount); */
/*     break; */
/*   case NxMSG_CATEGORY: */
/*     printf("msg %8ld: CATEGORY\n", msgCount); */
/*     break; */
/*   case NxMSG_SYMBOLCHANGE: */
/*     printf("msg %8ld: SYMBOLCHANGE\n", msgCount); */
/*     break; */
/*   case NxMSG_SYMBOLSPIN: */
/*     printf("msg %8ld: SYMBOLSPIN\n", msgCount); */
/*     break; */
/*   } */
/*   return NxCALLBACKRETURN_CONTINUE; */
/* } */

#endif /* __INXCORE_H */
