#ifndef _WININF_H
#define _WININF_H

struct NxCoreSystem;
struct NxCoreMessage;

#ifdef __WINE__
   #define STDCALL __stdcall
#else
   #define STDCALL
#endif

/*
  Dependencies to NxCore and other windows stuff has been removed from
  the header because we would like to test wineing.cc without the
  depending on windows related stuff so we can test and debug as much
  of the code as possible. By separating by windows and linux we can
  achieve much of it.

  The consequence is that we will need to implement the callback and
  its behaviour in wininf.win.cc.
 */

int  wininf_nxcore_load();

int  wininf_nxcore_run(char *tape,
                      int STDCALL (*fn) (const NxCoreSystem *,
                                         const NxCoreMessage *));

void wininf_nxcore_free();

int  wininf_file_exists(const char *path);

#endif /* _INXCORE_H */
