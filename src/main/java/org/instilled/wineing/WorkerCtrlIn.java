package org.instilled.wineing;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import org.instilled.wineing.core.ResponseProcessor;
import org.instilled.wineing.core.Worker;
import org.instilled.wineing.core.ZMQChannel;
import org.instilled.wineing.core.ZMQChannel.ZMQChannelType;
import org.instilled.wineing.gen.WineingCtrlProto.Response;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.zeromq.ZMQException;

import com.google.protobuf.InvalidProtocolBufferException;

public class WorkerCtrlIn implements Worker
{
    public static final Logger log = LoggerFactory
            .getLogger(WorkerCtrlIn.class);

    private String _cchanIn;
    private ResponseProcessor _defaultResponseProcessor;
    private Map<Long, ResponseProcessor> _responseProcessors;

    private volatile boolean _running;

    public WorkerCtrlIn(String cchanIn)
    {
        _cchanIn = cchanIn;
        _responseProcessors = Collections
                .synchronizedMap(new HashMap<Long, ResponseProcessor>());
        setDefaultResponseProcessor(null);
    }

    public void shutdown()
    {
        _running = false;
    }

    public void setDefaultResponseProcessor(ResponseProcessor p)
    {
        _defaultResponseProcessor = p;
    }

    public void setResponseProcessor(long requestId, ResponseProcessor p)
    {
        _responseProcessors.put(requestId, p);
    }

    @Override
    public void run()
    {
        _running = true;

        // Start incoming channel. Listens to replies from
        // Wineing
        ZMQChannel cchan_in = new ZMQChannel(_cchanIn,
                ZMQChannelType.PULL_CONNECT);
        cchan_in.bind();

        while (_running)
        {
            Response res;
            try
            {
                byte[] buffer = cchan_in.receive();
                res = Response.parseFrom(buffer);
                processResponse(res);

            } catch (InvalidProtocolBufferException e)
            {
                log.error("Failed to process Response message.", e);
            } catch (ZMQException e)
            {
                // Ignore. We expect an exception when shutting down
            }

            // Do something with response
        }
        cchan_in.close();
    }

    private void processResponse(Response res)
    {
        if (_responseProcessors.containsKey(res.getRequestId()))
        {
            ResponseProcessor responseProcessor = _responseProcessors
                    .remove(res.getRequestId());
            responseProcessor.process(res);
        } else if (_defaultResponseProcessor != null)
        {
            _defaultResponseProcessor.process(res);
        }
    }
}
