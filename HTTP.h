/*
 * HTTP.h
 *
 *  Created on: 24 May 2020
 *      Author: gwinilts
 */


#include <string>
#include <string.h>
#include <thread>
#include <mutex>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include "./sha1.hpp"
#include "./base64/base64.h"
#include "./gxplatform.h"

#ifndef HTTP_H_
#define HTTP_H_

using namespace std;

namespace HT {
	enum METHOD {GET, POST};
	enum STATUS {OK, N_FOUND, FAILED, SWITCH};
	struct header {
		string name;
		string value;
	};

	inline bool is_big_endian(void) {
		union {
			uint32_t i;
			char c[4];
		} bint = {0x01020304};

		return bint.c[0] == 1;
	}

	inline string getMagic(string key) {
		string cat = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

		SHA1 c;

		c.update(cat);

		cat = c.final();

		char *hexed = new char[cat.length() / 2];

		for (int i = 0, o = 0; i < cat.length(); i += 2, o++) {
			hexed[o] = stoul(cat.substr(i, 2), nullptr, 16);
		}

		cat = base64_encode(reinterpret_cast<const unsigned char *>(hexed), cat.length() / 2, false);

		delete [] hexed;

		return cat;
	}
}

/*
 * general note:
 *
 * when getting constructed, HTTP figures out if the connection is valid http
 * if it isn't then all member methods will do nothing and return a default (null, false, etc)
 */

class HTTP {
private:

	int sock;
	mutex m_buf; // useless

	HT::header* in;
	HT::header* out;
	size_t h_in_count;

	size_t h_out_count;
	size_t h_out_grow;
	size_t h_out_mult;

	void sread(); // ok

	bool valid(char * &line, int length); // ok
	bool v;
	bool f;
	void naughty(); // todo if the request must immediately be rejected


	void addHeader(const char* name, size_t nlen, const char* value, size_t vlen);
	int genHead(char* &sbuf, HT::STATUS, size_t);


public:
	HTTP(int sock); // ok
	virtual ~HTTP(); // todo

	string getHeader(string name); // ok
	bool hasHeader(string name); // ok
	bool headerPartialEquals(string name, string some_partial_value); // ok
	bool headerEquals(string name, string some_value); // ok
	bool failed();

	void setHeader(string name, string value, bool mult); // for response

	char* payload;
	int payload_size;
	HT::METHOD method;


	string uri;

	void respond(HT::STATUS s, char* data, size_t length);
	void respondFile(HT::STATUS s, G_FILE *fd);

};

#endif /* HTTP_H_ */
