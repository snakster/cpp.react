
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//#define REACT_ENABLE_LOGGING

#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"
#include "react/Algorithm.h"

using namespace std;
using namespace react;

// Defines a domain.
// Each domain represents a separate dependency graph, managed by a dedicated propagation engine.
// Reactives of different domains can not be combined.
REACTIVE_DOMAIN(D, ToposortEngine<sequential_concurrent>)

void SignalExample3()
{
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
    REACTIVE_DOMAIN(L, ToposortEngine<sequential_concurrent>)
    REACTIVE_DOMAIN(R, ToposortEngine<sequential_concurrent>)

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

    cout << endl;
}

void SignalExample5()
{
    REACTIVE_DOMAIN(L, ToposortEngine<sequential_concurrent>)
    REACTIVE_DOMAIN(R, ToposortEngine<sequential_concurrent>)

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

    cout << endl;
}

int main()
{
    SignalExample3();
    SignalExample4();

#ifdef REACT_ENABLE_LOGGING
    std::ofstream logfile;
    logfile.open("log.txt");

    D::Log().Write(logfile);
    logfile.close();
#endif

    return 0;
}