
#include "wininf.h"

#include <windows.h>
#include <stdio.h>

#include "NxCoreAPI.h"
#include "logging/logging.h"

static HINSTANCE g_hlib;

int wininf_nxcore_load()
{
  log(LOG_DEBUG, "Loading NxCoreAPI64.dll");
  g_hlib = ::LoadLibrary("lib/NxCoreAPI64.dll");

  if(!g_hlib) {
    log(LOG_ERROR,
        "Failed loading NxCoreAPI64.dll. Reason %d",
        GetLastError());
    return -1;
  }

  return 0;
}

int wininf_nxcore_run(char *tape,
                      int __stdcall (*fn) (const NxCoreSystem *,
                                           const NxCoreMessage *))
{
  NxCoreProcessTape processTapeFn =
    (NxCoreProcessTape) ::GetProcAddress(g_hlib, "sNxCoreProcessTape");
  if(!processTapeFn) {
    log(LOG_ERROR, "Failed loading NxCoreProcessTape function");
    return -1;
  }

  processTapeFn(tape, 0, 0, 0, fn);

  return 0;
}

void wininf_nxcore_free()
{
  ::FreeLibrary(g_hlib);
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
