#pragma once

#include "react/Defs.h"

#include <assert.h>
#include <cstddef>
#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

////////////////////////////////////////////////////////////////////////////////////////
REACT_BEGIN

////////////////////////////////////////////////////////////////////////////////////////
/// NonCopyable
////////////////////////////////////////////////////////////////////////////////////////
class NonCopyable
{
protected:
	NonCopyable() {}
	~NonCopyable() {}

private:
	NonCopyable(const NonCopyable&);
	NonCopyable& operator=(const NonCopyable&);
};

////////////////////////////////////////////////////////////////////////////////////////
/// ThreadLocalPtr
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class ThreadLocalStaticPtr
{
public:
	static T*	Get()			{ return ptr_; }
	static void	Set(T* ptr)		{ ptr_ = ptr; }
	static void	Reset()			{ ptr_ = nullptr; }
	static bool	IsNull()		{ return ptr_ == nullptr; }

private:
	ThreadLocalStaticPtr() {}

	static __declspec(thread) T*	ptr_;
};

template <typename T>
T*	ThreadLocalStaticPtr<T>::ptr_(nullptr);

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

// Expand args by wrapping them in a dummy function
// Use comma operator to replace potential void return value with 0
#define REACT_EXPAND_PACK(...) pass((__VA_ARGS__ , 0) ...)

////////////////////////////////////////////////////////////////////////////////////////
/// Format print bits
////////////////////////////////////////////////////////////////////////////////////////
void PrintBits(size_t const size, void const* const p);

////////////////////////////////////////////////////////////////////////////////////////
// Get current date/time
////////////////////////////////////////////////////////////////////////////////////////
const std::string CurrentDateTime();

////////////////////////////////////////////////////////////////////////////////////////
// Get unique random numbers from range.
////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename TGen>
const std::vector<T> GetUniqueRandomNumbers(TGen gen, T from, T to, int count)
{
	std::vector<T>	data(1 + to - from);
	int c = from;
	for (auto& p : data)
		p = c++;

	for (int i=0; i<count; i++)
	{
		std::uniform_int_distribution<T> dist(i,(to - from));
		auto r = dist(gen);
		
		if (r != i)
			std::swap(data[i], data[r]);
	}
	data.resize(count);
	std::sort(data.begin(), data.end());

	return std::move(data);
}

////////////////////////////////////////////////////////////////////////////////////////
// Identity (workaround to enable enable decltype()::X)
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct Identity
{
	typedef T Type;
};

REACT_END