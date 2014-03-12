
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

////////////////////////////////////////////////////////////////////////////////////////
namespace react {

////////////////////////////////////////////////////////////////////////////////////////
/// BasicSingleton
////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
class BasicSingleton
{
public:
	static T& Instance()
	{
		static T instance;
		return instance;
	}

protected:
	BasicSingleton() {}
	BasicSingleton(const BasicSingleton& other) {}
	BasicSingleton(BasicSingleton&& other) {}
};

} //~namespace react