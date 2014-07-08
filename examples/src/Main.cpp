
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//#define REACT_ENABLE_LOGGING

#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"
#include "react/Algorithm.h"

#include "tbb/tick_count.h"

#include <chrono>

using namespace std;
using namespace react;

// Defines a domain.
// Each domain represents a separate dependency graph, managed by a dedicated propagation engine.
// Reactives of different domains can not be combined.


void SignalExample3()
{
    REACTIVE_DOMAIN(D, sequential_concurrent)

    cout << "Signal Example 3" << endl;

    auto src = MakeVar<D>(0);

    // Input values can be manipulated imperatively in observers.
    // Inputs are implicitly thread-safe, buffered and executed in a continuation turn.
    // This continuation turn is queued just like a regular turn.
    // If other turns are already queued, they are executed before the continuation.
    auto cont = MakeContinuation(src, [&] (int v) {
        cout << "V: " << v << endl;
        if (v < 10)
            src <<= v+1;
    });

    src <<= 1;

    cout << endl;
}

void SignalExample4()
{
    REACTIVE_DOMAIN(L, sequential_concurrent, ToposortEngine)
    REACTIVE_DOMAIN(R, sequential_concurrent, ToposortEngine)

    cout << "Signal Example 4" << endl;

    auto srcL = MakeVar<L>(0);
    auto srcR = MakeVar<R>(0);

    auto contL = MakeContinuation<L,R>(srcL, [&] (int v) {
        cout << "L->R: " << v << endl;
        if (v < 10)
            srcR <<= v+1;
    });

    auto contR = MakeContinuation<R,L>(srcR, [&] (int v) {
        cout << "R->L: " << v << endl;
        if (v < 10)
            srcL <<= v+1;
    });

    srcL <<= 1;
    printf("End\n");

    cout << endl;
}

void SignalExample5()
{
    REACTIVE_DOMAIN(L, sequential_concurrent, ToposortEngine)
    REACTIVE_DOMAIN(R, sequential_concurrent, ToposortEngine)

    cout << "Signal Example 5" << endl;

    auto srcL = MakeVar<L>(0);
    auto depL1 = MakeVar<L>(0);
    auto depL2 = MakeVar<L>(0);
    auto srcR = MakeVar<R>(0);

    auto contL = MakeContinuation<L,R>(
        Monitor(srcL),
        With(depL1, depL2),
        [&] (int v, int depL1, int depL2) {
            cout << "L->R: " << v << endl;
            if (v < 10)
                srcR <<= v+1;
        });

    auto contR = MakeContinuation<R,L>(
        Monitor(srcR),
        [&] (int v) {
            cout << "R->L: " << v << endl;
            if (v < 10)
                srcL <<= v+1;
        });

    srcL <<= 1;
    printf("End\n");

    cout << endl;
}

void testme()
{
    REACTIVE_DOMAIN(D, sequential_concurrent)

    std::vector<int> results;

    auto f_0 = [] (int a) -> int
    {
        int k = 0;
        for (int i = 0; i<10000; i++)
            k += i;
        return a + k;
    };

    auto f_n = [] (int a, int b) -> int
    {
        int k = 0;
        for (int i=0; i<10000; i++)
            k += i;
        return a + b + k;
    };

    auto n1 = MakeVar<D>(0);
    auto n2 = n1 ->* f_0;
    auto n3 = ((n2, n1) ->* f_n) ->* f_0;
    auto n4 = n3 ->* f_0;
    auto n5 = ((((n4, n3) ->* f_n), n1) ->* f_n) ->* f_0;
    auto n6 = n5 ->* f_0;
    auto n7 = ((n6, n5) ->* f_n) ->* f_0;
    auto n8 = n7 ->* f_0;
    auto n9 = ((((((n8, n7) ->* f_n), n5) ->* f_n), n1) ->* f_n) ->* f_0;
    auto n10 = n9 ->* f_0;
    auto n11 = ((n10, n9) ->* f_n) ->* f_0;
    auto n12 = n11 ->* f_0;
    auto n13 = ((((n12, n11) ->* f_n), n9) ->* f_n) ->* f_0;
    auto n14 = n13 ->* f_0;
    auto n15 = ((n14, n13) ->* f_n) ->* f_0;
    auto n16 = n15 ->* f_0;
    auto n17 = ((((((n16, n15) ->* f_n), n13) ->* f_n), n9) ->* f_n)  ->* f_0;

    auto src = MakeEventSource<D,int>();

    atomic<int> c( 0 );

    Observe(src, [&] (int v){
        c++;
    });

    auto x0 = tbb::tick_count::now();

    TransactionStatus st;

    for (int i=0; i<10000; i++)
    {
        AsyncTransaction<D>(st, [&,i] {
            n1 <<= 1+i;
        });
    }

    for (int i=0; i<10000; i++)
    {
        AsyncTransaction<D>(st, [&,i] {
            n1 <<= 20000+i;
        });
    }

    for (int i=0; i<10000; i++)
    {
        AsyncTransaction<D>(st, [&,i] {
            n1 <<= 100000+i;
        });
    }

    st.Wait();

    //std::thread t3([&] {
    //    for (int i=0; i<10000; i++)
    //        n1 <<= 1+i;
    //});

    //std::thread t2([&] {
    //    for (int i=0; i<10000; i++)
    //        n1 <<= 20000+i;
    //});

    //std::thread t1([&] {
    //    for (int i=0; i<10000; i++)
    //        n1 <<= 100000+i;
    //});

    //t3.join();
    //t2.join();
    //t1.join();
    //std::chrono::milliseconds dura( 10000 );
    //std::this_thread::sleep_for( dura );

    auto x1 = tbb::tick_count::now();

    double d = (x1 - x0).seconds();
    printf("Time %g\n", d);
    printf("Updates %d\n", c.load());
    printf("n1 %d\n", n1());
}

int main()
{
    SignalExample3();
    SignalExample4();
    testme();

#ifdef REACT_ENABLE_LOGGING
    std::ofstream logfile;
    logfile.open("log.txt");

    D::Log().Write(logfile);
    logfile.close();
#endif

    return 0;
}