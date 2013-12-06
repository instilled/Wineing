package org.instilled.wineing;

import java.io.IOException;

import org.instilled.wineing.core.Worker;
import org.instilled.wineing.core.ZMQChannel;
import org.instilled.wineing.core.ZMQChannel.ZMQChannelType;
import org.instilled.wineing.gen.WineingMarketDataProto.MarketData;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.zeromq.ZMQException;

import com.google.protobuf.CodedInputStream;

public class WorkerMarket implements Worker
{
    public static final Logger log = LoggerFactory
            .getLogger(WorkerMarket.class);

    private String _mchan;

    volatile boolean _running;

    private ZMQChannel _market;

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
        byte[] buffer = new byte[1024];

        _running = true;

        _market = new ZMQChannel(_mchan, ZMQChannelType.SUB);
        _market.bind();

        long count = 0;
        while (_running)
        {

            try
            {
                int read = _market.receive(buffer, 0, buffer.length);
                CodedInputStream is = CodedInputStream.newInstance(
                        buffer, 0, read);
                MarketData marketData = MarketData.parseFrom(is);

                if (count % 1000 == 0)
                {
                    log.debug(String
                            .format("Message received (printing every 1000) [%s]",
                                    marketData.getType()));
                }

                count++;
            } catch (IOException e)
            {
                log.error("Failed to process MarketData message.", e);
            } catch (ZMQException e)
            {
                // Ignore. We expect an exception when shutting down
            }
        }
        log.debug("Received " + count + " messages");

        _market.close();
    }
}
