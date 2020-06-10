/*
 * MsgQueue.h
 *
 *  Created on: 27 May 2020
 *      Author: gwinilts
 */

#ifndef WS_MSGQUEUE_H_
#define WS_MSGQUEUE_H_

#include "stdlib.h"

namespace WS {
	enum class OPCODE: unsigned char {
		CONT = 0x0, TEXT = 0x1, RAW = 0x2, PING = 0x9, PONG = 0xA, FATAL = 0xFF
	};

	struct frame {
		bool fin;
		OPCODE opcode;
		size_t length;
		char* payload;
	};

	struct msg {
		OPCODE type;
		size_t length;
		char* data;
	};

	struct link {
		struct msg *msg;
		link *next;
	};
}

class MsgQueue {
	WS::link *top;
	WS::link *bottom;

	WS::link *first;

	size_t _link_count;
	bool pushSegToQueue;

	void exportSegMsg();

public:
	MsgQueue(bool genMsgFromSegment); // if genMsgFromSegment is true, when a segment is pushed with last=true the list of segments will be turned into a message and pushed onto the end of the queue
	virtual ~MsgQueue();

	void pushSegment(char* segment, size_t length, bool last);
	void pushSegment(char* segment, size_t length, bool last, WS::OPCODE op);

	WS::frame nextSegment();
	bool hasNextSegment();

	WS::msg nextMsg(); // get topMostLink
	bool hasNext(); // has topMost link

	void clear();

	void addMsg(WS::OPCODE op, char* segment, size_t length); // append after bottom
};

#endif /* WS_MSGQUEUE_H_ */
