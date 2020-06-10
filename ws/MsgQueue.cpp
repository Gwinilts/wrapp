/*
 * MsgQueue.cpp
 *
 *  Created on: 27 May 2020
 *      Author: gwinilts
 */

#include "MsgQueue.h"

MsgQueue::MsgQueue(bool genMsgFromSegments) {

	top = NULL;
	bottom = NULL;
	first = NULL;

	_link_count = 0;
	pushSegToQueue = genMsgFromSegments;
}

MsgQueue::~MsgQueue() {
	clear();
}

void MsgQueue::pushSegment(char *segment, size_t length, bool last) {
	pushSegment(segment, length, last, WS::OPCODE::CONT);
}

void MsgQueue::pushSegment(char *segment, size_t length, bool fin, WS::OPCODE op) {
	WS::link *last;

	if (first == NULL) {
		first = last = new WS::link;
		last->next = NULL;
		last->msg = new WS::msg;

		last->msg->data = segment;
		last->msg->length = length;
		last->msg->type = op;
	} else {
		while (last->next != NULL) {
			last = last->next;
		}

		last->next = new WS::link;
		last = last->next;
		last->next = NULL;

		last->msg = new WS::msg;
		last->msg->data = segment;
		last->msg->length = length;
		last->msg->type = op;
	}

	if (fin && pushSegToQueue) {
		exportSegMsg();
	}

}

void MsgQueue::exportSegMsg() {
	WS::link *seg;
	WS::msg msg;

	size_t x;

	if ((seg = first) == NULL) return;

	msg.length = 0;
	msg.type = first->msg->type;

	while (seg != NULL) {
		msg.length += seg->msg->length;
		seg = seg->next;
	}

	msg.data = new char[msg.length];
	x = 0;
	seg = first;

	while (seg != NULL) {
		first = seg;
		for (size_t i = 0; i < seg->msg->length; i++) {
			msg.data[x++] = seg->msg->data[i];
		}
		delete [] seg->msg->data;
		delete seg->msg;
		seg = seg->next;
		delete first;
	}

	first = NULL;

	addMsg(msg.type, msg.data, msg.length);
}

WS::frame MsgQueue::nextSegment() {
	WS::link *next;
	WS::msg *msg;
	WS::frame frame;

	next = first;

	if (first->next != NULL) {
		first = first->next;
	} else {
		first = NULL; // delete next when we're done please
	}

	msg = next->msg; // delete this too when done please

	if (next->next != NULL) {
		frame.fin = next->next->msg->type != WS::OPCODE::CONT;
	}

	frame.opcode = msg->type;
	frame.payload = msg->data;
	frame.length = msg->length;

	delete msg;
	delete next;

	return frame;
}

bool MsgQueue::hasNextSegment() {
	return first != NULL;
}

WS::msg MsgQueue::nextMsg() {
	WS::msg msg;
	WS::link *link = top;

	if (link == bottom) { // only one item in this little queue
		msg = *link->msg;
		bottom = top = NULL;
	} else {
		top = link->next;
		msg = *link->msg;
	}
	_link_count--;
	delete link;
	return msg;

}

bool MsgQueue::hasNext() {
	return _link_count > 0;
}

void MsgQueue::clear() {
	WS::link *link = top;

	while (link != NULL) {
		top = link;

		link = link->next;

		delete [] top->msg->data;
		delete top->msg;
		delete top;
	}

	link = first;

	while (link != NULL) {
		first = link;

		link = link->next;

		delete [] first->msg->data;
		delete first->msg;
		delete first;
	}

	_link_count = 0;

	top = bottom = first = NULL;
}

void MsgQueue::addMsg(WS::OPCODE op, char *data, size_t length) {
	WS::msg *msg = new WS::msg;
	WS::link *link = new WS::link;

	msg->type = op;
	msg->length = length;
	msg->data = data;

	link->msg = msg;
	link->next = NULL; // probably pointless assignment


	if (bottom == NULL) { // there are no items in this little queue
		top = bottom = link;
		_link_count = 1;
		return;
	}

	if (bottom == top) { // there is one item in this little queue
		top->next = link;
		bottom = link;
		_link_count = 2;
		return;
	}

	bottom->next = link;
	bottom = link;
	_link_count++;
}
