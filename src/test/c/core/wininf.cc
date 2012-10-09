
/*
 * Stub
 */ 

#include "core/wininf.h"

int wininf_nxcore_load()
{
  return 0;
}

int wininf_nxcore_run(char *tape,
                      int STDCALL (*fn) (const NxCoreSystem *,
                                         const NxCoreMessage *))
{
  return 0;
}

void wininf_nxcore_free()
{
  
}

int wininf_file_exists(const char *str)
{
  return 0;
}
