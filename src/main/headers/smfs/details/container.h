/*
 * container.h
 *
 *  Created on: 16 Apr 2016
 *      Author: rowan
 */

#ifndef SMFS_DETAILS_CONTAINER_H_
#define SMFS_DETAILS_CONTAINER_H_

#include <stdexcept>

namespace smfs {

template<typename M> class collection;

namespace details {

template<typename C, typename M>
class collection_wrapper_base : public smfs::collection<M> {
	using typename collection<M>::size_type;

protected:
	std::shared_ptr<C> const delegate;

public:

	collection_wrapper_base(C* delegate) : delegate(delegate) {}
	collection_wrapper_base(std::shared_ptr<C> const &delegate) : delegate(delegate) {}

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

template<typename C, typename M>
class contiguous_wrapper : collection_wrapper_base<C,M> {
	using typename collection<M>::size_type;
	using typename collection<M>::value_type;

public:

	contiguous_wrapper(std::shared_ptr<C> const delegate) : collection_wrapper_base(delegate) {}

	value_type & at(size_type index) override {
		return delegate->at(index);
	}

	value_type const & at(size_type index) const override {
		return std::forward<value_type>(delegate->at(index));
	}
};

template<typename C, typename M>
class linked_wrapper : collection_wrapper_base<C,M> {
	using typename collection<M>::size_type;
	using typename collection<M>::value_type;

public:

	linked_wrapper(std::shared_ptr<C> const delegate) : collection_wrapper_base(delegate) {}

#define IT_TO_INDEX(FN_BEGIN, FN_END) \
	for(auto it=FN_BEGIN(delegate); it!=FN_END(delegate); ++it) {\
		if(index--=0) return *it;\
	}\
	throw std::out_of_range("Index '" + index + "' out of range.")

	value_type & at(size_type index) override {
		IT_TO_INDEX(std::begin, std::end);
	}

	value_type const & at(size_type index) const override {
		IT_TO_INDEX(std::cbegin, std::cend);
	}
};

template<typename C, typename M> auto has_at(C* container)
		-> decltype(std::declval<C>().at(0)!=std::declval<M>(), std::true_type) {}

template<typename C, typename M> std::false_type has_at(void* container) {}

} //namespace details
} // namesoace smfs

#endif /* SMFS_DETAILS_CONTAINER_H_ */