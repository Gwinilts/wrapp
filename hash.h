/*
 * hash.h
 *
 *  Created on: 12 Jun 2020
 *      Author: gwinilts
 */

#ifndef HASH_H_
#define HASH_H_

#include "sha1.hpp"
#include <string>
#include <cstring>

int getHash(char * &out, std::string what) {
	SHA1 c;
	std::string h;
	int size;

	c.update(what);

	h = c.final();

	size = h.length() / 2;

	out = new char[size];

	for (unsigned int i = 0, o = 0; i < h.length(); i += 2, o++) {
		out[o] = stoul(h.substr(i, 2), nullptr, 16);
	}

	return size;
}

int compareHash(char* mine, char* other, size_t length) {
	return memcmp(other, mine, length);
}



#endif /* HASH_H_ */
