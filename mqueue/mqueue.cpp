/*
 * mqueue.cpp
 *
 *  Created on: 6 Jun 2020
 *      Author: gwinilts
 */

#include "mqueue.h"

mqueue::mqueue() {
	top = NULL;
	bottom = NULL;
	_itr = NULL;

	_link_count = 0;
}

mqueue::~mqueue() {
	// TODO Auto-generated destructor stub
}
bool mqueue::hasNext() {
	return _link_count > 0;
}

void* mqueue::nextItem() {
	mq::link *link = top;
	void* obj = top->obj;


	if (link == bottom) {
		bottom = top = NULL;
	} else {
		top = link->next;
	}
	_link_count--;
	delete link;

	return obj;
}

void mqueue::iter() {
	_itr = top;
}

void mqueue::addItem(void* item) {
	mq::link *link = new mq::link;

	link->obj = item;
	link->next = NULL;

	if (bottom == NULL) {
		top = bottom = link;
		_link_count = 1;
		return;
	}

	if (bottom == top) {
		top->next = link;
		bottom = link;
		_link_count = 2;
		return;
	}

	bottom->next = link;
	bottom = link;
	_link_count++;
}

void mqueue::clear() {
	mq::link *link = top;

	while (link != NULL) {
		top = link->next;

		delete link;
	}

	_link_count = 0;
	top = bottom = NULL;
}

