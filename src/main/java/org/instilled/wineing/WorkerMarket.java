package org.instilled.wineing;

import org.instilled.wineing.core.Worker;
import org.instilled.wineing.core.ZMQChannel;
import org.instilled.wineing.core.ZMQChannel.ZMQChannelType;
import org.instilled.wineing.gen.WineingMarketDataProto.MarketData;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.zeromq.ZMQException;

import com.google.protobuf.InvalidProtocolBufferException;

public class WorkerMarket implements Worker
{
    public static final Logger log = LoggerFactory
            .getLogger(WorkerMarket.class);

    private String _mchan;

    volatile boolean _running;

    public WorkerMarket(String mchan)
    {
        _mchan = mchan;
    }

    public void shutdown()
    {
        _running = false;
    }

    @Override
    public void run()
    {
        _running = true;

        ZMQChannel market = new ZMQChannel(_mchan, ZMQChannelType.SUB);
        market.bind();

        long count = 0;
        while (_running)
        {

            try
            {
                byte[] buf = market.receive();

                MarketData marketData = MarketData.parseFrom(buf);

                if (count % 1000 == 0)
                {
                    log.debug(String
                            .format("Message received (printing every 1000) [%s]",
                                    marketData.getType()));
                }

                count++;
            } catch (InvalidProtocolBufferException e)
            {
                log.error("Failed to process MarketData message.", e);
            } catch (ZMQException e)
            {
                // Ignore. We expect an exception when shutting down
            }
        }
        log.debug("Received " + count + " messages");

        market.close();
    }
}
