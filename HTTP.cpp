/*
 * HTTP.cpp
 *
 *  Created on: 24 May 2020
 *      Author: gwinilts
 */

#include "HTTP.h"
#include <cstring>

/*
 * ok so when http gets constructed it starts a read cycle on the socket, reading 2048 bytes at a time
 *
 *
 *
 */

namespace HT {
	const char *ok_line = "HTTP/1.1 200 OK\r\n";
	const char *nf_line = "HTTP/1.1 404 NOT-FOUND\r\n";
	const char *bad_line = "HTTP/1.1 400 BAD-REQUEST\r\n";
	const char *switch_line = "HTTP/1.1 101 SWITCHING-PROTOCOLS\r\n";
	const char *sp = ": ";
	const char *crlf = "\r\n";
}


HTTP::HTTP(int sock) {
	this->sock = sock;

	this->h_in_count = 0;
	this->h_out_count = 0;
	this->h_out_grow = 5;
	this->h_out_mult = 5;

	this->out = new HT::header[5];

	this->payload = {0};

	this->v = false;
	this->f = false;
	this->sread();
}

bool HTTP::valid(char * &line, int length) { // maybe length should be size_t? what if the req_uri length was 10x10^20 hur hur hur
	/**
	 * 1: get the method. we can assume that method length will be 3 or 4 (GET or POST)
	 * so if we cannot find a space in the first 5 char just reject the request.
	 *
	 * 2: get the HTTP version. We want HTTP/1.1. if the right most space is more then 9 char from the end we can reject the request.
	 * if it's not HTTP/1.1 we can say nada
	 *
	 * 4: read from the 5th or 6th char to length - 10? as the uri
	 */

	char _get[4] = {'G', 'E', 'T', ' '};
	char _post[5] = {'P', 'O', 'S', 'T', ' '};
	char _ver[9] = {' ', 'H', 'T', 'T', 'P', '/', '1', '.', '1'};

	size_t uri_len;
	size_t m;

	char *_buf;

	bool _v_get = true;
	bool _v_post = true;
	bool _v_ver = true;

	_v_post = memcmp(_post, line, 5) == 0;
	_v_get = memcmp(_get, line, 4) == 0;
	/*
	for (int i = 0; i < length && i <= 5; i++) {
		if (i < 4) _v_get &= (line[i] == _get[i]);
		_v_post &= (line[i] == _post[i]);
	}*/

	if (!(_v_get || _v_post)) return false;

	if (_v_get) this->method = HT::GET;
	if (_v_post) this->method = HT::POST;

	//cout << "valid method" << endl;

	if (length < 10) return false;

	_v_ver = memcmp(_ver, line + (length - 9), 9) == 0;

	/*
	for (int i = 0; i < 9; i++) {
		_v_ver &= (line[length - (i + 1)] == _ver[8 - i]);
	}*/

	if (!_v_ver) return false;

	//cout << "version ok" << endl;

	this->uri = "";

	m = _v_get ? 4 : 5;
	uri_len = length - 9 - m;

	_buf = new char[uri_len];
	memcpy(_buf, line + m, uri_len);
	/*
	for (int i = _v_get ? 4 : 5; i < length - 9; i++) {
		this->uri += line[i];
	}*/

	this->uri = string(_buf, uri_len);

	logx(this->uri);

	if (this->uri.length() > 2148) return false; // i'm not even expecting urls to be that long

	//cout << this->uri << endl;

	this->v = true;

	return true;
}

