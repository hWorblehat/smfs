/*
 * container.h
 *
 *  Created on: 7 Apr 2016
 *      Author: Rowan Lonsdale
 */

#ifndef SMFS_CONTAINER_H_
#define SMFS_CONTAINER_H_

#include <memory>
#include <type_traits>
#include "smfs/details/container.h"

namespace smfs {

	/**
	 * An indexable collection of members (such as a @ref[std::list] or @ref[std::vector]).
	 */
	template<typename M>
	class collection {

	public:
		/**
		 * The type of members of this collection.
		 */
		typedef M value_type;

		/**
		 * The type used to describe the size of this collection.
		 */
		typedef size_t size_type;

		/**
		 * @return The number of elements in this collection.
		 */
		virtual size_type size() const = 0;

		/**
		 * @return @code{true} iff this collection contains no elements.
		 */
		virtual bool empty() const = 0;

		/**
		 * @return The maximum number of elements this collection can hold.
		 */
		virtual size_type max_size() const = 0;

		/**
		 * Gets a reference to the member at the given index.
		 * @param index The index of the member to retrieve.
		 * @return The requested member.
		 */
		virtual value_type & at(size_type index) = 0;

		/**
		 * Gets a const reference to the member at the given index.
		 * @param index The index of the member to retrieve.
		 * @return The requested member, unmodifiable.
		 */
		virtual value_type const & at(size_type index) const = 0;

		/**
		 * Gets a reference to the member at the given index.
		 * @param index The index of the member to retrieve.
		 * @return The requested member.
		 */
		value_type & operator[](size_type index) {
			return at(index);
		}

		/**
		 * Gets a const reference to the member at the given index.
		 * @param index The index of the member to retrieve.
		 * @return The requested member, unmodifiable.
		 */
		value_type const & operator[](size_type index) const {
			return std::forward<value_type>(at(index));
		}

		/**
		 * Destroys this collection.
		 */
		virtual ~collection(){}
	};

	/**
	 * Creates a new collection that warps the given collection object.
	 * @param toWrap An r-value reference to a std::shared_ptr to an object that obeys the semantics of a sequence container.
	 * @return A pointer to a collection wrapping the given object.
	 */
	template<typename M, typename C>
	std::enable_if_t<details::has_at<C,M>(), collection<M>*> wrap_collection(std::shared_ptr<C> &&toWrap) {
		return new smfs::details::contiguous_wrapper<C,M>(std::forward<std::shared_ptr<C>>(toWrap));
	}

	/**
	 * Creates a new collection that warps the given collection object.
	 * @param toWrap A reference to a std::shared_ptr to an object that obeys the semantics of a sequence container.
	 * @return A pointer to a collection wrapping the given object.
	 */
	template<typename M, typename C>
	std::enable_if_t<details::has_at<C,M>(), collection<M>*> wrap_collection(std::shared_ptr<C> &toWrap) {
		return new smfs::details::contiguous_wrapper<C,M>(std::shared_ptr<C>(toWrap));
	}

	/**
	 * Creates a new collection that warps the given collection object.
	 * @param toWrap An r-value reference to a std::shared_ptr to an object that obeys the semantics of a sequence container.
	 * @return A pointer to a collection wrapping the given object.
	 */
	template<typename M, typename C>
	std::enable_if_t<!details::has_at<C,M>(), collection<M>*> wrap_collection(std::shared_ptr<C> &&toWrap) {
		return new smfs::details::linked_wrapper<C,M>(std::forward<std::shared_ptr<C>>(toWrap));
	}

	/**
	 * Creates a new collection that warps the given collection object.
	 * @param toWrap A reference to a std::shared_ptr to an object that obeys the semantics of a sequence container.
	 * @return A pointer to a collection wrapping the given object.
	 */
	template<typename M, typename C>
	std::enable_if_t<!details::has_at<C,M>(), collection<M>*> wrap_collection(std::shared_ptr<C> &toWrap) {
		return new smfs::details::linked_wrapper<C,M>(std::shared_ptr<C>(toWrap));
	}

}

#endif /* SMFS_CONTAINER_H_ */
