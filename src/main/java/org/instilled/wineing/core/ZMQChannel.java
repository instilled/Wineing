package org.instilled.wineing.core;

import org.zeromq.ZMQ;
import org.zeromq.ZMQ.Context;
import org.zeromq.ZMQ.Socket;

/**
 * An abstraction to the ZMQ Java binding hiding details of socket
 * creation, initialization and connection. For now {@link ZMQChannel}
 * creates just one {@link ZMQ.Context} using one IO thread, see
 * {@link ZMQ#context(int)}.<br>
 * <br>
 * <b>Note</b>: This class is not thread-safe. An application usually
 * creates one (or more) {@link ZMQChannel}s per {@link Thread}. This is
 * due to the nature of ZMQ sockets. Making {@link ZMQChannel}
 * thread-safe is easy but due to performance requirements was not
 * undertaken.
 */
public class ZMQChannel
{
    private static Context CTX = ZMQ.context(1);

    private String _fqcn;
    private ZMQChannelType _type;

    private Socket _sock;

    public ZMQChannel(String fqcn, ZMQChannelType type)
    {
        _fqcn = fqcn;
        _type = type;
    }

    /**
     * Initializes the ZMQ context and socket but does not yet to the
     * socket.
     */
    private void init()
    {
        _sock = CTX.socket(_type.getNativeType());
    }

    /**
     * Connects to the socket. Making the channel ready for
     * sending/receiving data. <br>
     * <br>
     * <b>Note:</b> If the channel type is of {@link ZMQChannelType#SUB}
     * this method will initially subscribe to all messages.
     */
    public void bind()
    {
        init();

        switch (_type)
        {
        case REP:
        case PUB:
        case PULL_BIND:
        case PUSH_BIND:
            _sock.bind(_fqcn);
            break;

        case REQ:
        case SUB:
        case PULL_CONNECT:
        case PUSH_CONNECT:
            _sock.connect(_fqcn);
            break;

        default:
            throw new IllegalStateException("Case not implemented ["
                    + _type + "]");
        }

        if (ZMQChannelType.SUB.equals(_type))
        {
            _sock.subscribe(new byte[0]);
        }
    }

    public String getFqcn()
    {
        return _fqcn;
    }

    public ZMQChannelType getType()
    {
        return _type;
    }

    public Socket getSocket()
    {
        return _sock;
    }

    /**
     * Sends data to socket.<br>
     * <br>
     * The send operation is not supported for all
     * {@link ZMQChannelType}s. Correct use is left to the application.
     * 
     * @param data
     */
    public void send(byte[] data)
    {
        _sock.send(data, 0);
    }

    /**
     * Does exactly the same as {@link ZMQChannel#receive()} but does
     * not block waiting for a message to arrive.
     * 
     * @param buffer
     * @param offset
     * @param len
     * @return Number of bytes read into buffer.
     */
    public byte[] receiveNoblock()
    {
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
    public int receiveNoblock(byte[] buffer, int offset, int len)
    {
        // TODO use _sock.recv(buffer, ..., ..., ...) instead of
        // allocating a data buffer with each call
        // return _sock.recv(buffer, 0, len, ZMQ.NOBLOCK);
        throw new IllegalStateException("Method not implemented.");
    }

    /**
     * Shall receive a message. This is a blocking operation. <br>
     * <br>
     * The receive operation is not available for every
     * {@link ZMQChannelType}. Correct use is left to the application. <br>
     * <br>
     * See {@link Socket#recv(byte[], int, int, int)} for a detailed
     * description.
     * 
     * @return Number of bytes received.
     */
    public byte[] receive()
    {
        // TODO use _sock.recv(buffer, ..., ..., ...) instead of
        // allocating a data buffer with each call

        // Somehow this dosen't work
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
    public int receive(byte[] buffer, int offset, int len)
    {
        // TODO use _sock.recv(buffer, ..., ..., ...) instead of
        // allocating a data buffer with each call

        // Some how this dosen't work
        // return _sock.recv(buffer, offset, len, 0);
        throw new IllegalStateException("Method not implemented.");
    }

    /**
     * Terminates the socket releasing any resources. The channel is no
     * longer valid and has to be {@link #init()} and {@link #bind()}
     * again.
     */
    public void close()
    {
        _sock.close();
    }

    /**
     * Releases all ZMQ resources.
     */
    public static void destroy()
    {
        CTX.term();
    }

    /**
     * ZMQ socket type abstraction. Each enum maps to the <em>type</em>
     * in the <em>zmq_socket</em> function.
     * 
     * @see http://api.zeromq.org/2-1:zmq-socket
     */
    public static enum ZMQChannelType
    {
        /**
         * Server PUB/SUB channel.
         */
        PUB(ZMQ.PUB), //

        /**
         * Client PUB/SUB channel.
         */
        SUB(ZMQ.SUB), //

        /**
         * Client REQ/REP channel.
         */
        REQ(ZMQ.REQ), //

        /**
         * Server REQ/REP channel.
         */
        REP(ZMQ.REP),

        /**
		 * 
		 */
        PUSH_BIND(ZMQ.PUSH),

        /**
		 * 
		 */
        PUSH_CONNECT(ZMQ.PUSH),

        /**
		 * 
		 */
        PULL_BIND(ZMQ.PULL),

        /**
		 * 
		 */
        PULL_CONNECT(ZMQ.PULL);

        private int _type;

        private ZMQChannelType(int type)
        {
            _type = type;
        }

        public int getNativeType()
        {
            return _type;
        }
    }
}
