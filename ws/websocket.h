/*
 * websocket.h
 *
 *  Created on: 27 May 2020
 *      Author: gwinilts
 */

#ifndef WS_WEBSOCKET_H_
#define WS_WEBSOCKET_H_

#include "./MsgQueue.h"
#include <thread>
#include <mutex>

/**
 * how we gon' do this
 *
 * on init we start a thread for readLoop
 * readLoop loops as long as the websocket is considered active and attempts to read msgs from the websocket
 * if a msg arrives we lock in_mtx and add the msg to in(MsgQueue)
 *
 * at the end of init we start workLoop
 * workLoop takes one msg off in(MsgQueue) and sends it through onMessage (if there is a onMsgHandler)
 * workLoop takes one msg off out(MsgQueue) and sends it down the socket.
 *
 *
 * notes
 *
 * when we get a ping we should respond with only one pong, even if we get another ping before the pong is sent
 * MsgQueue needs to have a hasOP or etc method to determine if there is already a ping in the queue.
 * or a addIfNone method specifically for ping/pong?
 *
 * nope,
 */

class websocket {
private:
	int sock;
	bool hasMsgHandler;

	void (*msgHandler)(WS::OPCODE, size_t, char *);
	void *msgTgt;
	void (*tgtMsgHandler)(void *thing, WS::OPCODE, size_t, char*);

	std::mutex in_mtx;
	MsgQueue *in; // in needs a mutex because it is xed in this thread and xed in another thread through read loop

	std::mutex out_mtx;
	MsgQueue *out;

	long _pong;
	long _timeout;

	bool _open;
	bool _readLoop;

	std::mutex x_mtx;

	std::thread __read;
	std::thread __work;

	void workLoop(); // where we do our msg processing and trigger call backs



	void readLoop(); // where we do out reads on another thread so ve are free to send msgs
	void pushFrame(struct WS::frame frame);

	void sendFrame(struct WS::frame frame);

	void uhOh();

public:
	websocket(int sock);
	virtual ~websocket();

	void onMessage(void (*msgHandler)(WS::OPCODE, size_t, char*));
	void onMessage(void (*msgHandler)(void*, WS::OPCODE, size_t, char*), void* something);

	void sendMessage(bool text, size_t length, char * data);
	void sendCtl(unsigned char, size_t length, char* data);

	bool open();
	void close();

	// func(websocket me, char *payload, int p_length)
};

#endif /* WS_WEBSOCKET_H_ */
