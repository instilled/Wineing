#ifndef _WCHAN_H
#define _WCHAN_H

#include "WineingCtrlProto.pb.h"
#include "WineingMarketDataProto.pb.h"

/**
 * The control channel (zmq req/rep). This is where wineing send
 * control messages.
 */
#define WCHAN_TYPE_CTRL               1

/**
 * Wineing streams market data updates on this channel to (all)
 * connected clients. This is a zmq PUB channel.
 */
#define WCHAN_TYPE_MARKET             2

#define WCHAN_RECV_BLOCK              0
#define WCHAN_RECV_NOBLOCK  ZMQ_NOBLOCK


/**
 * A channel definition. The current implementation uses ZMQ as the
 * underlying communication layer. It hides implementation details.
 */
struct wchan;

//typedef struct {
//
//} wchan_req;
//
//typedef struct {
//
//} wchan_resp;

//typedef WineingCmdProto::Response (*fnProcessMsg)(WineingCmdProto::Request);

/**
 * Initializes a zmq channel. Allocates a new wchan struct, invokes
 * zmq_init and finally zmq_socket. Clients should choose IPC as the
 * transport protocol, i.e. *ipc:///tmp/wineing/wchan/in*. *wchan_init*
 * WILL NOT create directories to match the *fqcn*, the client is
 * required to do so.
 *
 * \param fqcn
 * \param type One of WCHAN_TYPE_CTRL, or WCHAN_TYPE_MARKET
 *
 * \sa http://api.zeromq.org/2-1:zmq-socket#toc8
 * \sa http://api.zeromq.org/2-1:zmq-socket
 */
wchan* wchan_init(const char *fqcn, int type);

/**
 * Waits for a client to connect. Uses zmq's REQ/REP pattern.
 */
void wchan_wait_for_client(const char * fqcn);

/**
 * Binds wchan to its endpoint. After a call to *wchan_start*
 * the socket is ready for use.
 *
 * \sa http://api.zeromq.org/2-1:zmq-ipc
 */
void wchan_start(wchan *c);

/**
 * Stops a channel.
 * TODO: currently unused
 */
void wchan_stop(wchan *);

/**
 * Closes a wchan. Closing involves (in that order):
 * - closing the zmq socket
 * - terminating the zmq context
 * - freeing the memory pointed by wchan *
 *
 * Obviously any reference to wchan * will be invalid after calling
 * *wchan_free*.
 *
 * \param c Pointer to the wchan to be closed.
 */
void wchan_free(wchan *c);

/**
 * Receives a message from the control channel. If no message is
 * available *wchan_recv_ctrl* will block until a message is available.
 *
 * \param c     The wchan (control channel) to receive a message from.
 * \param msg   Request object (valid only if return == 1).
 * \return      If ZMQ call failed -1, 0 if parsing of message failed,
 *              or 1 in case of success.
 */
int wchan_recv_ctrl(wchan *c, WineingCtrlProto::Request &msg);

/**
 * Sends a control response message to the client channel.
 *
 * Important: Procedure is not thread safe. Concurrent access by two
 * (or more) threads will lead to corrupt data (access to a static
 * array).
 */
void wchan_send_ctrl(wchan *c, WineingCtrlProto::Response &msg);

/**
 * Sends market data update message to the client.
 *
 * Important: Procedure is not thread safe. Concurrent access by two
 * (or more) threads will lead to corrupt data (access to a static
 * array).
 */
void wchan_send_market(wchan *c, WineingMarketDataProto::MarketData &msg);

#endif /* _WCHAN_H */
