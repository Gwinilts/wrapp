/*
 * ClientThread.h
 *
 *  Created on: 26 May 2020
 *      Author: gwinilts
 */

#ifndef CLIENTTHREAD_H_
#define CLIENTTHREAD_H_

#include <thread>
#include <iostream>
#include <unistd.h>
#include "./HTTP.h"
#include "./ws/websocket.h"
#include "./binman/binman.h"

#ifndef NETWORK_NETWORKLAYER_H_
#include "./network/networklayer.h"
#endif

#ifndef CPPTESTAGAIN_CHESS_H
#include "./chess/chess.h"
#endif


#include "./gxplatform.h"

class networklayer;
class chessgame;

namespace ctl {
	enum class msg_type: unsigned char {
		MIN = 0x11, // SET MIN AND MAX AS VALUES GET ADDED
		MAX = 0x15,
		rsc = 0x11,
		begin = 0x12,
		confirm = 0x13,
		contest = 0x14,
		upoll = 0x15
	};

	inline bool isMsgType(unsigned char v) {
		if (v < static_cast<unsigned char>(msg_type::MIN)) return false;
		if (v > static_cast<unsigned char>(msg_type::MAX)) return false;
		return true;
	}
}


using namespace std;

class binman;

class ClientThread {
private:
	bool running;
	bool dying;
	thread me;
	int sock;
	void *ast;

	websocket *ws;
	string bcast;

	binman* bin;
	long bid;

	networklayer *layer;

	chessgame *chess;

	std::mutex x_mtx;

	void fail(HTTP *h);
	void wsGetResource(size_t length, char * data);
	int wsGenRscMsg(char * &buf, string name, string mime, string data);

	void internalMsgHandler(ctl::msg_type, size_t, char*);

	void startNetworkLayer(size_t, char*);

	void nlUserPoll(size_t, char*);

	void masterWorkLoop();

	bool operator==(long bid) {
		return this->bid == bid;
	}

	bool isBid(long bid) {
		return this->bid == bid;
	}

public:
	ClientThread(int sock, void *ast, string bcast, binman* bin);
	virtual ~ClientThread();

	void wsMsgHandler(WS::OPCODE, size_t, char *);
	void nlMsgHandler(unsigned char, string, string, unsigned short int, char*);

	void stop();
	void run();
	void start();

	void shutdown();

	static void wsCallback(void *ref, WS::OPCODE, size_t, char*);
	static void nlCallback(void * ref, unsigned char, string, string, unsigned short int, char*);
};

#endif /* CLIENTTHREAD_H_ */