void HTTP::sread() {
	/**
	 * read in blocks of 2048
	 * if we read 8192 and don't find crlf+crlf then respond 400
	 */

	char buffer[2048], lval[1024], rval[1024];

	char *full = new char[8192];

	int fill = 0, r, last_crlf, div, h_end = -1, lvlen, rvlen;

	h_in_count = 0;

	//cout << "hello, i am steve and today i will read message." << endl;

	while (h_end < 0) {
		r = read(this->sock, buffer, sizeof(buffer));

		if (r > 0) {
			if ((fill + r) >= 8192) r = 8192 - r;

			memcpy(full + fill, buffer, r);
			/*
			for (int i = fill, o = 0; (o < r) && (i < 8192); i++, o++) {
				full[i] = buffer[o];
			}*/

			fill += r;
		}

		//cout << "oh cool, i read " << r << " bytes!" << endl;

		if (r > 20 && !this->v) { // try to find the request line and validate it
			for (int i = 0; i < r; i++) {
				if (full[i] == '\n' && i > 0) {
					if (full[i - 1] == '\r') {
						if (!this->valid(full, i - 1)) {
							delete [] full;
							this->naughty();
							return;
						} else {
							last_crlf = i;
							break;
						}
					}
				}
			}
		}

		if (this->v) {
			h_in_count = 0;
			for (int i = last_crlf + 1; i < fill; i++) {
				if (full[i] == '\n' && full[i - 1] == '\r') {
					if (full[i - 2] == '\n' && full[i - 3] == '\r') {
						h_end = i;
						break;
					} else {
						h_in_count++;
					}
				}
			}
		}

		if ((fill >= 8192) && ((h_end < 0) || (!this->v))) {
			delete [] full;
			this->naughty();
			return;
		}
	}

	this->in = new HT::header[h_in_count];

	r = 0;

	for (int i = last_crlf + 1; i < h_end - 1; i++) {
		if (full[i] == '\r' || full[i] == ':') {
			if (full[i + 1] == ' ') {
				div = i;
				i += 2;
			}
			if (full[i + 1] == '\n') {
				lvlen = (div - last_crlf - 1);
				rvlen = (i - div - 2);

				for (int o = last_crlf + 1, b = 0; (o < div) && (b < 1024) ; o++, b++) {
					lval[b] = toupper(full[o]);
				}

				for (int o = div + 2, b = 0; (o < i) && (b < 1024); o++, b++) {
					rval[b] = full[o];
				}

				this->in[r].name = string(lval, lvlen);
				this->in[r].value = string(rval, rvlen);

				//cout << this->in[r].name << ": " << this->in[r].value << endl;

				last_crlf = i + 1;
				i += 2;
				r++;
			}
		}
	}

	if (method == HT::POST) {
		if (hasHeader("content-length")) {
			try {
				payload_size = stoi(getHeader("content-length"));
				if (payload_size <= 8192) {
					payload = new char[payload_size];

					for (int i = h_end + 1, o = 0; (i < fill) && (o < payload_size); i++, o++) {
						payload[o] = full[i];
						div = o;
					}

					if (fill - (h_end + 1) < payload_size) {
						last_crlf = (payload_size - (fill - (h_end + 1))); // bytes remaining to be read
						fill = 0;

						while (fill < last_crlf) {
							r = read(sock, buffer, sizeof(buffer));

							if (r > 0) {
								for (int i = fill, o = 0; (o < r) && (i < 8192); i++, o++) {
									full[i] = buffer[o];
								}

								fill += r;
							} else {
								delete [] full;
								this->naughty();
								return;
							}
						}

						for (int i = 0, o = div + 1; (i < fill) && (o < payload_size); i++, o++) {
							payload[o] = full[i];
						}
					}
				} else {
					delete [] full;
					this->naughty();
					return;
				}
			} catch (...) {
				delete [] full;
				this->naughty();
				return;
			}
		}
	}

	delete [] full;
}

void HTTP::naughty() {
	this->v = true;
	setHeader("x-hacker", "you are a naughty client", false);
	respond(HT::FAILED, NULL, 0);
	delete [] in;
	delete [] out;
	delete [] payload;
	this->v = false;
	this->f = true;
}

bool HTTP::failed() {
	return this->f;
}

bool HTTP::hasHeader(string name) {
	if (!this->v) return false;
	for (char &c: name) c = toupper(c);

	for (size_t i = 0; i < this->h_in_count; i++) {
		if (this->in[i].name == name) return true;
	}

	return false;
}

string HTTP::getHeader(string name) {
	if (!this->v) return NULL;
	for (char &c: name) c = toupper(c);

	for (size_t i = 0; i < this->h_in_count; i++) {
		if (this->in[i].name == name) return this->in[i].value;
	}

	return NULL;
}

bool HTTP::headerEquals(string name, string value) {
	if (!this->v) return false;
	for (char &c: name) c = toupper(c);

	for (size_t i = 0; i < this->h_in_count; i++) {
		if (this->in[i].name == name) return this->in[i].value == value;
	}

	return false;
}

bool HTTP::headerPartialEquals(string name, string partial) {
	if (!this->v) return false;
	for (char &c: name) c = toupper(c);
	string t;

	for (size_t i = 0; i < this->h_in_count; i++) {
		if (this->in[i].name == name) {
			t = this->in[i].value;
			if (t.length() >= partial.length()) {
				for (size_t i = 0; i < t.length() - partial.length() + 1; i++) {
					if (t.compare(i, partial.length(), partial) == 0) return true;
				}
			}
		}
	}
	return false;
}

