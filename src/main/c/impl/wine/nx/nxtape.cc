
// Include windows.h first because it will cause redefinition errors
// for 'struct timeval'. There's a work around to this: unbind timeval
// after including posix headers. We leave it that way for the sake of
// simplicity.
#include <windows.h>

#include "conc/conc.h"
#include "core/wineing.h"
#include "net/chan.h"
#include "nx/nxtape.h"
#include "nx/nxinf.h"
#include "gen/WineingCtrlProto.pb.h"
#include "gen/WineingMarketDataProto.pb.h"

#include "NxCoreAPI.h"

#include <unistd.h>
#include <pthread.h>
#include <sstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

/**
 * Prcesses each market data update from NxCore sends it through a ZMQ
 * channel to the client. The
 */
int STDCALL my_processTapeFn(const NxCoreSystem *pNxCoreSys,
                             const NxCoreMessage *pNxCoreMsg)
{
  using namespace WineingMarketDataProto;

  char *buffer;
  static MarketData m;
  static int t_version = DEFAULTS_SHARED_VERSION_READ_INIT;
  static w_ctrl t_data = {
    WINEING_CTRL_CMD_INIT,
    new char[WINEING_CTRL_DEFAULT_DATA_SIZE],
    0
  };

  t_version = lazy_update_local_if_changed(t_version,
                                           &t_data,
                                           &g_data,
                                           _copy_shared_to_local);

  // Because we reuse protobuf objects we to clear them
  m.Clear();

  // TODO: we need to pre-allocate a batch of buffers (ring buffer?)
  // this is required because we don't know when zmq has released an
  // underlying buffer. we could use *hint as a callback but then
  // again would require a thread safe ring-buffer or whatever data
  // structre we use.

  switch( pNxCoreMsg->MessageType )
    {
    case NxMSG_STATUS:
      m.set_type(MarketData::STATUS);

      int buf_size = m.ByteSize();
      buffer = new char[buf_size];
      google::protobuf::io::ArrayOutputStream os (buffer, buf_size);
      m.SerializeToZeroCopyStream(&os);
      chan_send(g_market_thread_mchan, buffer, m.ByteSize());
      break;

    // case NxMSG_EXGQUOTE:
    //   m.set_type(MarketData::QUOTE_EX);
    //   chan_send(g_mchan, buffer, m.ByteSize(), NULL);
    //   break;

    // case NxMSG_MMQUOTE:
    //   m.set_type(MarketData::QUOTE_EX);
    //   chan_send(g_mchan, buffer, m.ByteSize(), NULL);
    //   break;

    // case NxMSG_TRADE:
    //   m.set_type(MarketData::QUOTE_EX);
    //   chan_send(g_mchan, buffer, m.ByteSize(), NULL);
    //   break;

    // case NxMSG_CATEGORY:
    //   m.set_type(MarketData::QUOTE_EX);
    //   chan_send(g_mchan, buffer, m.ByteSize(), NULL);
    //   break;
      //case NxMSG_SYMBOLCHANGE:
      //break;
      //case NxMSG_SYMBOLSPIN:
      //break;
    }

  return t_data.cmd < WINEING_CTRL_CMD_MARKET_RUN ?
    NxCALLBACKRETURN_STOP : NxCALLBACKRETURN_CONTINUE;
}
