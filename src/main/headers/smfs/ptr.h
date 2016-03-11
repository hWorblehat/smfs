/*
 * ptr.h
 *
 *  Created on: 9 Mar 2016
 *      Author: Rowan Lonsdale
 */

#ifndef SMFS_PTR_H_
#define SMFS_PTR_H_

#include <type_traits>
#include <memory>
#include <utility>

/**
 * A generalised type for any kind of smart pointer.
 */
template<typename T> class any_ptr {
	void* wrapped;

	any_ptr() = delete;

	any_ptr(void* toWrap) : wrapped(toWrap) {}

public:

	static any_ptr<T> wrap(std::shared_ptr<T>& toWrap) {
		return any_ptr(new std::shared_ptr<T>(toWrap));
	}

	static any_ptr<T> wrap(std::shared_ptr<T>&& toWrap) {
		return any_ptr(new std::shared_ptr<T>(std::forward(toWrap)));
	}


	/**
	 * Deletes the delegate pointer.
	 */
	~any_ptr() {
		delete(wrapped);
	}

	/**
	 * Dereferences this pointer.
	 * @return The object this pointer points to.
	 */
	typename std::add_lvalue_reference<T>::type operator*() const {
		return (typename std::add_lvalue_reference<T>::type) **wrapped;
	}

	T* operator->() const {
		return (T*) *wrapped;
	}

	/**
	 * Checks if this pointer owns an object.
	 * @return @code{true} if this pointer owns a non-null object.
	 */
	bool operator bool() const {
		return (bool) *wrapped;
	}
};

#endif /* SMFS_PTR_H_ */
