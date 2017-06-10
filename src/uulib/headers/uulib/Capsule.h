/*
 * Any.h
 *
 *  Created on: 12 Apr 2017
 *      Author: rowan
 */

#ifndef UULIB_CAPSULE_H_
#define UULIB_CAPSULE_H_

#include <ostream>

#include "internal/capsule.h"

namespace uulib {

using namespace uulib::internal::capsule;

template<typename I, size_t S>
class Capsule {

	UnionStorage<S> storage;
	Accessor const * accessor;

public:

	static constexpr std::size_t static_limit = UnionStorage<S>::staticSize;

	~Capsule() {
		accessor->destroy(storage);
	}

	Capsule() = delete;

	template<
		typename T,
		typename = enable_if_base_t<I, stripped<T>>
	>
	Capsule(T val) :
			storage(typeSize<T>()), accessor(AccessorImpl<T>::instance()) {
		new (storage.data()) stripped<T>(std::forward<T>(val));
	}

	Capsule(Capsule<I, S> const & other) :
			storage(other.accessor->size), accessor(other.accessor) {
		accessor->copy(other.storage, storage);
	}

	Capsule(Capsule<I, S> && other) :
			storage(std::move(other.storage), other.accessor),
			accessor(other.accessor) {}

	I const * get() const {
		return static_cast<I const *>(storage.data());
	}

	I* get() {
		return static_cast<I*>(storage.data());
	}

	I const & operator*() const {
		return *get();
	}

	I& operator*() {
		return *get();
	}

	I const * const operator->() const {
		return get();
	}

	I* const operator->() {
		return get();
	}

	bool operator==(Capsule<I, S> const &other) const {
		if(this==&other) {
			return true;
		}
		return *get()==*other.get();
	}

	bool operator!=(Capsule<I, S> const &other) const {
		return !operator==(other);
	}

	Capsule<I, S>& operator=(Capsule<I, S> const & other) {
		if(this!=&other && (accessor!=other.accessor || !accessor->assign(other.storage, storage))) {
			accessor->destroy(storage);
			accessor = other.accessor;
			storage.reserve(accessor->size);
			accessor->copy(other.storage, storage);
		}
		return *this;
	}

	Capsule<I, S>& operator=(Capsule<I, S> && other) {
		if(this!=&other) {
			moveAssign(other.storage, storage, other.accessor, accessor);
		}
		return *this;
	}

};

template<typename I, std::size_t S>
std::ostream& operator<<(std::ostream& os, Capsule<I, S> const & any) {
	return os << *any;
}

template <typename I, typename T>
using CapsuleBigEnough = enable_if_base_t<I, T, Capsule<I, sizeof(T)>>;

} //namespace uulib

#endif /* UULIB_CAPSULE_H_ */
