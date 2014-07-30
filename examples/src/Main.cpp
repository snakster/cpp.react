
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

void testme()
{
    // Note: This project exists as a sandbox where I occasionally stage new examples.
    // Currently it's empty.
}

int main()
{
    testme();

#ifdef REACT_ENABLE_LOGGING
    std::ofstream logfile;
    logfile.open("log.txt");

    D::Log().Write(logfile);
    logfile.close();
#endif

    return 0;
}