#ifndef _WININF_H
#define _WININF_H

#include "NxCoreAPI.h"
#include "chan.h"

/*
  Dependencies to NxCore and other windows stuff has been removed from
  the header because we would like to test wineing.cc without the
  depending on windows related stuff so we can test and debug as much
  of the code as possible. By separating by windows and linux we can
  achieve much of it.

  The consequence is that we will need to implement the callback and
  its behaviour in wininf.win.cc.
 */

typedef int __stdcall (*nxcore_callback)(const NxCoreSystem *psys,
                                         const NxCoreMessage *pmsg);

int wininf_nxcore_load();

int  wininf_nxcore_run(char *tape, nxcore_callback callback);

void  wininf_nxcore_free();

int wininf_file_exists(const char *path);

#endif /* _INXCORE_H */
