
/*
 * An 100% linux implementation of the nxcore callback. Only used for testing.
 *
 * Windows version: <root>/src/main/c/tape_processor/tape_processor.win.cc
 *
 */

#include "nx/nxtape.h"

#include "net/chan.h"

int STDCALL nxtape_process(const NxCoreSystem *pNxCoreSys,
                           const NxCoreMessage *pNxCoreMsg)
{
  // do nothing
  return 0;
}

void nxtape_init(chan *cchan_out, chan *mchan)
{
  // do nothing
}
