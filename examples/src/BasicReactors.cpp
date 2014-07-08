
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <utility>
#include <vector>

#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"
#include "react/Reactor.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Creating reactive loops
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace std;
    using namespace react;

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    using PointT = pair<int,int>;
    using PathT  = vector<PointT>;

    vector<PathT> paths;

    EventSourceT<PointT> mouseDown = MakeEventSource<D,PointT>();
    EventSourceT<PointT> mouseUp   = MakeEventSource<D,PointT>();
    EventSourceT<PointT> mouseMove = MakeEventSource<D,PointT>();

    ReactorT loop
    {
        [&] (ReactorT::Context ctx)
        {
            PathT points;

            points.emplace_back(ctx.Await(mouseDown));

            ctx.RepeatUntil(mouseUp, [&] {
                points.emplace_back(ctx.Await(mouseMove));
            });

            points.emplace_back(ctx.Await(mouseUp));

            paths.push_back(points);
        }
    };

    void Run()
    {
        cout << "Example 1 - Creating reactive loops" << endl;

        mouseDown << PointT( 1,1 );
        mouseMove << PointT( 2,2 ) << PointT( 3,3 ) << PointT( 4,4 );
        mouseUp   << PointT( 5,5 );

        mouseMove << PointT( 999,999 );

        mouseDown << PointT( 10,10 );
        mouseMove << PointT( 20,20 );
        mouseUp   << PointT( 30,30 );

        for (const auto& path : paths)
        {
            cout << "Path: ";
            for (const auto& point : path)
                cout << "(" << point.first << "," << point.second << ")   ";
            cout << endl;
        }

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 2 - Creating reactive loops
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example2
{
    using namespace std;
    using namespace react;

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    using PointT = pair<int,int>;
    using PathT  = vector<PointT>;

    vector<PathT> paths;

    EventSourceT<PointT> mouseDown = MakeEventSource<D,PointT>();
    EventSourceT<PointT> mouseUp   = MakeEventSource<D,PointT>();
    EventSourceT<PointT> mouseMove = MakeEventSource<D,PointT>();

    VarSignalT<int>      counter   = MakeVar<D>(103);

    ReactorT loop
    {
        [&] (ReactorT::Context ctx)
        {
            PathT points;

            points.emplace_back(ctx.Await(mouseDown));

            auto count = ctx.Get(counter);

            ctx.RepeatUntil(mouseUp, [&] {
                points.emplace_back(ctx.Await(mouseMove));
            });

            points.emplace_back(ctx.Await(mouseUp));

            paths.push_back(points);
        }
    };

    void Run()
    {
        cout << "Example 2 - Creating reactive loops" << endl;

        mouseDown << PointT( 1,1 );
        mouseMove << PointT( 2,2 ) << PointT( 3,3 ) << PointT( 4,4 );
        mouseUp   << PointT( 5,5 );

        counter <<= 42;

        mouseMove << PointT( 999,999 );

        counter <<= 80;

        mouseDown << PointT( 10,10 );
        mouseMove << PointT( 20,20 );
        mouseUp   << PointT( 30,30 );

        for (const auto& path : paths)
        {
            cout << "Path: ";
            for (const auto& point : path)
                cout << "(" << point.first << "," << point.second << ")   ";
            cout << endl;
        }

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