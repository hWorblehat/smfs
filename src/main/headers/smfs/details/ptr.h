/*
 * ptr.h
 *
 *  Created on: 14 Mar 2016
 *      Author: rowan
 */

#ifndef SMFS_DETAILS_PTR_H_
#define SMFS_DETAILS_PTR_H_

#include <memory>
#include <utility>

namespace smfs {

template<typename T> class any_ptr;

namespace details {

template<typename T> class any_ptr {

	friend class smfs::any_ptr<T>;

protected:
	virtual T* get() const = 0;

public:
	virtual ~any_ptr(){}

};

template<typename T> class any_smart_ptr : any_ptr<T> {
	std::shared_ptr<T> delegate;

	friend class smfs::any_ptr<T>;

	any_smart_ptr(std::shared_ptr<T>& delegate) : delegate(delegate) {}
	any_smart_ptr(std::shared_ptr<T>&& delegate) : delegate(std::forward(delegate)) {}

protected:
	T* get() const override {
		return delegate.get();
	}
};

}

}

#endif /* SMFS_DETAILS_PTR_H_ */
