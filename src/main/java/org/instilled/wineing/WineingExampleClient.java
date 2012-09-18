package org.instilled.wineing;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.OptionBuilder;
import org.apache.commons.cli.OptionGroup;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.PosixParser;
import org.instilled.wineing.core.ResponseProcessor;
import org.instilled.wineing.core.WineingRemoteAPI;
import org.instilled.wineing.core.WineingRemoteAPI.WineingRemoteAPIImpl;
import org.instilled.wineing.core.ZMQChannel;
import org.instilled.wineing.core.ZMQChannel.ZMQChannelType;
import org.instilled.wineing.gen.WineingCtrlProto.Request;
import org.instilled.wineing.gen.WineingCtrlProto.Response;
import org.instilled.wineing.gen.WineingMarketDataProto.MarketData;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.protobuf.InvalidProtocolBufferException;

/**
 * Issues:
 * <ul>
 * <li>Market data protobuf incomplete</li>
 * <li>Avoid re-allocating a new buffer every time receive is being invoked</li>
 * </ul>
 */
public class WineingExampleClient {
	public static final Logger log = LoggerFactory
			.getLogger(WineingExampleClient.class);

	private volatile boolean _running;
	private List<Runnable> _workers = new ArrayList<Runnable>(2);

	private WineingRemoteAPI _api;

	public static void main(String[] args) {
		log.debug("Starting " + WineingExampleClient.class.getSimpleName());

		// Command line stuff
		CommandLine cmd = parseCMDLine(args);
		String cchan = cmd.getOptionValue("cchan");
		String mchan = cmd.getOptionValue("mchan");
		String tape = cmd.getOptionValue("tape-file");

		// Initialization
		WineingExampleClient client = new WineingExampleClient();
		client.init(cchan, mchan);
		client.start();

		// Using the API
		WineingRemoteAPI api = client.api();
		{
			// Requests tape
			api.start(tape, new ResponseProcessor() {
				@Override
				public void process(Response r) {
				}
			});

			// Do some work: see WineingExampleClient#market

			// Terminate the appliaction
			api.shutdown(new ResponseProcessor() {
				@Override
				public void process(Response r) {

				}
			});
		}

		// Shutdown hook
		Runtime.getRuntime().addShutdownHook(new Thread() {
			@Override
			public void run() {
				log.debug("Shutting down "
						+ WineingExampleClient.class.getSimpleName());
			}
		});
	}

	public void init(String cchan, String mchan) {
		log.debug("Initializing " + getClass().getSimpleName());

		// WineingRemopeAPI writes its actions to a queue. Requests
		// are processed by the ctrl thread.
		BlockingQueue<Request> ctrlQueue = new ArrayBlockingQueue<Request>(1);
		_api = new WineingRemoteAPIImpl(ctrlQueue);

		// Control thread
		log.debug(" -> control channel");
		ZMQChannel ctrl = new ZMQChannel(cchan, ZMQChannelType.REQ);
		ctrl.connect();
		_workers.add(ctrl(ctrl, ctrlQueue));

		// Market thread
		log.debug(" -> market data channel");
		ZMQChannel market = new ZMQChannel(mchan, ZMQChannelType.SUB);
		market.init();
		market.start();
		_workers.add(market(market));
	}

	public void start() {
		_running = true;
		
		// TODO single threads better than executor pool?
		ExecutorService executor = Executors.newFixedThreadPool(2);
		executor.submit(_workers.get(0));
		executor.submit(_workers.get(1));
		log.debug("Application started successfully");
	}

	public WineingRemoteAPI api() {
		return _api;
	}

	private Runnable ctrl(final ZMQChannel ctrl,
			final BlockingQueue<Request> ctrlQueue) {
		Runnable r = new Runnable() {
			// private byte[] buffer = new byte[2048];

			@Override
			public void run() {

				while (_running) {
					// Send
					try {
						Request request = ctrlQueue.take();
						if (log.isDebugEnabled()) {
							log.debug(String.format(
									"Sending request [id: %s, type: %s]",
									request.getRequestId(), request.getType()
											.name()));
						}

						byte[] buffer = request.toByteArray();
						ctrl.send(buffer);

					} catch (InterruptedException e) {
						// TODO proper error handling
						log.error("Failed to send request.", e);
					}

					// Receive
					try {
						byte[] buf = ctrl.receive();
						// if (r < 0) {
						// log.error("Failed reading response.");
						// continue;
						// }

						Response response = Response.parseFrom(buf);
						switch (response.getCode()) {
						case OK:
							if (log.isDebugEnabled()) {
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
					} catch (InvalidProtocolBufferException e) {
						log.error("Failed to parse Wineing response.", e);
					}
				}
			}
		};
		return r;
	}

	private Runnable market(final ZMQChannel market) {
		Runnable r = new Runnable() {
			// private byte[] buffer = new byte[2048];

			@Override
			public void run() {
				long count = 0;
				while (_running) {
					try {
						byte[] buf = market.receive();
						// if (r < 0) {
						// log.error("Failed parsing market data message");
						// continue;
						// }

						MarketData marketData = MarketData.parseFrom(buf);

						if (count % 1000 == 0) {
							log.debug(String
									.format("Message received (printing every 1000) [%s]",
											marketData.getType()));
						}

						count++;
					} catch (InvalidProtocolBufferException e) {
						log.error("Failed to parse MarketData message.", e);
					}
				}

				log.debug("Received " + count + " messages");
			}
		};
		return r;
	}

	private static CommandLine parseCMDLine(String[] args) {
		CommandLine line = null;
		Options ops = _options();
		try {
			CommandLineParser parser = new PosixParser();
			line = parser.parse(ops, args);
		} catch (ParseException exp) {
			HelpFormatter formatter = new HelpFormatter();
			formatter
					.printHelp(WineingExampleClient.class.getSimpleName(), ops);
			System.exit(1);
		}

		return line;
	}

	@SuppressWarnings("static-access")
	private static Options _options() {
		Options o = new Options();

		o.addOption(new Option("h", "help", false, "Prints this message"));

		// cchan option
		o.addOption(OptionBuilder
				.hasArg()
				.withArgName("fqcn")
				.isRequired()
				.withDescription(
						"Control channel name.                                 " //
								+ "The control channel is a synchronous ZMQ " //
								+ "channel. The client shall connect with ZMQ's " //
								+ "REP to this channel.").withLongOpt("cchan")
				.create("c"));

		// mchan option
		o.addOption(OptionBuilder
				.hasArg()
				.withArgName("fqcn")
				.isRequired()
				.withDescription(
						"Market data channel.                                  " //
								+ "The client receives market data on this channel. " //
								+ "It is a ZMQ SUB channel.")
				.withLongOpt("mchan").create("m"));

		// real-time/backtest
		OptionGroup g = new OptionGroup();
		g.isRequired();
		g.addOption(OptionBuilder
				.withDescription(
						"Real-time.                                            " //
								+ "Uses real-time market data.")
				.withLongOpt("real-time").create("r"));
		g.addOption(OptionBuilder
				.hasArg()
				.withArgName("tape")
				.withDescription(
						"Backtesting mode.                                     " //
								+ "Requests TAPE from Wineing.")
				.withLongOpt("tape-file").create("t"));
		o.addOptionGroup(g);

		return o;
	}
}