void HTTP::addHeader(const char *name, size_t nlen, const char* value, size_t vlen) {
	this->h_out_count++;
	HT::header *tmp;

	if (this->h_out_count == this->h_out_grow) {
		cout << "growing the array" << endl;
		tmp = this->out;
		this->h_out_grow += this->h_out_mult;

		this->out = new HT::header[this->h_out_grow];

		if (this->h_out_count > 0) {
			for (size_t i = 0; i < (this->h_out_count - 1); i++) {
				this->out[i] = tmp[i];
			}

			delete [] tmp;
		}
	}

	//cout << "set " << name << " to " << value << endl;

	//cout << this->h_out_count << endl;

	this->out[this->h_out_count - 1].name = string(name, nlen);
	this->out[this->h_out_count - 1].value = string(value, vlen);

	//cout << this->out[this->h_out_count - 1].name << endl;
	//cout << this->out[this->h_out_count - 1].value << endl;

	//cout << this->out[this->h_out_count - 1].name << endl;
}

void HTTP::setHeader(string name, string value, bool mult) {
	if (!this->v) return;
	for (char &c: name) c = toupper(c);

	if (!mult) {
		for (size_t i = 0; i < this->h_out_count; i++) {
			if (this->out[i].name == name) {
				this->out[i].value = value;
				return;
			}
		}
	}
	this->addHeader(name.c_str(), name.length(), value.c_str(), value.length());
}

int HTTP::genHead(char* &sbuf, HT::STATUS status, size_t c_len) {
	char const *buf;

	size_t size;
	int pos = 0;

	switch (status) {
	case HT::OK:
		size = strlen(HT::ok_line);
		buf = HT::ok_line;
		break;
	case HT::N_FOUND:
		size = strlen(HT::nf_line);
		buf = HT::nf_line;
		break;
	case HT::SWITCH:
		size = strlen(HT::switch_line);
		buf = HT::switch_line;
		break;
	default:
		size = strlen(HT::bad_line);
		buf = HT::bad_line;
		break;
	}

	if (c_len > 0) {
		this->setHeader("content-length", to_string(c_len), false);
	}

	for (size_t i = 0; i < this->h_out_count; i++) {
		size += this->out[i].name.length();
		size += this->out[i].value.length() + strlen(HT::sp) + strlen(HT::crlf);
	}

	size += strlen(HT::crlf);

	if (c_len > 0) size += c_len;

	sbuf = new char[size]; // NOTE: must be deleted by the calling fuction

	for (size_t i = 0; i < strlen(buf); i++) {
		sbuf[pos++] = buf[i];
	}

	for (size_t i = 0; i < this->h_out_count; i++) {
		for (char c: this->out[i].name) {
			sbuf[pos++] = c;
		}
		for (size_t i = 0; i < strlen(HT::sp); i++) {
			sbuf[pos++] = HT::sp[i];
		}
		for (char c: this->out[i].value) {
			sbuf[pos++] = c;
		}
		for (size_t i = 0; i < strlen(HT::crlf); i++) {
			sbuf[pos++] = HT::crlf[i];
		}
	}

	for (size_t i = 0; i < strlen(HT::crlf); i++) {
		sbuf[pos++] = HT::crlf[i];
	}

	//cout << strlen(sbuf) << endl;

	return pos;
}

void HTTP::respondFile(HT::STATUS status, G_FILE *fd) {
	if (!this->v) return;
	char buf[2048];
	char *head;
	int read;

	stringstream ss;
	string s;

	this->setHeader("transfer-encoding", "chunked", false);

	read = this->genHead(head, status, 0);

	send(this->sock, head, read, 0);

	delete [] head;


	while (read > 0) {
		read = g_fread(buf, sizeof(char), 2046, fd);

		buf[read] = '\r';
		buf[read + 1] = '\n';

		ss.str("");

		ss << std::hex << read << "\r\n";

		s = ss.str();

		send(this->sock, s.c_str(), s.length(), 0);
		send(this->sock, buf, read + 2, 0);
	}

	s = "\r\n";

	send(this->sock, HT::crlf, strlen(HT::crlf), 0);
}

void HTTP::respond(HT::STATUS status, char *data, size_t length) {
	if (!this->v) return;
	int pos = 0;
	char *buf;

	pos = this->genHead(buf, status, length);

	for (size_t i = 0; i < length; i++) {
		buf[pos++] = data[i];
	}

	send(this->sock, buf, pos, 0);

	delete [] buf;
	delete [] data;
}




HTTP::~HTTP() {
	// TODO Auto-generated destructor stub
	// this is gonna be fun hahahaha

	delete [] this->in;
	delete [] this->out;
	delete [] this->payload;
}

