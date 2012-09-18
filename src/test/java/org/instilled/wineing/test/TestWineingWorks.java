package org.instilled.wineing.test;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import org.instilled.wineing.core.WineingRemoteAPI;
import org.instilled.wineing.core.ZMQChannel;
import org.instilled.wineing.core.WineingRemoteAPI.WineingRemoteAPIImpl;
import org.instilled.wineing.core.ZMQChannel.ZMQChannelType;
import org.instilled.wineing.gen.WineingCtrlProto.Request;
import org.junit.Assert;

public class TestWineingWorks {

	private static String DEFAULT_CCHAN = "tcp://*:9990";
	private static String DEFAULT_MCHAN = "tcp://*:9992";
	private static String DEFAULT_TAPE_ROOT = "C:\\md\\";
	private static String DEFAULT_TAPE_FILE = "20100506.DQ.nxc";

	public void testWineing() {
		withDefaultServer(withDefaultClient(), new Action() {
			@Override
			public void execute(Client c) {
			}
		});
	}

	private static void withDefaultServer(Client c, Action a) {

		withServer(c, a, DEFAULT_CCHAN, DEFAULT_MCHAN, DEFAULT_TAPE_ROOT);
	}

	private static void withServer(Client c, Action a, String cchan,
			String mchan, String tapeRoot) {
		File exe = new File("target/wineing-0.0.1/wineing.exe");
		String cmd = exe.getAbsolutePath();

		if (!exe.isFile()) {
			Assert.fail("Could not find wineing.exe. You might need to run 'make' first from the <wineing-install-root>.");
		}

		try {
			Process p = Runtime.getRuntime().exec(
					String.format("%s --cchan=%s --mchan=%s --tape-root=%s", //
							cmd, //
							cchan, //
							mchan, //
							tapeRoot));

			// Give the server time to boot
			try {
				Thread.sleep(500);
			} catch (InterruptedException e) {
			}

			c.init();

			a.execute(c);

			c.shutdown();
			p.destroy();
		} catch (IOException e) {
			Assert.fail("Failed executing shell sommands. ");
		}
	}

	private static Client withDefaultClient() {
		return withClient(DEFAULT_CCHAN, DEFAULT_MCHAN);
	}

	private static Client withClient(String cchan, String mchan) {
		Client c = new Client(cchan, mchan);
		return c;
	}

	private static class Client {
		private ZMQChannel _ctrl;
		private ZMQChannel _market;
		private WineingRemoteAPI api;

		public Client(String cchan, String mchan) {
			_ctrl = new ZMQChannel(cchan, ZMQChannelType.REQ);
			_market = new ZMQChannel(cchan, ZMQChannelType.REQ);

			BlockingQueue<Request> queue = new ArrayBlockingQueue<Request>(1);
			api = new WineingRemoteAPIImpl(queue);
		}

		public void init() {
			_ctrl.connect();
			_market.connect();
		}

		public void shutdown() {
			_ctrl.close();
			_market.close();
		}

		public WineingRemoteAPI api() {
			return api;
		}
	}

	private static interface Action {
		void execute(Client c);
	}
}
