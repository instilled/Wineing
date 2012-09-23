package org.instilled.wineing;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import org.instilled.wineing.core.Worker;
import org.instilled.wineing.core.ZMQChannel;
import org.instilled.wineing.core.ZMQChannel.ZMQChannelType;
import org.instilled.wineing.gen.WineingCtrlProto.Request;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class WorkerCtrlOut implements Worker
{
    public static final Logger log = LoggerFactory
            .getLogger(WorkerCtrlOut.class);

    private String _cchan_out;
    private BlockingQueue<Request> _ctrlQueue;

    private volatile boolean _running;

    public WorkerCtrlOut(String cchan_out)
    {
        _cchan_out = cchan_out;
        _ctrlQueue = new ArrayBlockingQueue<Request>(10);
    }

    public void shutdown()
    {
        _running = false;
    }

    public void addRequest(Request r)
    {
        _ctrlQueue.add(r);
    }

    @Override
    public void run()
    {
        _running = true;

        ZMQChannel ctrl_out = new ZMQChannel(_cchan_out,
                ZMQChannelType.PUB);
        ctrl_out.bind();

        while (_running)
        {
            // Send
            try
            {
                Request request = _ctrlQueue.take();
                if (log.isDebugEnabled())
                {
                    log.debug(String.format(
                            "Sending request [id: %s, type: %s]",
                            request.getRequestId(), request.getType()
                                    .name()));
                }

                byte[] buffer = request.toByteArray();
                ctrl_out.send(buffer);

            } catch (InterruptedException e)
            {
                // TODO proper error handling
                log.error("Failed to send request.", e);
            }
        }

        ctrl_out.close();
    }
}
