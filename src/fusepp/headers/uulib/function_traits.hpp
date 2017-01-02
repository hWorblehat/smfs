/*
 * function_traits.hpp
 *
 *  Created on: 29 Oct 2016
 *      Author: rowan
 */

#ifndef UULIB_FUNCTION_TRAITS_HPP_
#define UULIB_FUNCTION_TRAITS_HPP_

#include <cstddef>
#include <tuple>

namespace uulib {

/**
 * Gives access to the return type, arity, and argument types of a function.
 * @param <T> A function type or pointer-to-function type.
 */
template<typename T> class function_traits;

template<typename R, typename... Args>
class function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {};

template<typename R, typename... Args>
class function_traits<R(Args...)> {
public:

	/**
	 * The return type of the function.
	 */
	using return_type = R;

	/**
	 * The arity (number of arguments) of the function.
	 */
	static constexpr std::size_t arity = sizeof...(Args);

	/**
	 * A tuple type whose member type sequence matches the types of the function arguments.
	 */
	using args_tuple = std::tuple<Args...>;

	/**
	 * The typename signature of the function.
	 */
	using function_type = R(Args...);

	/**
	 * The type of a pointer to the function.
	 */
	using function_pointer_type = R(*)(Args...);

private:

	template<std::size_t I>
	struct arg {
		static_assert(I < arity, "error: invalid parameter index.");
		using type = typename std::tuple_element<I, args_tuple>::type;
	};

public:

	/**
	 * The type of the function's argument at the given index.
	 * @param <I> The index of the argument to get the type of.
	 */
	template<std::size_t I>
	using arg_t = arg<I>::type;

};

} // namespace uulib

#endif /* UULIB_FUNCTION_TRAITS_HPP_ */
