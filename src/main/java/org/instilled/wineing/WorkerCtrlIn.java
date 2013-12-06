package org.instilled.wineing;

import java.io.IOException;
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

import com.google.protobuf.CodedInputStream;

public class WorkerCtrlIn implements Worker
{
    public static final Logger log = LoggerFactory
            .getLogger(WorkerCtrlIn.class);

    private String _cchanIn;
    private ResponseProcessor _defaultResponseProcessor;
    private Map<Long, ResponseProcessor> _responseProcessors;

    private volatile boolean _running;

    private ZMQChannel _cchan_in;

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
        _cchan_in.close();
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
        byte[] buffer = new byte[1024];

        _running = true;

        // Start incoming channel. Listens to replies from
        // Wineing
        _cchan_in = new ZMQChannel(_cchanIn,
                ZMQChannelType.PULL_CONNECT);
        _cchan_in.bind();

        while (_running)
        {
            Response res;
            try
            {
                int read = _cchan_in.receive(buffer, 0, buffer.length);
                CodedInputStream is = CodedInputStream.newInstance(
                        buffer, 0, read);
                res = Response.parseFrom(is);
                processResponse(res);

            } catch (IOException e)
            {
                log.error("Failed to process Response message.", e);
            } catch (ZMQException e)
            {
                // Ignore. We expect an exception when shutting down
            }

            // Do something with response
        }
        _cchan_in.close();
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
