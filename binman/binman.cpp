/*
 * binman.cpp
 *
 *  Created on: 8 Jun 2020
 *      Author: gwinilts
 */

#include "binman.h"

binman::binman() {
	_count = 0;
	bits = new mqueue;
	bobs = new mqueue;

	__run = false;

}

binman::~binman() {
	delete bits;
	delete bobs;
}

void binman::finished(long bid) {
	b::bin *bin;

	bits_mtx.lock();

	bits->iter();

	bin = bits->iterNext<b::bin>();

	while (bin != NULL) {
		if (bin->bid == bid) break;
		bin = bits->iterNext<b::bin>();
	}

	if (bin != NULL) {
		bobs_mtx.lock();

		bobs->addItem(bin);

		bobs_mtx.unlock();
	}

	bits_mtx.unlock();
}

void binman::start() {
	__run = true;
	_run = std::thread(&binman::workLoop, this);
}

void binman::stop() {
	x_mtx.lock();
	__run = false;
	x_mtx.unlock();

	_run.join();

	bits->clear<b::bin>();
	bobs->clear<b::bin>();
}

void binman::workLoop() {
	b::bin *bin;
	while (true) {

		bits_mtx.lock();
		bobs_mtx.lock();

		this_thread::sleep_for(std::chrono::milliseconds(15));

		bobs->iter();
		bin = bobs->iterNext<b::bin>();

		while (bin != NULL) {
			//logx("delete ct " + to_string(bin->bid));
			bin->item->stop();
			bin = bobs->iterNext<b::bin>();
		}

		bobs->clear<b::bin>();

		bobs_mtx.unlock();
		bits_mtx.unlock();

		this_thread::sleep_for(std::chrono::milliseconds(1500));

		x_mtx.lock();
		if (!__run) {
			//logx("bye bye binman");
			return;
		}
		x_mtx.unlock();
	}
}



long binman::reg(ClientThread* t) {
	b::bin* it;

	bits_mtx.lock();

	bits->addItem(it = new b::bin(t, _count++));

	bits_mtx.unlock();
	//logx("registered ct - " + to_string(it->bid));
	return it->bid;
}

