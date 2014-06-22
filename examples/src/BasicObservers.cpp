
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <utility>

#include "react/Domain.h"
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

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    auto x = MakeVar<D>(1);

    namespace v1
    {
        void Run()
        {
            cout << "Example 1 - Creating subject-bound observers (v1)" << endl;

            {
                // Create a signal in the function scope
                auto mySignal = MakeSignal(x, [] (int x) { return x; } );

                // The lifetime of the observer is bound to mySignal.
                // After Run() returns, mySignal is destroyed, and so is the observer
                Observe(mySignal, [] (int mySignal) {
                    cout << mySignal << endl;
                });

                x <<= 2; // output: 2
            }

            x <<= 3; // no ouput

            cout << endl;
        }
    }

    namespace v2
    {
        void Run()
        {
            cout << "Example 1 - Creating subject-bound observers (v2)" << endl;

            // Outer scope
            {
                // Unbound observer
                ObserverT obs; 

                // Inner scope
                {
                    auto mySignal = MakeSignal(x, [] (int x) { return x; } );

                    // Move-assign to obs
                    obs = Observe(mySignal, [] (int mySignal) {
                        cout << mySignal << endl;
                    });

                    // The node linked to mySignal is now also owned by obs

                    x <<= 2; // output: 2
                }
                // ~Inner scope

                // mySignal was destroyed, but as long as obs exists and is still
                // attached to the signal node, this signal node won't be destroyed

                x <<= 3; // output: 3
            }
            // ~Outer scope

            // obs was destroyed
            // -> the signal node is no longer owned by anything and is destroyed
            // -> the observer node is destroyed as it was bound to the subject

            x <<= 4; // no ouput

            cout << endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 2 - Detaching observers manually
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example2
{
    using namespace std;
    using namespace react;

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<> trigger = MakeEventSource<D>();

    void Run()
    {
        cout << "Example 2 - Detaching observers manually" << endl;

        ObserverT obs = Observe(trigger, [] (Token) {
            cout << "Triggered!" << endl;
        });

        trigger.Emit(); // output: Triggered!

        obs.Detach();   // Remove the observer

        trigger.Emit(); // no output

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 3 - Using scoped observers
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example3
{
    using namespace std;
    using namespace react;

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<> trigger = MakeEventSource<D>();

    void Run()
    {
        cout << "Example 3 - Using scoped observers" << endl;

        // Inner scope
        {
            ScopedObserverT scopedObs
            (
                Observe(trigger, [] (Token) {
                    cout << "Triggered!" << endl;
                })
            );

            trigger.Emit(); // output: Triggered!
        }
        // ~Inner scope

        trigger.Emit(); // no output

        // Note the semantic difference between ScopedObserverT and ObserverT.
        //
        // During its lifetime, the ObserverT handle of an observer guarantees that the
        // observed subject will not be destroyed and allows explicit detach.
        // But even after the ObserverT handle is destroyed, the subject may continue to exist
        // and so will the observer.
        //
        // ScopedObserverT has similar semantics to a scoped lock.
        // When it's destroyed, it detaches and destroys the observer.

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Run examples
///////////////////////////////////////////////////////////////////////////////////////////////////
int main()
{
    example1::v1::Run();
    example1::v2::Run();

    example2::Run();

    example3::Run();

    return 0;
}