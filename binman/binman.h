/*
 * binman.h
 *
 *  Created on: 8 Jun 2020
 *      Author: gwinilts
 */

#ifndef BINMAN_BINMAN_H_
#define BINMAN_BINMAN_H_

#include <thread>
#include <mutex>
#include "../ClientThread.h"
#include "../mqueue/mqueue.h"

class ClientThread;

namespace b {
	class bin {
	public:
		ClientThread* item;
		long bid;

		bin(ClientThread* itm, long b) {
			item = itm;
			bid = b;
		}

		~bin() {
			delete item;
		}

		bool operator==(bin other) {
			return bid = other.bid;
		}

		operator ClientThread*() {
			return item;
		}
	};
}

class binman {
private:
	long _count;
	mqueue *bits;
	mqueue *bobs;

	std::mutex bits_mtx;
	std::mutex bobs_mtx;


	std::thread _run;
	bool __run;
	std::mutex x_mtx;
public:
	binman();
	virtual ~binman();

	long reg(ClientThread* t);
	void finished(long bid);
	void workLoop();
	void start();
	void stop();
};

#endif /* BINMAN_BINMAN_H_ */
