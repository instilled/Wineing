package org.instilled.wineing;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import org.instilled.wineing.core.Worker;
import org.instilled.wineing.core.ZMQChannel;
import org.instilled.wineing.core.ZMQChannel.ZMQChannelType;
import org.instilled.wineing.gen.WineingCtrlProto.Request;
import org.instilled.wineing.gen.WineingCtrlProto.Response;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.protobuf.InvalidProtocolBufferException;

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
                ZMQChannelType.PUSH_CONNECT);
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

            // Receive
            try
            {
                byte[] buf = ctrl_out.receive();
                // if (r < 0) {
                // log.error("Failed reading response.");
                // continue;
                // }

                Response response = Response.parseFrom(buf);
                switch (response.getCode())
                {
                case OK:
                    if (log.isDebugEnabled())
                    {
                        log.debug(String
                                .format("Received server response [id: %d, msg: %s]",
                                        response.getRequestId(),
                                        response.getCode().name()));
                    }
                    break;
                case ERR:
                    // TODO propagate state
                    throw new IllegalStateException(String.format(
                            "Failed to execute command. Error: %s.",
                            response.getText()));

                default:
                    break;
                }
            } catch (InvalidProtocolBufferException e)
            {
                log.error("Failed to parse Wineing response.", e);
            }
        }

        ctrl_out.close();
    }
}
