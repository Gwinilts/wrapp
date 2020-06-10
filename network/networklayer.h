/*
 * networklayer.h
 *
 *  Created on: 4 Jun 2020
 *      Author: gwinilts
 */

#ifndef NETWORK_NETWORKLAYER_H_
#define NETWORK_NETWORKLAYER_H_

#include "../ClientThread.h"
#include "../mqueue/mqueue.h"
#include <string>
#include <thread>

#define DEST_PORT 32168
#define SRC_PORT 32169

namespace nl {
	const char sig[8] = {0xb, 0xe, 0xe, 0xf, 0xc, 0xa, 0xc, 0xa};

	enum class op: unsigned char {
		POKE = 0x01,
		CLAIM = 0x02,
		CONTEST = 0x03,
		POLL = 0x04 // only here for _since(what, time) (kinda like enum map in java) should never come down the wire
	};

	const char op_max = 0x04;
	const char op_min = 0x01;

	struct msg {
		std::string sender;
		std::string recipient;
		unsigned char opcode;
		char* payload;
		unsigned short length;
	};

	inline bool isLayerOp(unsigned char v) {
		if (v < static_cast<unsigned char>(op_min)) return false;
		if (v > static_cast<unsigned char>(op_max)) return false;
		return true;
	}

	class peer {
	public:
		std::string name;
		long time;
		peer(std::string name, long time) {
			this->name = name;
			this->time = time;
		};

		bool operator==(peer p) {
			return p.name == name;
		};

		bool operator==(std::string name) {
			return this->name == name;
		}

		operator std::string() {
			return string(this->name);
		}

		void poke(long time) {
			this->time = time;
		}

		bool expired(long now, long expiry) {
			return (now - this->time) > expiry;
		}
	};

	typedef void (*external_msg_handler) (void *, unsigned char, string, string, unsigned short int, char*);
}


class networklayer {
private:

	std::string name;

	std::mutex in_mtx;
	mqueue *in;
	std::mutex out_mtx;
	mqueue *out;

	mqueue *ulist;
	std::mutex u_mtx;

	long *_since;
	bool since(nl::op verb, long);

	int out_sock;
	int in_sock;

	bool _readLoop;
	std::thread __read;

	bool _workLoop;
	std::thread __work;

	std::mutex x_mtx;

	bool nameConfirmed;
	unsigned char claimCount;

	bool hasExternalMsgHandler;
	nl::external_msg_handler externalMsgHandler;
	void * external_msg_target;

	struct sockaddr *dest;
	socklen_t d_len;

	int * bc;

	struct sockaddr *src;
	socklen_t s_len;

	struct sockaddr *_out;
	socklen_t o_len;

	void readLoop();
	void _read();
	void _write(nl::msg *);

	void handle(nl::msg *);
	void handleInternal(nl::msg *);

	void pollUserList();
	long now();

	nl::msg *generate(unsigned char, string, string, unsigned short, char* payload);

	void workLoop();
public:
	networklayer(std::string, std::string);
	virtual ~networklayer();

	void send(unsigned char, string, string, unsigned short, char*);
	void sendTo(unsigned char, string, unsigned short, char*);
	void setExternalMsgHandler(nl::external_msg_handler, void*);

	void setName(string name);

	size_t getActiveUserList(string *&list);

	void start();
	void stop();
};

#endif /* NETWORK_NETWORKLAYER_H_ */
