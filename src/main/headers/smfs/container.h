/*
 * container.h
 *
 *  Created on: 7 Apr 2016
 *      Author: Rowan Lonsdale
 */

#ifndef SMFS_CONTAINER_H_
#define SMFS_CONTAINER_H_

#include <memory>

namespace smfs {

template<typename M>
class collection {

public:
	typedef M value_type;
	typedef size_t size_type;

	virtual size_type size() const = 0;
	virtual bool empty() const = 0;
	virtual size_type max_size() const = 0;

	virtual ~collection(){}
};

template<typename C, typename M>
class collection_wrapper : public collection<M> {
	std::shared_ptr<C> const delegate;

	using typename collection<M>::size_type;

public:

	collection_wrapper(std::shared_ptr<C> const &delegate) : delegate(delegate) {
	}

	size_type size() const override  {
		return delegate->size();
	}

	bool empty() const override {
		return delegate->empty();
	}

	size_type max_size() const override {
		return delegate->max_size();
	}
};

}

#endif /* SMFS_CONTAINER_H_ */
