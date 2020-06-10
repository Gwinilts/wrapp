/*
 * websocket.cpp
 *
 *  Created on: 27 May 2020
 *      Author: gwinilts
 */

#include "websocket.h"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <ctime>
#include <chrono>
#include <cstring>

#include "../gxplatform.h"

using namespace std;

bool prop(char byte, char pos) {
	if ((pos < 1) || (pos > 8)) return false;
	return ((1 << (pos - 1)) & byte) > 0;
}

long mili() {
	std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
			std::chrono::system_clock::now().time_since_epoch()
	);

	return ms.count();
}

namespace WS {
	char* getPing() {
		char *ping = new char[9];
		ping[0] = 'b';
		ping[1] = 'e';
		ping[2] = 'e';
		ping[3] = 'f';
		ping[4] = 'c';
		ping[5] = 'a';
		ping[6] = 'c';
		ping[7] = 'a';
		ping[8] = '\0';

		return ping;
	}
}



websocket::websocket(int sock) {
	this->sock = sock;
	this->in = new MsgQueue(true);
	this->out = new MsgQueue(false);
	this->_open = false;
	this->_readLoop = false;

	_pong = -1;
	_timeout = 0;

}

websocket::~websocket() {
	delete this->in;
	delete this->out;
}

void websocket::onMessage(void (*func)(WS::OPCODE, size_t, char*)) {
	this->msgHandler = func;
	this->hasMsgHandler = true;
}

void websocket::onMessage(void (*func)(void*, WS::OPCODE, size_t, char*), void* something) {
	this->tgtMsgHandler = func;
	this->hasMsgHandler = true;
	this->msgTgt = something;
}

bool websocket::open() {
	_open = true;
	_readLoop = true;

	__read = std::thread(&websocket::readLoop, this);
	//__read.detach();
	__work = std::thread(&websocket::workLoop, this);
	//__work.detach();

	return false;
}

void websocket::uhOh() {
	if (hasMsgHandler) {
		tgtMsgHandler(msgTgt, WS::OPCODE::FATAL, 0, NULL);
	}
}

void websocket::close() {
	x_mtx.lock();
	this->_readLoop = false;
	this->_open = false;
	::shutdown(sock, SHUT_RDWR);
	x_mtx.unlock();

	if (__read.joinable()) __read.join();
	if (__work.joinable()) __work.join();
	logx("close websocket finished");
}

void websocket::pushFrame(WS::frame frame) { // you are responsible for deleting the frame's payload pointer
	switch (frame.opcode) {
	case WS::OPCODE::CONT:
		in_mtx.lock();
		in->pushSegment(frame.payload, frame.length, frame.fin);
		in_mtx.unlock();
		break;
	case WS::OPCODE::PONG:
		_pong = mili();
		_timeout = 0;
		break;
	case WS::OPCODE::PING:
		cout << "got a ping " << endl;
		out_mtx.lock();
		out->addMsg(WS::OPCODE::PONG, frame.payload, frame.length);
		out_mtx.unlock();
		break;
	case WS::OPCODE::RAW:
	case WS::OPCODE::TEXT:
		if (frame.fin) {
			in_mtx.lock();
			in->addMsg(frame.opcode, frame.payload, frame.length);
			in_mtx.unlock();
		} else {
			logx("msg is seg");
			in_mtx.lock();
			in->pushSegment(frame.payload, frame.length, false, frame.opcode);
			in_mtx.unlock();
		}
		break;
	default:
		uhOh();
	}
}

void websocket::sendFrame(struct WS::frame frame) {
	char buf[2048];
	size_t fill;

	unsigned char* _buf;

	buf[0] = static_cast<unsigned char>(frame.opcode) & 0b00001111;

	if (frame.fin) {
		buf[0] |= 0b10000000;
	}

	fill = 2;

	if (frame.length < 126) {
		buf[1] = frame.length & 0b01111111;
	} else {
		if (frame.length <= 0xEFFF) { // 16 bit size
			buf[1] = 126;

			_buf = reinterpret_cast<unsigned char *>(&frame.length);

			buf[2] = _buf[1];
			buf[3] = _buf[0];

			fill += 2;
		} else { // 64 bit size
			buf[1] = 127;

            _buf = reinterpret_cast<unsigned char *>(&frame.length);

            buf[2] = _buf[7];
            buf[3] = _buf[6];
            buf[4] = _buf[5];
            buf[5] = _buf[4];
            buf[6] = _buf[3];
            buf[7] = _buf[2];
            buf[8] = _buf[1];
            buf[9] = _buf[0];

			fill += 8;
		}
	}

	send(sock, buf, fill, 0);
	send(sock, frame.payload, frame.length, 0);

	delete [] frame.payload;
}

