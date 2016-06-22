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
	using size_type = typename collection<M>::size_type;

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
class contiguous_wrapper : public collection_wrapper_base<C,M> {
	typedef typename collection<M>::size_type size_type;
	typedef typename collection<M>::value_type value_type;

public:

	contiguous_wrapper(std::shared_ptr<C> const &delegate) : collection_wrapper_base<C,M>(delegate) {}

	value_type & at(size_type index) override {
		return this->delegate->at(index);
	}

	value_type const & at(size_type index) const override {
		return std::forward<value_type>(this->delegate->at(index));
	}
};

template<typename C, typename M>
class linked_wrapper : collection_wrapper_base<C,M> {
	using typename collection<M>::size_type;
	using typename collection<M>::value_type;

public:

	linked_wrapper(std::shared_ptr<C> const delegate) : collection_wrapper_base<C,M>(delegate) {}

#define IT_TO_INDEX(FN_BEGIN, FN_END) \
	for(auto it=FN_BEGIN(this->delegate); it!=FN_END(this->delegate); ++it) {\
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

template<typename C, typename M>
class has_at {
	static constexpr auto test(C* container, M* member) -> decltype(container->at(0)!=*member, std::true_type()) { return std::true_type(); }
	static constexpr std::false_type test(...) { return std::false_type(); }

public:

	static bool const value = test(0,0)();
};



} //namespace details
} // namesoace smfs

#endif /* SMFS_DETAILS_CONTAINER_H_ */
