
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <utility>

#include "react/Signal.h"
#include "react/Event.h"
#include "react/Observer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Creating subject-bound observers
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace std;
    using namespace react;

    ReactiveGroup<> group;

    VarSignal<int> x( group, 1 );

    void Run()
    {
        cout << "Example 1 - Creating subject-bound observers (v1)" << endl;

        {
            Signal<int> mySignal( [] (int x) { return x; }, x );

            Observer<> obs( [] (int mySignal) { cout << mySignal << endl; }, mySignal );

            x <<= 2; // output: 2
        }

        x <<= 3; // no ouput

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 2 - Detaching observers manually
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example2
{
    using namespace std;
    using namespace react;

    ReactiveGroup<> group;

    EventSource<> trigger( group );

    void Run()
    {
        cout << "Example 2 - Detaching observers manually" << endl;

        Observer<> obs(
            [] (EventRange<> in)
            {
                for (auto _ : in)
                    cout << "Triggered!" << endl;
            },
            trigger);

        trigger.Emit(); // output: Triggered!

        obs.Cancel();   // Remove the observer

        trigger.Emit(); // no output

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Run examples
///////////////////////////////////////////////////////////////////////////////////////////////////
int main()
{
    example1::Run();
    example2::Run();

    return 0;
}