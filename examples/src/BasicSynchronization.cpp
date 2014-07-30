
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include "tbb/tick_count.h"

#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"
#include "react/Observer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Asynchronous transactions
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace react;
    using namespace std;

    REACTIVE_DOMAIN(D, sequential_concurrent)

    class Sensor
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        EventSourceT<int>   Samples     = MakeEventSource<D,int>();
    };

    void Run()
    {
        cout << "Example 1 - Asynchronous transactions" << endl;

        Sensor mySensor;

        Observe(mySensor.Samples, [] (int v) {
            cout << v << endl;
        });

        TransactionStatus status;

        AsyncTransaction<D>(status, [&] {
            mySensor.Samples << 30 << 31 << 31 << 32;
        });

        AsyncTransaction<D>(status, [&] {
            mySensor.Samples << 40 << 41 << 51 << 62;
        });

        // Waits until both transactions are completed.
        // This does not mean that both transactions are interleaved.
        status.Wait();

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 2 - Transaction merging
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example2
{
    using namespace react;
    using namespace std;

    REACTIVE_DOMAIN(D, sequential_concurrent)

    class Sensor
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        EventSourceT<int>   Samples     = MakeEventSource<D,int>();
    };

    const int K = 100000;

    namespace v1
    {
        void Run()
        {
            cout << "Example 2 - Transaction merging (no merging)" << endl;

            Sensor mySensor;
            int sum = 0;

            Observe(mySensor.Samples, [&] (int v) {
                sum += v;
            });

            TransactionStatus status;

            cout << "Executing " << K << " async transactions...";

            auto t0 = tbb::tick_count::now();

            for (int i=0; i < K; i++)
            {
                AsyncTransaction<D>(status, [&] {
                    mySensor.Samples << 3 << 4 << 2 << 1;
                });
            }

            status.Wait();

            double d = (tbb::tick_count::now() - t0).seconds();

            cout << " done." << endl;

            cout << "  Sum: " << sum << endl;
            cout << "  Time: " << d << endl;

            cout << endl;
        }
    }

    namespace v2
    {
        void Run()
        {
            cout << "Example 2 - Transaction merging (allow merging)" << endl;

            Sensor mySensor;
            int sum = 0;

            Observe(mySensor.Samples, [&] (int v) {
                sum += v;
            });

            TransactionStatus status;

            cout << "Executing " << K << " async transactions...";

            auto t0 = tbb::tick_count::now();

            for (int i=0; i < K; i++)
            {
                AsyncTransaction<D>(allow_merging, status, [&] {
                    mySensor.Samples << 3 << 4 << 2 << 1;
                });
            }

            status.Wait();

            double d = (tbb::tick_count::now() - t0).seconds();

            cout << " done." << endl;

            cout << "  Sum: " << sum << endl;
            cout << "  Time: " << d << endl;

            cout << endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 3 - Continuations (1)
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example3
{
    using namespace react;
    using namespace std;

    REACTIVE_DOMAIN(D, sequential_concurrent)

    class Widget
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        VarSignalT<string>  Label1 = MakeVar<D>(string( "Change" ));
        VarSignalT<string>  Label2 = MakeVar<D>(string( "me!" ));

        EventSourceT<>      Reset  = MakeEventSource<D>();

        Widget() :
            resetCont_
            (
                MakeContinuation<D>(
                    Reset,
                    [this] (Token) {
                        Label1 <<= string( "Change" );
                        Label2 <<= string( "me!" );
                    })
            )
        {}

    private:
        Continuation<D> resetCont_;
    };

    void Run()
    {
        cout << "Example 3 - Continuations (1)" << endl;

        Widget myWidget;

        Observe(myWidget.Label1, [&] (const string& v) {
            cout << "Label 1 changed to " << v << endl;
        });

        Observe(myWidget.Label2, [&] (const string& v) {
            cout << "Label 2 changed to " << v << endl;
        });

        myWidget.Label1 <<= "Hello";
        myWidget.Label2 <<= "world";

        cout << "Resetting..." << endl;

        myWidget.Reset();

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Run examples
///////////////////////////////////////////////////////////////////////////////////////////////////
int main()
{
    example1::Run();

    example2::v1::Run();
    example2::v2::Run();

    example3::Run();

    return 0;
}