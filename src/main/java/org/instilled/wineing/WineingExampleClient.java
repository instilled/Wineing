package org.instilled.wineing;

import java.util.LinkedList;
import java.util.List;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.OptionBuilder;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;
import org.instilled.wineing.core.ResponseProcessor;
import org.instilled.wineing.core.WineingRemoteAPI;
import org.instilled.wineing.core.Worker;
import org.instilled.wineing.core.ZMQChannel;
import org.instilled.wineing.core.ZMQChannel.ZMQChannelType;
import org.instilled.wineing.gen.WineingCtrlProto.Request;
import org.instilled.wineing.gen.WineingCtrlProto.Request.Builder;
import org.instilled.wineing.gen.WineingCtrlProto.Request.Type;
import org.instilled.wineing.gen.WineingCtrlProto.Response;
import org.instilled.wineing.gen.WineingCtrlProto.Response.ResponseCode;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.protobuf.InvalidProtocolBufferException;

/**
 * Issues:
 * <ul>
 * <li>Market data protobuf incomplete</li>
 * <li>Avoid re-allocating a new buffer every time receive is being
 * invoked</li>
 * </ul>
 */
public class WineingExampleClient
{
    public static final Logger log = LoggerFactory
            .getLogger(WineingExampleClient.class);

    private WineingClientCtx _ctx;

    private WineingRemoteAPI _api;

    private List<Worker> _workers = new LinkedList<Worker>();

    public static void main(String[] args)
    {
        // Command line stuff
        CommandLine cmd = parseCMDLine(args);
        String schan = cmd.getOptionValue("schan");
        String cchan = cmd.getOptionValue("cchan");
        String tape = cmd.getOptionValue("tape-file");

        WineingClientCtx ctx = new WineingClientCtx();
        ctx.schan = schan;
        ctx.cchan = cchan;

        log.debug("Starting "
                + WineingExampleClient.class.getSimpleName());

        // Initialization
        final WineingExampleClient client = new WineingExampleClient();
        client.init(ctx);
        client.start();

        // Using the API
        WineingRemoteAPI api = client.api();
        {
            api.setDefaultResponseProcessor(new ResponseProcessor()
            {
                @Override
                public void process(Response r)
                {
                    log.info(String
                            .format("Response for %s received [id: %i, code: %s, text: %s]", //
                                    Request.Type.START, //
                                    r.getRequestId(), //
                                    r.getCode(), //
                                    r.getText()));
                }
            });

            // Requests tape
            api.start(tape, new ResponseProcessor()
            {
                @Override
                public void process(Response r)
                {
                    log.info(String
                            .format("Response for %s received [id: %i, code: %s, text: %s]", //
                                    Request.Type.START, //
                                    r.getRequestId(), //
                                    r.getCode(), //
                                    r.getText()));
                }
            });

            api.start(tape, new ResponseProcessor()
            {
                @Override
                public void process(Response r)
                {
                    log.info(String
                            .format("Response for %s received [id: %i, code: %s, text: %s]", //
                                    Request.Type.SHUTDOWN, //
                                    r.getRequestId(), //
                                    r.getCode(), //
                                    r.getText()));
                }
            });

            // Do some work: see WorkerMarket.java

            // Terminate the appliaction
            api.shutdown(new ResponseProcessor()
            {
                @Override
                public void process(Response r)
                {
                    log.info(String
                            .format("Response for %s received [id: %i, code %s]", //
                                    Request.Type.SHUTDOWN, //
                                    r.getRequestId(), //
                                    r.getCode()));
                }
            });
        }

        client.shutdown();

        // Shutdown hook
        Runtime.getRuntime().addShutdownHook(new Thread()
        {
            @Override
            public void run()
            {
                log.debug("Shutting down "
                        + WineingExampleClient.class.getSimpleName());
                client.api().shutdown(null);
                client.shutdown();
            }
        });
    }

    public void init(WineingClientCtx ctx)
    {
        _ctx = ctx;
    }

    private void connectToServer(WineingClientCtx ctx)
    {
        log.info("Synchronizing with server");

        ZMQChannel chan = new ZMQChannel(ctx.schan, ZMQChannelType.REQ);
        chan.bind();

        // Send cchan (publishing control messages on this channel)
        Builder reqBuilder = Request.newBuilder();
        reqBuilder.setRequestId(1);
        reqBuilder.setType(Type.INIT);
        reqBuilder.setCchanFqcn(ctx.cchan);
        Request req = reqBuilder.build();
        chan.send(req.toByteArray());

        // Receive channel information
        byte[] buf = chan.receive();
        Response res;
        try
        {
            res = Response.parseFrom(buf);
        } catch (InvalidProtocolBufferException e)
        {
            throw new IllegalStateException(
                    "Failed to receive INIT message from Wineing", e);
        }

        if (!(ResponseCode.INIT.equals(res.getCode()) && //
                res.hasCchanFqcn() && res.hasMchanFqcn()))
        {
            throw new IllegalStateException(
                    "Did not receive either CCHAN-in or MCHAN fqcn");
        }

        String cchan_out = res.getCchanFqcn();
        String mchan = res.getMchanFqcn();

        if (log.isDebugEnabled())
        {
            log.debug(String.format(
                    "Exchanged ctrl [%s] and market [%s] endpoints", //
                    cchan_out, //
                    mchan));
        }

        ctx.cchan_out = cchan_out;
        ctx.mchan = mchan;
    }

