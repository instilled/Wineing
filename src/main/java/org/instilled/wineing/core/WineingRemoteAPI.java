package org.instilled.wineing.core;

import java.util.concurrent.BlockingQueue;

import org.instilled.wineing.gen.WineingCtrlProto.Request;
import org.instilled.wineing.gen.WineingCtrlProto.Request.Builder;
import org.instilled.wineing.gen.WineingCtrlProto.Request.Type;

public interface WineingRemoteAPI {
	void start(ResponseProcessor p);

	void start(String tape, ResponseProcessor p);

	void stop(ResponseProcessor p);

	void shutdown(ResponseProcessor p);

	public static class WineingRemoteAPIImpl implements WineingRemoteAPI {
		private BlockingQueue<Request> _ctrlQueue;

		public WineingRemoteAPIImpl(BlockingQueue<Request> ctrlQueue) {
			_ctrlQueue = ctrlQueue;
		}

		@Override
		public void start(ResponseProcessor p) {
			start(null, p);
		}

		@Override
		public void start(String tapeFile, ResponseProcessor p) {
			_ctrlQueue.offer(build(Type.START, tapeFile));
		}

		@Override
		public void stop(ResponseProcessor p) {
			_ctrlQueue.offer(build(Type.STOP));
		}

		@Override
		public void shutdown(ResponseProcessor p) {
			_ctrlQueue.offer(build(Type.SHUTDOWN));
		}

		private static Request build(Type type) {
			return build(type, null);
		}

		private static Request build(Type type, String tape) {
			Builder builder = Request.newBuilder();
			builder.setRequestId(getRequestId());
			builder.setType(type);
			if (tape != null) {
				builder.setTapeFile(tape);
			}
			Request r = builder.build();
			return r;
		}

		private static long getRequestId() {
			return System.currentTimeMillis();
		}
	}
}