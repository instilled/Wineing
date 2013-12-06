

#include "nx/nxinf.h"

#include "net/chan.h"

/**
 * The thread invoking nxtape_init should own the chan instances,
 * cchan_out, and mchan.
 *
 * \param [in] cchan_out Not thread safe! Channel to send control
 *                       messages to the client
 * \param [in] mchan     Not thread safe! Channel to send market data
 *                       messages to the client
 */
void nxtape_init(chan *cchan_out, chan *mchan);

int STDCALL nxtape_process(const NxCoreSystem *pNxCoreSys,
                           const NxCoreMessage *pNxCoreMsg);
