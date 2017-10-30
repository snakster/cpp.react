
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>

#include "tbb/tick_count.h"

#include "react/state.h"
#include "react/event.h"
#include "react/observer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Asynchronous transactions
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace react;
    using namespace std;

    Group group;

    class Sensor
    {
    public:
        EventSource<int> samples = EventSource<int>::Create(group);
    };

    void Run()
    {
        cout << "Example 1 - Asynchronous transactions" << endl;

        Sensor mySensor;

        auto obs = Observer::Create([&] (const auto& events)
            {
                for (auto t : events)
                    cout << t << endl;
            }, mySensor.samples);

        SyncPoint sp;

        group.EnqueueTransaction([&]
            {
                mySensor.samples << 30 << 31 << 31 << 32;
            }, sp);

        group.EnqueueTransaction([&]
            {
                mySensor.samples << 40 << 41 << 51 << 62;
            }, sp);

        // Waits until both transactions are completed.
        // This does not mean that both transactions are interleaved.
        sp.Wait();

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

    Group group;

    class Sensor
    {
    public:
        EventSource<int> samples = EventSource<int>::Create(group);
    };

    const int K = 10000;

    namespace v1
    {
        void Run()
        {
            cout << "Example 2 - Transaction merging (no merging)" << endl;

            Sensor mySensor;
            int sum = 0;

            auto obs = Observer::Create([&] (const auto& events)
                {
                    for (int e : events)
                        sum += e;
                }, mySensor.samples);

            SyncPoint sp;

            cout << "Executing " << K << " async transactions...";

            auto t0 = tbb::tick_count::now();

            for (int i=0; i < K; i++)
            {
                group.EnqueueTransaction([&]
                    {
                        mySensor.samples << 3 << 4 << 2 << 1;
                    }, sp);
            }

            sp.Wait();

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

            auto obs = Observer::Create([&] (const auto& events)
                {
                    for (int e : events)
                        sum += e;
                }, mySensor.samples);

            SyncPoint sp;

            cout << "Executing " << K << " async transactions...";

            auto t0 = tbb::tick_count::now();

            for (int i=0; i < K; i++)
            {
                group.EnqueueTransaction([&]
                    {
                        mySensor.samples << 3 << 4 << 2 << 1;
                    }, sp, TransactionFlags::allow_merging);
            }

            sp.Wait();

            double d = (tbb::tick_count::now() - t0).seconds();

            cout << " done." << endl;

            cout << "  Sum: " << sum << endl;
            cout << "  Time: " << d << endl;

            cout << endl;
        }
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

    return 0;
}