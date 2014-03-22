
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "react/Defs.h"

#include <assert.h>
#include <cstddef>
#include <ctime>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

/*********************************/ REACT_IMPL_BEGIN /*********************************/

////////////////////////////////////////////////////////////////////////////////////////
/// Unpack tuple - see
/// http://stackoverflow.com/questions/687490/how-do-i-expand-a-tuple-into-variadic-template-functions-arguments
////////////////////////////////////////////////////////////////////////////////////////

template<size_t N>
struct Apply {
    template<typename F, typename T, typename... A>
    static inline auto apply(F && f, T && t, A &&... a)
        -> decltype(Apply<N-1>::apply(std::forward<F>(f), std::forward<T>(t), std::get<N-1>(std::forward<T>(t)), std::forward<A>(a)...))
    {
        return Apply<N-1>::apply(std::forward<F>(f), std::forward<T>(t), std::get<N-1>(std::forward<T>(t)), std::forward<A>(a)...);
    }
};

template<>
struct Apply<0>
{
    template<typename F, typename T, typename... A>
    static inline auto apply(F && f, T &&, A &&... a)
        -> decltype(std::forward<F>(f)(std::forward<A>(a)...))
    {
        return std::forward<F>(f)(std::forward<A>(a)...);
    }
};

template<typename F, typename T>
inline auto apply(F && f, T && t)
	-> decltype(Apply<std::tuple_size<typename std::decay<T>::type>::value>::apply(std::forward<F>(f), std::forward<T>(t)))
{
    return Apply<std::tuple_size<typename std::decay<T>::type>::value>
		::apply(std::forward<F>(f), std::forward<T>(t));
}

////////////////////////////////////////////////////////////////////////////////////////
/// Helper to enable calling a function on each element of an argument pack.
/// We can't do f(args) ...; because ... expands with a comma.
/// But we can do nop_func(f(args) ...);
////////////////////////////////////////////////////////////////////////////////////////
template <typename... TArgs>
inline void pass(TArgs&& ...) {}

////////////////////////////////////////////////////////////////////////////////////////
/// Format print bits
////////////////////////////////////////////////////////////////////////////////////////
void PrintBits(size_t const size, void const* const p);

////////////////////////////////////////////////////////////////////////////////////////
// Identity (workaround to enable enable decltype()::X)
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct Identity
{
	using Type = T;
};

////////////////////////////////////////////////////////////////////////////////////////
// Generic RAII scope guard
////////////////////////////////////////////////////////////////////////////////////////
template <typename F>
class ScopeGuard
{
public:
	explicit ScopeGuard(F func) :
		func_{ std::move(func) }
	{
	}

	~ScopeGuard() { func_(); }

	ScopeGuard() = delete;
	ScopeGuard(const ScopeGuard&) = delete;
	ScopeGuard& operator=(const ScopeGuard&) = delete;

private:
	F	func_;
};

enum class ScopeGuardDummy_ {};

template <typename F>
ScopeGuard<F> operator+(ScopeGuardDummy_, F&& func)
{
	return ScopeGuard<F>(std::forward<F>(func));
}

/**********************************/ REACT_IMPL_END /**********************************/

// Expand args by wrapping them in a dummy function
// Use comma operator to replace potential void return value with 0
#define REACT_EXPAND_PACK(...) REACT_IMPL::pass((__VA_ARGS__ , 0) ...)

// Concat
#define REACT_CONCAT_(l, r) l##r
#define REACT_CONCAT(l, r) REACT_CONCAT_(l, r)

// Anon var
#ifdef __COUNTER__
#define REACT_ANON_VAR(prefix) \
	REACT_CONCAT(prefix, __COUNTER__)
#else
#define REACT_ANON_VAR(s) \
	REACT_CONCAT(prefix, __LINE__)
#endif

// Scope guard helper
#define REACT_SCOPE_EXIT_PREFIX scopeExitGuard_

#define REACT_SCOPE_EXIT \
	auto REACT_ANON_VAR(REACT_SCOPE_EXIT_PREFIX) \
		= REACT_IMPL::ScopeGuardDummy_() + [&]()