/*
 * networklayer.cpp
 *
 *  Created on: 4 Jun 2020
 *      Author: gwinilts
 */

/**
 * so how we gon do this?
 *
 * take a packet size limit of 2048 bytes
 * try to read 2048 bytes each time
 *
 * a packet will consist of:
 * 	- a verity header       always 1 byte
 * 	this header contains the bytes {0b 0e 0e 0f 0c 0a 0c 0a}
 *
 * 	if the first 8 bytes of the packet do not match the above, drop it
 *
 * 	 * 	- the opcode, an 8 bit integer
 * 	the opcode is used to determine what this packet is for and to where or what it's payload should be sent
 *
 * 	- the size, a 16 bit integer      always 2 bytes
 * 	this is the size of the content
 *
 * 	if it is greater than 2040 drop the packet
 *
 * 	- the sender/recipient name size, an 8 bit integer always 1 byte
 *
 * 	this is the size of the sender name and the recipient name packed into a char
 * 	since the name limit is always 14, it takes at most 4 bits to represent the size
 *
 * 	the four rightmost bits are the size of the sender naem (the sender name is always sent)
 *
 * 	the four leftmost bits are the size of the recipient name
 *
 * 	if the four leftmost bits are 0000, there is no recipient name and this packet will
 * 	be processed by everybody who is listening
 *
 * 	- the sender name, given by the previous char
 *
 * 	- the recipient name, given by the previous char if it had a non zero value for [expr: 0b11110000 & it]
 *
 * 	- the payload, an array of bytes
 * 	this is the actual message content
 *
 * 	layout:
 *
 * 	8 bytes: signature
 * 	1 byte: opcode
 * 	2 bytes: size
 * 	1 byte: name size
 * 	string: sender
 * 	string: recipient
 * 	void: payload
 *
 *
 *
 */

#include "networklayer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
/*
long mili() {
	std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
			std::chrono::system_clock::now().time_since_epoch()
	);

	return ms.count();
}*/

long networklayer::now() {
	std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(
		std::chrono::system_clock::now().time_since_epoch()
	);

	return ms.count();
}

void networklayer::start() {
	_workLoop = true;
	_readLoop = true;

	__work = std::thread(&networklayer::workLoop, this);
	__read = std::thread(&networklayer::readLoop, this);

	logx("network layer has started");
}

void networklayer::stop() {
	logx("nl acquiring lock for stop");
	x_mtx.lock();
	_readLoop = false;
	_workLoop = false;

	shutdown(in_sock, SHUT_RDWR);
	shutdown(out_sock, SHUT_RDWR);

	x_mtx.unlock();
	logx("lock released");

	if (__read.joinable()) __read.join(); else logx("can't join nl read thread, perhaps it died already?");

	if (__work.joinable()) __work.join(); else logx("can't join nl work thread, perhaps it died already?");

	in->clear<nl::msg>();
	out->clear<nl::msg>();

	u_mtx.lock();
	ulist->clear<nl::peer>();
	u_mtx.unlock();

	logx("networklayer threads have exited");
}

networklayer::networklayer(std::string name, std::string bcast) {
	_since = new long[static_cast<unsigned char>(nl::op_max) + 1];

	for (size_t i = 0; i < nl::op_max + 1; i++) {
		_since[i] = 0;
	}

	this->name = name;

	if (this->name.empty()) {
		logx("name is empty");
	}

	logx("network layer " + name + ": " + bcast);

	struct sockaddr_in *addr = new sockaddr_in;
	addr->sin_port = htons(DEST_PORT);
	addr->sin_family = AF_INET;

	if (inet_pton(AF_INET, bcast.c_str(), &(addr->sin_addr)) == 1) {
		logx("address was understood");
	}



	dest = (sockaddr *) addr;
	d_len = sizeof(*addr);

	logx(d_len);


	addr = new sockaddr_in;
	addr->sin_port = htons(DEST_PORT);
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = INADDR_ANY;

	src = (sockaddr *) addr;
	s_len = sizeof(*addr);

	addr = new sockaddr_in;
	addr->sin_port = htons(SRC_PORT);
	addr->sin_family = AF_INET;

	_out = (sockaddr *) addr;
	o_len = sizeof(*addr);

	nameConfirmed = false;
	claimCount = 0;

	_workLoop = false; // becomes true on start()
	_readLoop = false;

	out_sock = socket(AF_INET, SOCK_DGRAM, 0);
	in_sock = socket(AF_INET, SOCK_DGRAM, 0);

	bc = new int(1);


	if (setsockopt(out_sock, SOL_SOCKET, SO_BROADCAST, &bc, sizeof(bc)) < 0) {
		logx("could not set sock reuse out");
	}

	if (setsockopt(in_sock, SOL_SOCKET, SO_BROADCAST | SO_REUSEADDR | SO_REUSEPORT, &bc, sizeof(bc)) < 0) {
		logx("could not set sock broadcast out");
	}

	if (out_sock < 0) {
		logx("create socket failed bitch");
	}

	if (in_sock < 0) {
		logx("create socket failed again bitch");
	}

	::bind(in_sock, src, s_len);
	//bind(out_sock, _out, o_len);

	in = new mqueue();
	out = new mqueue();

	externalMsgHandler = NULL;
	external_msg_target = NULL;
	hasExternalMsgHandler = false;

	ulist = new mqueue();
}

