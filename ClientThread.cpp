/*
 * ClientThread.cpp
 *
 *  Created on: 26 May 2020
 *      Author: gwinilts
 */

#include "ClientThread.h"

ClientThread::ClientThread(int sock, void *ast, string bcast, binman *bin) {
	this->sock = sock;
	this->ast = ast;
	this->ws = NULL;
	this->bcast = bcast;
	this->layer = NULL;
	this->chess = NULL;
	this->dying = false;
	this->bin = bin;
	this->bid = bin->reg(this);

}

ClientThread::~ClientThread() {
	delete ws;
	delete layer;
	delete chess;
	//logx("client thread destructor");
}

void ClientThread::start() {
	//logx("starting client thread...");
	running = true;
	this->me = std::thread(&ClientThread::run, this);
}

void ClientThread::stop() {
	//logx("stopping");
	x_mtx.lock();
	this->running = false;
	x_mtx.unlock();

	this->me.join();

	//logx("all finished");
}

void ClientThread::fail(HTTP *h) {
	h->setHeader("content-type", "text/html; charset=UTF-8", false);
	h->setHeader("connection", "close", false);
	h->respond(HT::FAILED, NULL, 0);
	::shutdown(sock, SHUT_RDWR);
	delete h;
	this->shutdown();
}

void ClientThread::wsCallback(void* ref, WS::OPCODE op, size_t length, char *data) {
	ClientThread *obj = (ClientThread*) ref;

	obj->wsMsgHandler(op, length, data);
}

void ClientThread::nlCallback(void* ref, unsigned char opcode, string sender, string recipient, unsigned short length, char* payload) {
	ClientThread *obj = (ClientThread*) ref;

	obj->nlMsgHandler(opcode, sender, recipient, length, payload);
}

void ClientThread::nlMsgHandler(unsigned char opcode, string sender, string recipient, unsigned short length, char* payload) {
	char* dmp;
	size_t x;
	if (ctl::isMsgType(opcode)) {
		switch (static_cast<ctl::msg_type>(opcode)) {
		case ctl::msg_type::confirm:
			logx("nl says we should tell client their name is confirmed.");
			ws->sendCtl(opcode, length, payload);
			break;
		case ctl::msg_type::contest:
			logx("nl says we should tell client their name is taken");
			ws->sendCtl(opcode, length, payload);
			break;
		default:
			logx("msg_type " + to_string(opcode) + " is unhandled");
		}
	}
	if (chs::isMsgType(opcode)) {
		if (chess != NULL) {
			chess->handleNetworkMsg(static_cast<chs::msg_type>(opcode), sender, length, payload);
		}
	}

	//delete [] payload;
}

void ClientThread::wsGetResource(size_t length, char *data) {
	string name, mime;
	size_t i, div = -1;
	G_FILE *file;
	char *buf, *msg;

	mime = "";

	for (i = 1; i < length; i++) {
		if (data[i] == ';') {
			div = i;
			break;
		} else {
			mime += data[i];
		}
	}

	if (div < 0) return;


	name = "";

	for (i = div + 1; i < length; i++) {
		name += data[i];
	}

	if (!g_file_exists(ast, name, G_RSC_PREFIX)) {
		logx("cant find " + name);
		return;
	}

	file = g_file_open(ast, name, G_RSC_PREFIX);

	div = g_total_read(file, buf);

	i = wsGenRscMsg(msg, name, mime, base64_encode(reinterpret_cast<const unsigned char*>(buf), div, false));

	delete [] buf;

	ws->sendMessage(true, i, msg);
	g_file_close(file);

}

int ClientThread::wsGenRscMsg(char * &buf, string name, string mime, string data) {
	string tx = "{\"type\": \"rsc\", \"name\": \"\", \"data\": \"data:;charset=UTF-8;base64,\"}";
	size_t pos = 0;
	size_t _d = tx.length() + name.length() + mime.length() + data.length();


	buf = new char[_d];

	for (char c: "{\"type\": \"rsc\", \"name\": \"") {
		buf[pos++] = c;
	}

	pos--;

	for (char c: name) {
		buf[pos++] = c;
	}

	for (char c: "\", \"data\": \"data:") {
		buf[pos++] = c;
	}

	pos--;

	for (char c: mime) {
		buf[pos++] = c;
	}

	for (char c: ";charset=UTF-8;base64,") {
		buf[pos++] = c;
	}

	pos--;

	for (char c: data) {
		buf[pos++] = c;
	}

	for (char c: "\"}") {
		buf[pos++] = c;
	}

	return pos - 1;
}

void ClientThread::internalMsgHandler(ctl::msg_type type, size_t length, char *payload) {
	switch (type) {
	case ctl::msg_type::rsc:
		wsGetResource(length, payload);
		break;
	case ctl::msg_type::begin:
		startNetworkLayer(length, payload);
		break;
	case ctl::msg_type::upoll:
		nlUserPoll(length, payload);
		break;
	default:
		logx("internal msg has an unhandled msg type.");
	}
}

