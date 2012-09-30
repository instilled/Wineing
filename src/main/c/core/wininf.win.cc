
#include "wininf.win.h"

#include <windows.h>
#include <stdio.h>

#include "NxCoreAPI.h"

#include "logging/logging.h"

void * wininf_nxcore_load()
{
  log(LOG_DEBUG, "Loading NxCoreAPI64.dll");
  HINSTANCE hLib = ::LoadLibrary("lib/NxCoreAPI64.dll");

  if(!hLib) {
    log(LOG_ERROR,
        "Failed loading NxCoreAPI64.dll. Reason %d",
        GetLastError());
    return NULL;
  }

  return (void*)hLib;
}

void wininf_nxcore_run(void *hLib,
                 wininf_nxcore_callback callback,
                 char *tape,
                 void *usrdata)
{
  NxCoreProcessTape pTapeProc =
    (NxCoreProcessTape) ::GetProcAddress((HINSTANCE)hLib,"sNxCoreProcessTape");
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

void wininf_nxcore_free(void *nxCoreLib)
{
  ::FreeLibrary((HINSTANCE)nxCoreLib);
}

int wininf_file_exists(const char *str)
{
  DWORD attrs = GetFileAttributes(str);
  if((attrs == INVALID_FILE_ATTRIBUTES)
     && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
    return -1;
  }
  return 0;
}
