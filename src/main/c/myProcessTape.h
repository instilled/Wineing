
#include <windows.h>

#include "core/chan.h"
#include "NxCoreAPI.h"
#include "WineingMarketDataProto.pb.h"
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

static chan g_cchan_out;
static chan g_mchan;

void myProcessTape_init(chan *cchan_out, chan *mchan)
{
  g_cchan_out = *cchan_out;
  g_mchan = *mchan;
}

/**
 * Prcesses each market data update from NxCore sends it through a ZMQ
 * channel to the client.
 */
int __stdcall myProcessTape(const NxCoreSystem *pNxCoreSys,
                            const NxCoreMessage *pNxCoreMsg)
{
  // using namespace WineingMarketDataProto;

  // static MarketData m;
  // static int t_version = DEFAULTS_SHARED_VERSION_INIT;
  // static w_ctrl t_data = {
  //   WINEING_CTRL_CMD_INIT,
  //   new char[WINEING_CTRL_DEFAULT_DATA_SIZE],
  //   0
  // };

  // t_version = lazy_update_local_if_changed(t_version,
  //                                          &t_data,
  //                                          &g_data,
  //                                          g_to_t);

  // // Because we reuse protobuf objects we have to clear them
  // m.Clear();

  // TODO: we need to pre-allocate a batch of buffers (ring buffer?)
  // this is required because we don't know when zmq has released an
  // underlying buffer. we could use *hint as a callback but then
  // again would require a thread safe ring-buffer or whatever data
  // structre we use.

  /*
  switch( pNxCoreMsg->MessageType )
    {
    case NxMSG_STATUS:
      m.set_type(MarketData::STATUS);
      chan_send_market(ctx.mchan, m);
      break;

    case NxMSG_EXGQUOTE:
      m.set_type(MarketData::QUOTE_EX);
      chan_send_market(ctx.mchan, m);
      break;

    case NxMSG_MMQUOTE:
      m.set_type(MarketData::QUOTE_EX);
      chan_send_market(ctx.mchan, m);
      break;

    case NxMSG_TRADE:
      m.set_type(MarketData::QUOTE_EX);
      chan_send_market(ctx.mchan, m);
      break;

    case NxMSG_CATEGORY:
      m.set_type(MarketData::QUOTE_EX);
      chan_send_market(ctx.mchan, m);
      break;
      //case NxMSG_SYMBOLCHANGE:
      //break;
      //case NxMSG_SYMBOLSPIN:
      //break;
    }
*/

  return NxCALLBACKRETURN_CONTINUE;
}


