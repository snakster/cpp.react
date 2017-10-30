
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <utility>

#include "react/Event.h"
#include "react/Observer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Hello world
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace std;
    using namespace react;

    // Defines a group.
    // Each group represents a separate dependency graph.
    // Reactives from different groups can not be mixed.
    Group group;

    // An event source that emits values of type string
    namespace v1
    {
        EventSource<string> mySource = EventSource<string>::Create(group);

        void Run()
        {
            cout << "Example 1 - Hello world (string source)" << endl;

            auto obs = Observer::Create([] (const auto& events)
                {
                    for (const auto& e : events)
                        cout << e << std::endl;
                }, mySource);

            mySource << string("Hello world #1");

            // Or without the operator:
            mySource.Emit(string("Hello world #2"));

            cout << endl;
        }
    }

    // An event source without an explicit value type
    namespace v2
    {
        EventSource<> helloWorldTrigger = EventSource<>::Create(group);

        void Run()
        {
            cout << "Example 1 - Hello world (token source)" << endl;

            int count = 0;

            auto obs = Observer::Create([&] (const auto& events)
                {
                    for (auto t : events)
                        cout << "Hello world #" << ++count << endl;
                }, helloWorldTrigger);

            helloWorldTrigger.Emit();

            // Or without the stream operator:
            helloWorldTrigger << Token::value;

            cout << endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 2 - Merging event streams
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example2
{
    using namespace std;
    using namespace react;

    Group group;

    // An event stream that merges both sources
    EventSource<> leftClick = EventSource<>::Create(group);
    EventSource<> rightClick = EventSource<>::Create(group);

    Event<> anyClick = Merge(leftClick, rightClick);

    void Run()
    {
        cout << "Example 2 - Merging event streams (Merge)" << endl;

        int count = 0;

        auto obs = Observer::Create([&] (const auto& events)
            {
                for (auto t : events)
                    cout << "clicked #" << ++count << endl;
            }, anyClick);

        leftClick.Emit();  // output: clicked #1 
        rightClick.Emit(); // output: clicked #2

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 3 - Filtering events
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example3
{
    using namespace std;
    using namespace react;

    Group group;

    EventSource<int> numbers = EventSource<int>::Create(group);

    Event<int> greater10 = Filter([] (int n) { return n > 10; }, numbers);

    void Run()
    {
        cout << "Example 3 - Filtering events" << endl;

        auto obs = Observer::Create([&] (const auto& events)
            {
                for (auto n : events)
                    cout << n << endl;
            }, greater10);

        numbers << 5 << 11 << 7 << 100; // output: 11, 100

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 4 - Filtering events
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example4
{
    using namespace std;
    using namespace react;

    Group group;

    // Data types
    enum class Tag { normal, critical };
    using TaggedNum = pair<Tag, int>;

    EventSource<int> numbers = EventSource<int>::Create(group);

    Event<TaggedNum> tagged = Transform<TaggedNum>([] (int n)
        {
            if (n > 10)
                return TaggedNum( Tag::critical, n );
            else
                return TaggedNum( Tag::normal, n );
        }, numbers);

    void Run()
    {
        cout << "Example 4 - Transforming  events" << endl;

        auto obs = Observer::Create([] (const auto& events)
            {
                for (TaggedNum e : events)
                {
                    if (e.first == Tag::critical)
                        cout << "(critical) " << e.second << endl;
                    else
                        cout << "(normal)  " << e.second << endl;
                }
            }, tagged);

        numbers << 5;   // output: (normal) 5
        numbers << 20;  // output: (critical) 20

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 5 - Queuing multiple inputs
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example5
{
    using namespace std;
    using namespace react;

    Group group;

    EventSource<int> src = EventSource<int>::Create(group);

    void Run()
    {
        cout << "Example 5 - Queuing multiple inputs" << endl;

        auto obs = Observer::Create([] (const auto& events)
            {
                for (int e : events)
                    cout << e << endl;
            }, src);
        // output: 1, 2, 3, 4

        group.DoTransaction([]
            {
                src << 1 << 2 << 3;
                src << 4;
            });

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
    example4::Run();
    example5::Run();

    return 0;
}