/*
 * if we are sending a segmented message then we cannot send another whole message until the segmented message is complete?
 */

void websocket::workLoop() {

	WS::msg msg;
	WS::frame frame;

	logx("websocket work loop begin --");

	long pong = mili();

	bool __run = true;


	while (__run) {
		if (_pong > 0) {
			if ((mili() - _pong) > 2500) {
				_timeout++;
				_pong = mili();
			}
		}
		if (_timeout > 5) {
			logx("socket is probably disconnected");
			_readLoop = false;
			return;
		}
		if (hasMsgHandler) {
			in_mtx.lock();
			if (in->hasNext()) {
				msg = in->nextMsg();
				if (msgTgt != NULL) {
					tgtMsgHandler(msgTgt, msg.type, msg.length, msg.data);
				} else {
					msgHandler(msg.type, msg.length, msg.data); // msg handler is responsible for deleting data when it is done.
				}
			}
			in_mtx.unlock();
		}

		out_mtx.lock();
		if (out->hasNextSegment()) {
			sendFrame(out->nextSegment());
		} else if (out->hasNext()) {
			msg = out->nextMsg();

			frame.fin = true;
			frame.length = msg.length;
			frame.opcode = msg.type;
			frame.payload = msg.data;

			sendFrame(frame);
		}

		if ((mili() - pong) > 2500) {
			out->addMsg(WS::OPCODE::PING, WS::getPing(), 8);
			pong = mili(); // haha we were overwhelming the client with pings
		}

		out_mtx.unlock();

		x_mtx.lock();
		__run = _open;
		x_mtx.unlock();

		this_thread::sleep_for(std::chrono::milliseconds(15));
	}

	logx("websocket work loop end --");
}

void websocket::sendMessage(bool text, size_t length, char* data) {
	out_mtx.lock();

	WS::OPCODE op;

	if (text) {
		op = WS::OPCODE::TEXT;
	} else {
		op = WS::OPCODE::RAW;
	}

	out->addMsg(op, data, length);
	out_mtx.unlock();
}

void websocket::sendCtl(unsigned char opcode, size_t length, char* data) {
	char* msg = new char[length + 1];
	msg[0] = opcode;

	if (data != NULL && length > 0) {
		memcpy(msg + 1, data, length);
		delete [] data;
	}

	sendMessage(false, length + 1, msg);
}


void websocket::readLoop() {
	size_t r, fill, fetch;

	bool mask;

	char buf[2048];
	char key[4];
	char _buf[8];

	struct WS::frame frame;
	bool fatal = false;

	bool __run = true;

	logx("websocket read loop begin --");

	while (__run) {
		//cout << "time to read a frame" << endl;
		r = read(sock, buf, 2);

		//cout << "2b read" << endl;

		if (r == 2) {
			frame.fin = prop(buf[0], 8);
			frame.opcode = static_cast<WS::OPCODE>(buf[0] & 0b00001111);
			mask = prop(buf[1], 8);
			frame.length = buf[1] & 0b01111111;
		} else {
			fatal = true;
			break;
		}

		if (!mask) {
			fatal = true;
			break;
		}

		if (frame.length > 125) {
			if (frame.length == 126) {
				if (read(sock, buf, 2) == 2) {
					_buf[0] = buf[1];
					_buf[1] = buf[0];
					frame.length = *reinterpret_cast<const unsigned short int*>(_buf);
				} else {
					fatal = true;
					break;
				}
			} else {
				if (read(sock, buf, 8) == 8) {
					_buf[0] = buf[7];
					_buf[1] = buf[6];
					_buf[2] = buf[5];
					_buf[3] = buf[4];
					_buf[4] = buf[3];
					_buf[5] = buf[2];
					_buf[6] = buf[1];
					_buf[7] = buf[0];
					frame.length = *reinterpret_cast<const unsigned long int*>(_buf);
				} else {
					fatal = true;
					break;
				}
			}
		}

		if (read(sock, key, 4) != 4) {
			fatal = true;
			break;
		}

		frame.payload = new char[frame.length];
		fill = 0;

		while (fill < frame.length) {
			if (frame.length < sizeof(buf)) {
				fetch = frame.length;
			} else {
				fetch = sizeof(buf);
			}
			r = read(sock, buf, fetch);

			for (size_t i = fill, b = 0; (i < frame.length) && (b < r); i++, b++) {
				frame.payload[i] = (buf[b] ^ key[i % 4]);
			}

			fill += r;
		}

		//cout << "i just read frame" << endl;

		pushFrame(frame);

		x_mtx.lock();
		__run = _readLoop;
		x_mtx.unlock();

		this_thread::sleep_for(std::chrono::milliseconds(15));
	}

	logx("websocket read loop end --");

	if (fatal) {
		uhOh();
	}
}

