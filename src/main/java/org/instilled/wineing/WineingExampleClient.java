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
import org.instilled.wineing.gen.WineingCtrlProto.Request;
import org.instilled.wineing.gen.WineingCtrlProto.Request.Builder;
import org.instilled.wineing.gen.WineingCtrlProto.Request.Type;
import org.instilled.wineing.gen.WineingCtrlProto.Response;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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
        // Shutdown hook
        Runtime.getRuntime().addShutdownHook(new Thread()
        {
            @Override
            public void run()
            {
                log.debug("Shutting down "
                        + WineingExampleClient.class.getSimpleName());
            }
        });

        // Command line stuff
        CommandLine cmd = parseCMDLine(args);
        String cchan_in = cmd.getOptionValue("cchan-in");
        String cchan_out = cmd.getOptionValue("cchan-out");
        String mchan = cmd.getOptionValue("mchan");
        String tape = cmd.getOptionValue("tape-file");

        WineingClientCtx ctx = new WineingClientCtx();
        ctx.cchan_in = cchan_in;
        ctx.cchan_out = cchan_out;
        ctx.mchan = mchan;

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
                            .format("[DEFAULT] Response for %s received [id: %d, type: %s, error text: %s]", //
                                    Request.Type.MARKET_START, //
                                    r.getRequestId(), //
                                    r.getType(), //
                                    r.getErrText()));
                }
            });

            // Requests tape (use default RequestProcessor)
            api.start(tape, null);

            for (int i = 0; i < 10; i++)
            {
                api.start(tape, new ResponseProcessor()
                {
                    @Override
                    public void process(Response r)
                    {
                        log.info(String
                                .format("Response for %s received [id: %d, type: %s, error text: %s]", //
                                        Request.Type.MARKET_START, //
                                        r.getRequestId(), //
                                        r.getType(), //
                                        r.getErrText()));
                    }
                });
            }

            // Do some work: see WorkerMarket.java
            try
            {
                Thread.sleep(2000);
            } catch (InterruptedException e)
            {
            }

            // Terminate the appliaction
            api.shutdown(new ResponseProcessor()
            {
                @Override
                public void process(Response r)
                {
                    log.info(String
                            .format("Response for %s received [id: %d, code %s]", //
                                    Request.Type.SHUTDOWN, //
                                    r.getRequestId(), //
                                    r.getType()));
                }
            });

            // Wait a few seconds before shutting down the
            // application
            try
            {
                Thread.sleep(1000);
            } catch (InterruptedException e)
            {
            }
            client.shutdown();

        }
    }

    public void init(WineingClientCtx ctx)
    {
        _ctx = ctx;
    }

    public void start()
    {
        log.debug("Initializing " + getClass().getSimpleName());

        // First we need to create the 'server' channel that will
        // listen for incoming Response messages sent by Wineing.
        WorkerCtrlIn workerCtrlIn = new WorkerCtrlIn(_ctx.cchan_in);
        _workers.add(workerCtrlIn);
        Thread worker_ctrl_in = new Thread(workerCtrlIn, "WorkerCtrlIn");
        worker_ctrl_in.start();

        // Control thread
        WorkerCtrlOut workerCtrlOut = new WorkerCtrlOut(_ctx.cchan_out);
        _workers.add(workerCtrlOut);
        Thread worker_ctrl_out = new Thread(workerCtrlOut,
                "WorkerCtrlOut");
        worker_ctrl_out.start();

        // Market thread
        WorkerMarket workerMarket = new WorkerMarket(_ctx.mchan);
        _workers.add(workerMarket);
        Thread worker_market = new Thread(workerMarket, "WorkerMarket");
        worker_market.start();

        _api = new WineingRemoteAPIImpl(workerCtrlOut, workerCtrlIn);
    }

    public void shutdown()
    {
        for (Worker worker : _workers)
        {
            worker.shutdown();
        }

        // We could use Thread.interrupt() method if ZMQ would not use
        // non-interruptible (native) channels. We thus
        // close/destroy the context which nearly does the same.
        //ZMQChannel.destroy();
    }

    public WineingRemoteAPI api()
    {
        return _api;
    }

    private static class WineingClientCtx
    {
        private String cchan_in;
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
            Request r = build(Type.MARKET_START, tapeFile);
            put(r, p);
        }

        @Override
        public void stop(ResponseProcessor p)
        {
            Request r = build(Type.MARKET_STOP);
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
            return System.nanoTime();
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
                        "Control channel-in. Where the application receives    " //
                                + "response messages from Wineing. This is a   " //
                                + "ZMQ PUSH/PULL socket")
                .withLongOpt("cchan-in").create("i"));

        // cchan option
        o.addOption(OptionBuilder
                .hasArg()
                .withArgName("fqcn")
                .isRequired()
                .withDescription(
                        "Control channel-out. Used to send control messages    "//
                                + "to Wineing. It is a ZMQ PUSH/PULL channel.") //
                .withLongOpt("cchan-out").create("o"));

        // cchan option
        o.addOption(OptionBuilder
                .hasArg()
                .withArgName("fqcn")
                .isRequired()
                .withDescription(
                        "Market data channel. The application receives market  " //
                                + "data updates on this channel. It's a ZMQ    "//
                                + "PUB/SUB channel.") //
                .withLongOpt("mchan").create("m"));

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