void ClientThread::startNetworkLayer(size_t length, char *data) {
	if (layer == NULL) {
		logx("client asked me to start networking with name", string(data, length));
		layer = new networklayer(string(data+1, length - 1), bcast);
		layer->setExternalMsgHandler(ClientThread::nlCallback, (void*)this);
		layer->start();

		chess = new chessgame(ws, layer);
	} else {
		layer->setName(string(data + 1, length - 1));
		logx("host asked me to start networking but i'm already networking.");
	}
}

void ClientThread::nlUserPoll(size_t length, char* payload) {
	if (layer == NULL) {
		logx("client asked me for a user poll but networking has not started yet");
		return;
	}

	stringstream json;

	string *list;
	string it;
	size_t size = layer->getActiveUserList(list);

	json << "{\"type\": \"user-list\", \"data\": [";

	for (size_t i = 0; i < size; i++) {
		json << "\"" << list[i] << "\"";
		if (i < size - 1) json << ", ";
	}

	json << "]}";

	it = json.str();

	char* data = new char[it.length()];

	memcpy(data, it.c_str(), it.length());
	ws->sendMessage(true, it.length(), data);
}



void ClientThread::wsMsgHandler(WS::OPCODE op, size_t length, char * data) {
	unsigned char type;

	if (op == WS::OPCODE::FATAL) {
		logx("websocket has encountered a fatal error.");
		dying = true;
	}

	if (length > 0) {
		if (op == WS::OPCODE::RAW) {
			type = data[0];

			if (ctl::isMsgType(type)) {
				internalMsgHandler(static_cast<ctl::msg_type>(type), length, data);
			} else if (chs::isMsgType(type)) {
				if (chess != NULL) {
					chess->handleClientMsg(static_cast<chs::msg_type>(type), length, data);
				}
			} else {
				logx("bad type " + to_string(type));
			}
		} else {
			logx("i don't know what to do with text msgs yet.");
		}
	} else {
		logx("msg had no payload");
	}
	delete [] data;
}

void ClientThread::shutdown() {
	logx("closing everything down.");

	if (ws != NULL) {
		logx("stopping websocket");
		ws->close();
	}

	if (chess != NULL) {
		logx("stopping chess");
		chess->stop();
	} else {
		//logx("chess didn't exist");
	}

	if (layer != NULL) {
		logx("stopping network layer");
		layer->stop();
	} else {
		//logx("networklayer didn't exist");
	}

	x_mtx.lock();
	running = false;
	x_mtx.unlock();

	logx("shutdown finished!");
	bin->finished(bid);
}

void ClientThread::masterWorkLoop() {
	bool __run = true;

	while (__run) {
		if (dying) {
			shutdown();
		} else {
			if (chess != NULL) chess->work();
			std::this_thread::sleep_for(20ms);
		}
		x_mtx.lock();
		__run = running;
		x_mtx.unlock();
	}

	logx("masterWorkLoop end --");
}

void ClientThread::run() {
	HTTP *h;
	G_FILE *file = NULL;

	string v;
	SHA1 s;

	h = new HTTP(this->sock);

	// when ever we make a HTTP we should immediately check if it failed and close the socket if it did.

	if (h->failed()) {
		close(sock);
		delete h;
		return;
	}

	if (h->method == HT::GET) {
		if (h->hasHeader("sec-websocket-version") && h->hasHeader("Upgrade") && h->hasHeader("sec-websocket-key")) {
			logx("oferred websocket upgrade");
			if (h->headerEquals("sec-websocket-version", "13") && h->headerEquals("upgrade", "websocket")) {
				h->setHeader("connection", "Upgrade", false);
				h->setHeader("upgrade", "websocket", false);

				logx(h->getHeader("sec-websocket-key").c_str(), NULL);

				v = HT::getMagic(h->getHeader("sec-websocket-key"));

                logx(v.c_str(), NULL);
				h->setHeader("sec-websocket-accept", v, false);
				h->respond(HT::SWITCH, nullptr, 0);

				logx("valid websocket");

				delete h;

				ws = new websocket(sock);
				ws->onMessage(&ClientThread::wsCallback, (void*)this);
				ws->open();

				logx("websocket is now open");

				masterWorkLoop();
				return;
			} else {
				logx("something very wrong here");

				h->setHeader("sec-websocket-version", "13", false);
				return fail(h);
			}
		} else {
			logx(h->uri.c_str(), NULL);

			if (h->uri == "/") {
				file = g_file_open(ast, "main.html");
				h->setHeader("content-type", "text/html; charset=UTF-8", false);
			} else if (h->uri == "/script") {
				file = g_file_open(ast, "script.js");
				h->setHeader("content-type", "text/javascript; charset=UTF-8", false);
			} else if (h->uri == "/lib") {
				file = g_file_open(ast, "lib.js");
				h->setHeader("content-type", "text/javascript; charset=UTF-8", false);
			} else if (h->uri == "/style") {
				file = g_file_open(ast, "style.css");
				h->setHeader("content-type", "text/css; charset=UTF-8", false);
			} else {
				return fail(h);
			}

			if (file != NULL) {
				h->setHeader("connection", "close", false);

				h->respondFile(HT::OK, file);

				g_file_close(file);
			}
		}
	} else {
		return fail(h);
	}

	::shutdown(sock, SHUT_RDWR);
	delete h;
	shutdown();
}