    public void start()
    {
        log.debug("Initializing " + getClass().getSimpleName());

        // First we need to create the 'server' channel that will
        // listen for incoming Response messages sent by Wineing.
        WorkerCtrlIn workerCtrlIn = new WorkerCtrlIn(_ctx.cchan);
        _workers.add(workerCtrlIn);
        Thread worker_ctrl_in = new Thread(workerCtrlIn);
        worker_ctrl_in.start();

        // We can now connect to the server exchanging ctrl and market
        // data channel endpoints (sets them in WineingClientCtx). Both
        // are server channels that we will connect to later.
        connectToServer(_ctx);

        // Give the server time to create the server sockets we connect
        // to as we might fail to connect to the channels otherwise.
        // TODO is there a better way to synchronize client and server?
        try
        {
            Thread.sleep(500);
        } catch (InterruptedException e)
        {
        }

        // Control thread
        WorkerCtrlOut workerCtrlOut = new WorkerCtrlOut(_ctx.cchan_out);
        _workers.add(workerCtrlOut);
        Thread worker_ctrl_out = new Thread(workerCtrlOut);
        worker_ctrl_out.start();

        // Market thread
        WorkerMarket workerMarket = new WorkerMarket(_ctx.mchan);
        _workers.add(workerMarket);
        Thread worker_market = new Thread(workerMarket);
        worker_market.start();

        _api = new WineingRemoteAPIImpl(workerCtrlOut, workerCtrlIn);
    }

    public void shutdown()
    {
        for (Worker worker : _workers)
        {
            worker.shutdown();
        }
        ZMQChannel.destroy();
    }

    public WineingRemoteAPI api()
    {
        return _api;
    }

    private static class WineingClientCtx
    {
        private String schan;
        private String cchan;
        private String cchan_out;
        private String mchan;
    }

    public static class WineingRemoteAPIImpl implements
            WineingRemoteAPI
    {
        private WorkerCtrlOut _out;
        private WorkerCtrlIn _in;

        public WineingRemoteAPIImpl(WorkerCtrlOut out, WorkerCtrlIn in)
        {
            _out = out;
            _in = in;
        }

        @Override
        public void setDefaultResponseProcessor(ResponseProcessor p)
        {
            _in.setDefaultResponseProcessor(p);
        }

        @Override
        public void start(ResponseProcessor p)
        {
            start(null, p);
        }

        @Override
        public void start(String tapeFile, ResponseProcessor p)
        {
            Request r = build(Type.START, tapeFile);
            put(r, p);
        }

        @Override
        public void stop(ResponseProcessor p)
        {
            Request r = build(Type.STOP);
            put(r, p);
        }

        @Override
        public void shutdown(ResponseProcessor p)
        {
            Request r = build(Type.SHUTDOWN);
            put(r, p);
        }

        private void put(Request r, ResponseProcessor p)
        {
            if (p != null)
            {
                _in.setResponseProcessor(r.getRequestId(), p);
            }
            _out.addRequest(r);
        }

        private static Request build(Type type)
        {
            return build(type, null);
        }

        private static Request build(Type type, String tape)
        {
            Builder builder = Request.newBuilder();
            builder.setRequestId(getRequestId());
            builder.setType(type);
            if (tape != null)
            {
                builder.setTapeFile(tape);
            }
            Request r = builder.build();
            return r;
        }

        private static long getRequestId()
        {
            return System.currentTimeMillis();
        }
    }

    private static CommandLine parseCMDLine(String[] args)
    {
        CommandLine line = null;
        Options ops = _options();
        try
        {
            CommandLineParser parser = new PosixParser();
            line = parser.parse(ops, args);
        } catch (ParseException exp)
        {
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp(
                    WineingExampleClient.class.getSimpleName(), ops);
            System.exit(1);
        }

        return line;
    }

    @SuppressWarnings("static-access")
    private static Options _options()
    {
        Options o = new Options();

        o.addOption(new Option("h", "help", false,
                "Prints this message"));

        // schan option
        o.addOption(OptionBuilder
                .hasArg()
                .withArgName("fqcn")
                .isRequired()
                .withDescription(
                        "Synchronization socket.                               " //
                                + "ZMQ REQ/REP socket")
                .withLongOpt("schan").create("s"));

        // cchan option
        o.addOption(OptionBuilder
                .hasArg()
                .withArgName("fqcn")
                .isRequired()
                .withDescription(
                        "Control channel name.                                 " //
                                + "The control channel is a synchronous ZMQ " //
                                + "channel. The client shall connect with ZMQ's " //
                                + "REP to this channel.")
                .withLongOpt("cchan").create("c"));

        o.addOption(OptionBuilder
                .hasArg()
                .withArgName("tape")
                .withDescription(
                        "Backtesting mode.                                     " //
                                + "Requests TAPE from Wineing.")
                .withLongOpt("tape-file").create("t"));

        return o;
    }
}
