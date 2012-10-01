
#include "wininf.win.h"

#include <windows.h>
#include <stdio.h>

#include "NxCoreAPI.h"
#include "logging/logging.h"
#include "myProcessTape.h"

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

int wininf_nxcore_run(chan *cchan_out, chan *mchan, char *tape)
{
  NxCoreProcessTape pTapeProc =
    (NxCoreProcessTape) ::GetProcAddress(g_hlib, "sNxCoreProcessTape");
  if(!pTapeProc) {
    log(LOG_ERROR, "Failed loading NxCoreProcessTape function");
    return -1;
  }

  myProcessTape_init(cchan_out, mchan);
  pTapeProc(tape, 0, 0, 0, myProcessTape);

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
