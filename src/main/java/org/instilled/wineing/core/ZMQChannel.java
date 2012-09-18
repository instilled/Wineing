package org.instilled.wineing.core;

import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Context;
import org.zeromq.ZMQ.Socket;

/**
 * An abstraction to the ZMQ Java binding to hide the details of socket
 * initialization and connection.
 */
public class ZMQChannel {
	private String _fqcn;
	private ZMQChannelType _type;

	private Context _ctx;
	private Socket _sock;

	public ZMQChannel(String fqcn, ZMQChannelType type) {
		_fqcn = fqcn;
		_type = type;
	}

	/**
	 * Initializes the ZMQ context and socket but does not yet to the socket.
	 * The channel will be ready for sending/receiving only after a call to
	 * {@link #start()}.
	 */
	public void init() {
		_ctx = ZMQ.context(1);

		int type;

		switch (_type) {

		case PUB:
			type = ZMQ.PUB;
			break;
		case SUB:
			type = ZMQ.SUB;
			break;

		case REP:
			type = ZMQ.REP;
			break;
		case REQ:
			type = ZMQ.REQ;
			break;

		default:
			throw new IllegalStateException(
					"Could not initialize ZMQChannel. Invalid type [" + _type
							+ "].");
		}
		_sock = _ctx.socket(type);
	}

	/**
	 * Connects to the socket. Making the channel ready for sending/receiving
	 * data. <br>
	 * <br>
	 * <b>Note:</b> If the channel type is of {@link ZMQChannelType#SUB} this
	 * method will initially subscribe to all messages.
	 */
	public void start() {
		switch (_type) {
		case REQ:
			_sock.connect(_fqcn);
			break;
		case SUB:
			_sock.connect(_fqcn);
			_sock.subscribe(new byte[0]);
			break;

		default:
			throw new IllegalStateException("Case not implemented [" + _type
					+ "]");
		}
	}

	public String getFqcn() {
		return _fqcn;
	}

	public ZMQChannelType getType() {
		return _type;
	}

	public Context getContext() {
		return _ctx;
	}

	public Socket getSocket() {
		return _sock;
	}

	/**
	 * Sends data to socket. ZMQ requires the last byte of the data to be '0',
	 * that is <code>data[data.length - 1] = 0</code>. <br>
	 * <br>
	 * The send operation is not supported for all {@link ZMQChannelType}s.
	 * Correct use is left to the application.
	 * 
	 * @param data
	 */
	public void send(byte[] data) {
		_sock.send(data, 0);
	}

	/**
	 * Does exactly the same as {@link ZMQChannel#receive()} but does not block
	 * waiting for a message to arrive.
	 * 
	 * @param buffer
	 * @param offset
	 * @param len
	 * @return Number of bytes read into buffer.
	 */
	public byte[] receiveNoblock() {
		// TODO use _sock.recv(buffer, ..., ..., ...) instead of
		// allocating a data buffer with each call
		return _sock.recv(ZMQ.NOBLOCK);
	}

	/**
	 * See {@link #receiveNoblock()}.
	 * 
	 * @param buffer
	 * @param offset
	 * @param len
	 * @return
	 */
	public int receiveNoblock(byte[] buffer, int offset, int len) {
		// TODO use _sock.recv(buffer, ..., ..., ...) instead of
		// allocating a data buffer with each call
		// return _sock.recv(buffer, 0, len, ZMQ.NOBLOCK);
		throw new IllegalStateException("Method not implemented.");
	}

	/**
	 * Shall receive a message. This is a blocking operation. <br>
	 * <br>
	 * The receive operation is not available for every {@link ZMQChannelType}.
	 * Correct use is left to the application. <br>
	 * <br>
	 * See {@link Socket#recv(byte[], int, int, int)} for a detailed
	 * description.
	 * 
	 * @return Number of bytes received.
	 */
	public byte[] receive() {
		// TODO use _sock.recv(buffer, ..., ..., ...) instead of
		// allocating a data buffer with each call

		// Some how this dosen't work
		// return _sock.recv(buffer, offset, len, 0);
		return _sock.recv(0);
	}

	/**
	 * See {@link #receive()}.
	 * 
	 * @param buffer
	 * @param offset
	 * @param len
	 * @return
	 */
	public int receive(byte[] buffer, int offset, int len) {
		// TODO use _sock.recv(buffer, ..., ..., ...) instead of
		// allocating a data buffer with each call

		// Some how this dosen't work
		// return _sock.recv(buffer, offset, len, 0);
		throw new IllegalStateException("Method not implemented.");
	}

	/**
	 * Terminates the socket releasing any resources. The channel is no longer
	 * valid and has to be {@link #init()} and {@link #start()} again.
	 */
	public void close() {
		_sock.close();
		_ctx.term();
	}

	/**
	 * Calls {@link #init()} and {@link #connect()}.
	 */
	public void connect() {
		init();
		start();
	}

	/**
	 * ZMQ socket type abstraction. Each enum maps to the <em>type</em> in the
	 * <em>zmq_socket</em> function.
	 * 
	 * @see http://api.zeromq.org/2-1:zmq-socket
	 */
	public static enum ZMQChannelType {
		/**
		 * Server PUB/SUB channel.
		 */
		PUB, //

		/**
		 * Client PUB/SUB channel.
		 */
		SUB, //

		/**
		 * Client REQ/REP channel.
		 */
		REQ, //

		/**
		 * Server REQ/REP channel.
		 */
		REP;
	}
}
