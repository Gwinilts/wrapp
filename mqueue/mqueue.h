/*
 * mqueue.h
 *
 *  Created on: 6 Jun 2020
 *      Author: gwinilts
 */

#ifndef MQUEUE_H_
#define MQUEUE_H_

#include <stdlib.h>
#include <mutex>
#include "../gxplatform.h"

namespace mq {
	struct link {
		void* obj;
		link *next;

		~link() {

		}
	};
}

class mqueue {
private:
	mq::link *top;
	mq::link *bottom;

	size_t _link_count;
	mq::link *_itr;

public:
	mqueue();
	virtual ~mqueue();

	void addItem(void* item);

	bool hasNext();
	void *nextItem();

	void clear();

	void iter();

	template <class T> T* iterNext();

	template <class T> size_t exp(T*&);
	template <class T, class V> size_t expAs(V*&);
	template <class T> bool has(T it);
	template <class T, class V> bool has(V it);
	template <class T> T* get(T it);
	template <class T, class V> T* getBy(V it);
	template <class T> bool remove(T it, bool all, bool del = true);
	template <class T, class V> bool removeBy(V it, bool all, bool del = true);
	template <class T> void clear();
};

template <class T> T* mqueue::iterNext() {
	if (_itr == NULL) return NULL;

	T* obj = (T*)_itr->obj;

	_itr = _itr->next;

	return obj;
}

template <class T> void mqueue::clear() {
	mq::link *link = top;
	T* obj;

	while (link != NULL) {
		obj = (T*)link->obj;
		delete obj;

		top = link->next;
		delete link;
		link = top;
	}

	_link_count = 0;

	top = NULL;
	bottom = NULL;
}

template <class T, class V> size_t mqueue::expAs(V* &ex) {
	ex = new V[_link_count];
	mq::link *next = top;

	size_t i = 0;
	T* src;
	V obj;

	while (next != NULL) {
		src = (T*)next->obj;

		obj = static_cast<V>(*src);

		ex[i++] = obj;

		next = next->next;
	}

	return _link_count;
}

template <class T> size_t mqueue::exp(T* &ex) {
	ex = new T[_link_count];
	mq::link *next = top;

	size_t i = 0;

	while (next != NULL) {
		ex[i++] = *(new T((  *(T*)next->obj     ))); // i'm worried that if it's T() instead of new T() then the members could get deallocated before
		next = next->next;  						 // they actually get used.
	}

	return _link_count;
}

template <class T> T* mqueue::get(T it) {
	mq::link *next = top;

	T* obj;

	while (next != NULL) {
		obj = (T*) next->obj;

		if (*obj == it) return (T*)next->obj;

		next = next->next;
	}

	return NULL;
}

template <class T, class V> T* mqueue::getBy(V it) {
	mq::link *next = top;

	T* obj;

	while (next != NULL) {
		obj = (T*) next->obj;

		if (*obj == it) return (T*)next->obj;

		next = next->next;
	}

	return NULL;
}

template <class T> bool mqueue::remove(T it, bool all, bool del) {
	mq::link *next = top;
	mq::link *prev = NULL;

	bool success = false;

	T* obj;

	if (del) {
		logx("this del will delete obj");
	}

	while (next != NULL) {
		obj = (T*) next->obj;

		if (it == *obj) {
			if (del) delete obj;
			if (prev == NULL) {
				top = next->next;

				delete next;
				next = top;
			} else {
				prev->next = next->next;

				delete next;
				next = next->next;
			}

			_link_count--;

			if (all) {
				success = true;
			} else {
				return true;
			}
		} else {
			prev = next;
			next = next->next;
		}
	}

	return success;
}

template <class T, class V> bool mqueue::removeBy(V it, bool all, bool del) {
	mq::link *next = top;
	mq::link *prev = NULL;

	bool success = false;

	T* obj;

	while (next != NULL) {
		obj = (T*) next->obj;

		if (*obj == it) {
			if (del) delete obj;
			if (prev == NULL) {
				top = next->next;

				delete next;
				next = top;
			} else {
				prev->next = next->next;

				delete next;
				next = next->next;
			}

			_link_count--;

			if (all) {
				success = true;
			} else {
				return true;
			}
		} else {
			prev = next;
			next = next->next;
		}
	}

	return success;
}

template <class T> bool mqueue::has(T it) {
	T *comp;
	mq::link *next = top;

	while (next != NULL) {
		comp = (T*) next->obj;

		if (*comp == it) return true;

		next = next->next;
	}
	return false;
}

template <class T, class V> bool mqueue::has(V it) {
	T *comp;
	mq::link *next = top;

	while (next != NULL) {
		comp = (T*) next->obj;

		if (*comp == it) return true;
		next = next->next;
	}

	return false;
}


#endif /* MQUEUE_H_ */
