/*
 * RStorage.h
 *
 *  Created on: 12 Jun 2020
 *      Author: gwinilts
 */

#ifndef RSTORAGE_RSTORAGE_H_
#define RSTORAGE_RSTORAGE_H_
#include <fstream>

namespace rs {
	struct branch {
		char hash[20];
		unsigned long up;
		unsigned long down;
		unsigned long ref;
		unsigned long score;
		unsigned char last;
	};
}


class RStorage {
private:
	std::fstream strings;
	std::fstream tree;

public:
	RStorage();
	virtual ~RStorage();
};

#endif /* RSTORAGE_RSTORAGE_H_ */
