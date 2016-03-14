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

#include "smfs/details/ptr.h"

namespace smfs {

/**
 * A generalised type for any kind of smart pointer.
 */
template<typename T> class any_ptr {
details::any_ptr<T>* delegate;

public:

	any_ptr(std::shared_ptr<T>& toWrap)
		: delegate(new details::any_smart_ptr<T>(toWrap)) {}

	any_ptr(std::shared_ptr<T>&& toWrap)
		: delegate(new details::any_smart_ptr<T>(std::forward(toWrap))) {}


	/**
	 * Deletes the delegate pointer.
	 */
	~any_ptr() {
		delete(delegate);
	}

	T* operator->() const {
		return delegate->get();
	}

	/**
	 * Dereferences this pointer.
	 * @return The object this pointer points to.
	 */
	T operator*() const {
		return *(delegate->get());
	}

	/**
	 * Checks if this pointer owns an object.
	 * @return @code{true} if this pointer owns a non-null object.
	 */
	operator bool() const {
		return delegate && (bool) delegate->get();
	}
};

}

#endif /* SMFS_PTR_H_ */