networklayer::~networklayer() {
	delete in;
	delete out;
	delete ulist;
	delete bc;
	delete dest;
	delete _since;
}

bool networklayer::since(nl::op verb, long time) {
	unsigned char index = static_cast<unsigned char>(verb);
	long now, dif;

	std::chrono::milliseconds ms = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::system_clock::now().time_since_epoch());

	now = ms.count();

	dif = now - _since[index];

	if (_since[index] == 0) {
		logx("initializing " + to_string(index));
		_since[index] = now;
		return true;
	} else {
		if (now - _since[index] > time) {
			_since[index] = now;
			return true;
		}
	}
	return false;
}

void networklayer::_write(nl::msg * msg) {
	char buf[2048];
	size_t size = msg->length + 12 + msg->sender.length() + msg->recipient.length();
	unsigned char t;
	size_t fill = 0, r_size = msg->recipient.length();

	if (size > 2048) {
		logx("will not send msg with invalid size");
	}

	if (!msg->recipient.empty()) {
		logx("sending to |" + msg->recipient + "|");
	}

	memcpy(buf, nl::sig, 8);
	fill += 8;

	buf[fill++] = msg->opcode;

	memcpy(buf + fill, &msg->length, 2);
	fill += 2;

	t = 0b00001111 & msg->sender.length();

	if (r_size > 0) {
		t |= r_size << 4;
	}

	buf[fill++] = t;

	memcpy(buf + fill, msg->sender.c_str(), msg->sender.length());
	fill += msg->sender.length();

	if (r_size > 0) {
		memcpy(buf + fill, msg->recipient.c_str(), r_size);
		fill += r_size;
	}

	if (msg->payload != NULL && msg->length > 0) {
		memcpy(buf + fill, msg->payload, msg->length);
		fill += msg->length;
	}

	sendto(out_sock, buf, fill, 0, dest, d_len);

	delete [] msg->payload;
	delete msg;
}

void networklayer::_read() {
	size_t r;

	nl::msg *msg;

	char buf[2048];
	unsigned short size;
	unsigned char s_size;
	unsigned char r_size;
	unsigned char opcode;

	size_t fill = 0;

	char* _tmp;

	r = recvfrom(in_sock, buf, 2048, 0, src, &s_len);

	//r = recvfrom(in_sock, buf, 2, 0, NULL, NULL);

	if (r < 12) {
		logx("dropping msg with impossible size");
		return;
	}

	if (memcmp(buf, nl::sig, 8) != 0) {
		logx("dropping msg with invalid sig");
		return;
	}

	fill += 8;

	opcode = buf[fill++];

	memcpy(&size, buf + fill, 2);

	fill += 2;

	s_size = buf[fill++];

	r_size = s_size >> 4;
	r_size &= 0b00001111;
	s_size &= 0b00001111;

	if (r_size > 14 || s_size > 14 || s_size < 3) {
		logx("dropping msg with impossible names");
		return;
	}

	if (size >= 2036) {
		logx("dropping impossibly large msg");
		return;
	}

	if (r < (size + s_size + r_size + 12)) {
		logx("dropping missized msg");
		return;
	}

	msg = new nl::msg;

	msg->length = size;
	msg->opcode = opcode;

	_tmp = new char[s_size];

	memcpy(_tmp, buf + fill, s_size);

	msg->sender = string(_tmp, s_size);
	msg->recipient = string();
	fill += s_size;

	delete [] _tmp;

	if (r_size > 0) {
		_tmp = new char[r_size];

		memcpy(_tmp, buf + fill, r_size);
		msg->recipient = string(_tmp, r_size);
		fill += r_size;

		delete [] _tmp;
	}

	_tmp = new char[size];
	memcpy(_tmp, buf + fill, size);

	msg->payload = _tmp;

	in_mtx.lock();

	in->addItem(msg);

	in_mtx.unlock();
}

