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

	template<typename M>
	class collection {

	public:
		typedef M value_type;
		typedef size_t size_type;

		virtual size_type size() const = 0;
		virtual bool empty() const = 0;
		virtual size_type max_size() const = 0;

		virtual value_type & at(size_type index) = 0;
		virtual value_type const & at(size_type index) const = 0;

		value_type & operator[](size_type index) {
			return at(index);
		}

		value_type const & operator[](size_type index) const {
			return std::forward<value_type>(at(index));
		}

		virtual ~collection(){}
	};

	template<typename M, typename C>
	std::enable_if_t<details::has_at<C,M>::value, collection<M>*> wrap_collection(std::shared_ptr<C> &toWrap) {
		return new smfs::details::contiguous_wrapper<C,M>(toWrap);
	}

	template<typename M, typename C>
	std::enable_if_t<!details::has_at<C,M>::value, collection<M>*> wrap_collection(std::shared_ptr<C> &toWrap) {
		return new smfs::details::linked_wrapper<C,M>(toWrap);
	}

}

#endif /* SMFS_CONTAINER_H_ */
