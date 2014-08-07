
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef REACT_COMMON_UTIL_H_INCLUDED
#define REACT_COMMON_UTIL_H_INCLUDED

#pragma once

#include "react/detail/Defs.h"

#include <tuple>
#include <type_traits>
#include <utility>

/***************************************/ REACT_IMPL_BEGIN /**************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Unpack tuple - see
/// http://stackoverflow.com/questions/687490/how-do-i-expand-a-tuple-into-variadic-template-functions-arguments
///////////////////////////////////////////////////////////////////////////////////////////////////

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

template<size_t N>
struct ApplyMemberFn {
    template <typename O, typename F, typename T, typename... A>
    static inline auto apply(O obj, F f, T && t, A &&... a)
        -> decltype(ApplyMemberFn<N-1>::apply(obj, f, std::forward<T>(t), std::get<N-1>(std::forward<T>(t)), std::forward<A>(a)...))
    {
        return ApplyMemberFn<N-1>::apply(obj, f, std::forward<T>(t), std::get<N-1>(std::forward<T>(t)), std::forward<A>(a)...);
    }
};

template<>
struct ApplyMemberFn<0>
{
    template<typename O, typename F, typename T, typename... A>
    static inline auto apply(O obj, F f, T &&, A &&... a)
        -> decltype((obj->*f)(std::forward<A>(a)...))
    {
        return (obj->*f)(std::forward<A>(a)...);
    }
};

template <typename O, typename F, typename T>
inline auto applyMemberFn(O obj, F f, T&& t)
    -> decltype(ApplyMemberFn<std::tuple_size<typename std::decay<T>::type>::value>::apply(obj, f, std::forward<T>(t)))
{
    return ApplyMemberFn<std::tuple_size<typename std::decay<T>::type>::value>
        ::apply(obj, f, std::forward<T>(t));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Helper to enable calling a function on each element of an argument pack.
/// We can't do f(args) ...; because ... expands with a comma.
/// But we can do nop_func(f(args) ...);
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename... TArgs>
inline void pass(TArgs&& ...) {}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Identity (workaround to enable enable decltype()::X)
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct Identity
{
    using Type = T;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DontMove!
///////////////////////////////////////////////////////////////////////////////////////////////////
struct DontMove {};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// DisableIfSame
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename T, typename U>
struct DisableIfSame :
    std::enable_if<! std::is_same<
        typename std::decay<T>::type,
        typename std::decay<U>::type>::value> {};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// AddDummyArgWrapper
///////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TArg, typename F, typename TRet, typename ... TDepValues>
struct AddDummyArgWrapper
{
    AddDummyArgWrapper(const AddDummyArgWrapper& other) = default;

    AddDummyArgWrapper(AddDummyArgWrapper&& other) :
        MyFunc( std::move(other.MyFunc) )
    {}

    template <typename FIn, class = typename DisableIfSame<FIn,AddDummyArgWrapper>::type>
    explicit AddDummyArgWrapper(FIn&& func) : MyFunc( std::forward<FIn>(func) ) {}

    TRet operator()(TArg, TDepValues& ... args)
    {
        return MyFunc(args ...);
    }

    F MyFunc;
};

template <typename TArg, typename F, typename ... TDepValues>
struct AddDummyArgWrapper<TArg,F,void,TDepValues...>
{
public:
    AddDummyArgWrapper(const AddDummyArgWrapper& other) = default;

    AddDummyArgWrapper(AddDummyArgWrapper&& other) :
        MyFunc( std::move(other.MyFunc) )
    {}

    template <typename FIn, class = typename DisableIfSame<FIn,AddDummyArgWrapper>::type>
    explicit AddDummyArgWrapper(FIn&& func) : MyFunc( std::forward<FIn>(func) ) {}

    void operator()(TArg, TDepValues& ... args)
    {
        MyFunc(args ...);
    }

    F MyFunc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// AddDefaultReturnValueWrapper
///////////////////////////////////////////////////////////////////////////////////////////////////
template 
<
    typename F,
    typename TRet,
    TRet return_value
>
struct AddDefaultReturnValueWrapper
{
    AddDefaultReturnValueWrapper(const AddDefaultReturnValueWrapper&) = default;

    AddDefaultReturnValueWrapper(AddDefaultReturnValueWrapper&& other) :
        MyFunc( std::move(other.MyFunc) )
    {}

    template
    <
        typename FIn,
        class = typename DisableIfSame<FIn,AddDefaultReturnValueWrapper>::type
    >
    explicit AddDefaultReturnValueWrapper(FIn&& func) :
        MyFunc( std::forward<FIn>(func) )
    {}

    template <typename ... TArgs>
    TRet operator()(TArgs&& ... args)
    {
        MyFunc(std::forward<TArgs>(args) ...);
        return return_value;
    }

    F MyFunc;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
/// IsCallableWith
///////////////////////////////////////////////////////////////////////////////////////////////////
template
<
    typename F,
    typename TRet,
    typename ... TArgs
>
class IsCallableWith
{
private:
    using NoT = char[1];
    using YesT = char[2];

    template
    <
        typename U,
        class = decltype(
            static_cast<TRet>(
                (std::declval<U>())(std::declval<TArgs>() ...)))
    >
    static YesT& check(void*);

    template <typename U>
    static NoT& check(...);

public:
    enum { value = sizeof(check<F>(nullptr)) == sizeof(YesT) };
};

/****************************************/ REACT_IMPL_END /***************************************/

// Expand args by wrapping them in a dummy function
// Use comma operator to replace potential void return value with 0
#define REACT_EXPAND_PACK(...) REACT_IMPL::pass((__VA_ARGS__ , 0) ...)

#endif // REACT_COMMON_UTIL_H_INCLUDED