
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <utility>

#include "react/state.h"
#include "react/event.h"
#include "react/observer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Creating subject-bound observers
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace std;
    using namespace react;

    Group group;

    StateVar<int> x = StateVar<int>::Create(group, 1);

    void Run()
    {
        cout << "Example 1 - Creating observers" << endl;

        {
            auto st = State<int>::Create([] (int x) { return x; }, x);

            auto obs = Observer::Create([] (int v) { cout << v << endl; }, st);

            x.Set(2); // output: 2
        }

        x.Set(3); // no ouput

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Run examples
///////////////////////////////////////////////////////////////////////////////////////////////////
int main()
{
    example1::Run();

    return 0;
}