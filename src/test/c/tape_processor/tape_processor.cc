
/*
 * An 100% linux implementation of the nxcore callback. Only used for testing.
 *
 * Windows version: <root>/src/main/c/tape_processor/tape_processor.win.cc
 *
 */

#include "tape_processor/tape_processor.h"

/**
 * Does nothing
 */
int STDCALL my_processTapeFn(const NxCoreSystem *pNxCoreSys,
                             const NxCoreMessage *pNxCoreMsg)
{
  return 0;
}