void networklayer::handleInternal(nl::msg * msg) {
	nl::peer *peer;
	switch (static_cast<nl::op>(msg->opcode)) {
	case nl::op::POKE:
		u_mtx.lock();

		if (ulist->has<nl::peer, std::string>(msg->sender)) {
			peer = ulist->getBy<nl::peer, std::string>(msg->sender);
			peer->poke(now());
		} else {
			ulist->addItem((void *)new nl::peer(msg->sender, now()));
			logx("new user " + msg->sender);
		}

		u_mtx.unlock();

		break;
	case nl::op::CLAIM:
		if (nameConfirmed) {
			if (name == msg->sender) {
				send(static_cast<unsigned char>(nl::op::CONTEST), name, "", 0, NULL);
			}
		}
		break;
	case nl::op::CONTEST:
		if (!nameConfirmed) {
			logx("my name has been contested!");
			claimCount = 0;
			handle(generate(static_cast<unsigned char>(ctl::msg_type::contest), name, "", 0, NULL));
			name = "";
		} else {
			logx("my name has been contested but it's already confirmed");
		}
		break;
	default:
		logx("got a bad op");
		return;
	}
}

void networklayer::handle(nl::msg *msg) {
	if (msg->recipient.empty() || msg->recipient == name) {
		if (nl::isLayerOp(msg->opcode)) {
			handleInternal(msg);
		} else {
			if (hasExternalMsgHandler) {
				externalMsgHandler(external_msg_target, msg->opcode, msg->sender, msg->recipient, msg->length, msg->payload);
			} else {
				logx("dropped unhandled external msg");
			}
		}
	} else {
		logx("dropped a message not meant for me");
	}
	delete [] msg->payload;
	delete msg;
}

void networklayer::setExternalMsgHandler(nl::external_msg_handler handler, void* tgt = NULL) {
	externalMsgHandler = handler;
	hasExternalMsgHandler = true;
	external_msg_target = tgt;
}

void networklayer::setName(string name) {
	if (nameConfirmed) {
		handle(generate(static_cast<unsigned char>(ctl::msg_type::confirm), name, "", 0, NULL));
		return;
	}
}

nl::msg *networklayer::generate(unsigned char op, string sender, string recipient, unsigned short length, char* payload) {
	nl::msg *msg = new nl::msg;

	msg->opcode = op;
	msg->sender = sender;
	msg->recipient = recipient;
	msg->length = length;
	msg->payload = payload;

	return msg;
}

void networklayer::sendTo(unsigned char opcode, std::string recipient, unsigned short length, char* payload) {
	nl::msg * msg = generate(opcode, name, recipient, length, payload);
	out_mtx.lock();

	out->addItem((void *) msg);

	out_mtx.unlock();
}

void networklayer::send(unsigned char opcode, std::string sender, std::string recipient, unsigned short length, char* payload) {
	nl::msg * msg = generate(opcode, sender, recipient, length, payload);
	out_mtx.lock();

	out->addItem((void *) msg);

	out_mtx.unlock();
}

size_t networklayer::getActiveUserList(string *&list) {
	u_mtx.lock();

	size_t size = ulist->expAs<nl::peer, std::string>(list);

	u_mtx.unlock();

	return size;
}

void networklayer::pollUserList() {
	u_mtx.lock();

	ulist->iter();

	nl::peer *peer;

	while ((peer = ulist->iterNext<nl::peer>()) != NULL) {
		if (peer->expired(now(), 5000)) {
			logx("removing " + peer->name);
			if (ulist->remove<nl::peer>(*peer, false)) {
				logx("removed successfully");
			} else {
				logx("remove failed");
			}
		}
	}

	u_mtx.unlock();
}

void networklayer::workLoop() {
	logx("networklayer workLoop begin --");

	nl::msg *next;
	bool __run = true;

	while (__run) {
		if (since(nl::op::POLL, 7000)) {
			pollUserList();
		}
		if (nameConfirmed) {
			out_mtx.lock();

			if (out->hasNext()) {
				next = (nl::msg*)out->nextItem();
				_write(next);
			}

			out_mtx.unlock();

			if (since(nl::op::POKE, 900)) {
				send(static_cast<unsigned char>(nl::op::POKE), name, "", 0, NULL);
			}
		} else {
			if (!name.empty()) {
				if (since(nl::op::CLAIM, 700)) {
					next = generate(static_cast<unsigned char>(nl::op::CLAIM), name, "", 0, NULL);
					_write(next);
					claimCount++;
				}
				if (claimCount > 5) {
					next = generate(static_cast<unsigned char>(ctl::msg_type::confirm), name, "", 0, NULL);
					handle(next);
					nameConfirmed = true;
				}
			}
		}
		in_mtx.lock();

		if (in->hasNext()) {
			next = (nl::msg*)in->nextItem();
			handle(next);
		}

		in_mtx.unlock();

		x_mtx.lock();
		__run = _workLoop;
		x_mtx.unlock();

		this_thread::sleep_for(std::chrono::milliseconds(15));

	}

	logx("network layer workloop end --");
}

void networklayer::readLoop() {
	logx("networklayer readloop begin --");

	bool __run = true;

	while (__run) {
		_read();

		x_mtx.lock();
		__run = _readLoop;
		x_mtx.unlock();

		this_thread::sleep_for(std::chrono::milliseconds(15));
	}

	logx("networklayer readloop end --");
}

