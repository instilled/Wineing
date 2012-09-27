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
                byte[] receive = cchan_in.receive();
                res = Response.parseFrom(receive);
                processResponse(res);

            } catch (InvalidProtocolBufferException e)
            {
                throw new IllegalStateException(
                        "Failed receiving Response", e);
            }

            // Do something with response
        }
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
