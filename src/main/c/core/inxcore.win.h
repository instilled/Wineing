#ifndef _INXCORE_H
#define _INXCORE_H

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

#endif /* _INXCORE_H */
