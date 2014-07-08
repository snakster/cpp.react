
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <utility>

#include "react/Domain.h"
#include "react/Event.h"
#include "react/Observer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Hello world
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace std;
    using namespace react;

    // Defines a domain.
    // Each domain represents a separate dependency graph, managed by a dedicated propagation engine.
    // Reactives of different domains can not be combined.
    REACTIVE_DOMAIN(D, sequential)

    // Define type aliases for the given domain in this namespace.
    // Now we can use EventSourceT instead of D::EventSourceT.
    USING_REACTIVE_DOMAIN(D)

    // An event source that emits values of type string
    namespace v1
    {
        EventSourceT<string> mySource = MakeEventSource<D,string>();

        void Run()
        {
            cout << "Example 1 - Hello world (string source)" << endl;

            Observe(mySource, [] (const string& s) {
                std::cout << s << std::endl;
            });

            mySource << string("Hello world #1");

            // Or without the operator:
            mySource.Emit(string("Hello world #2"));

            // Or as a function call:
            mySource(string("Hello world #3"));

            cout << endl;
        }
    }

    // An event source without an explicit value type
    namespace v2
    {
        EventSourceT<> helloWorldTrigger = MakeEventSource<D>();

        void Run()
        {
            cout << "Example 1 - Hello world (token source)" << endl;

            int count = 0;

            Observe(helloWorldTrigger, [&] (Token) {
                cout << "Hello world #" << ++count << endl;
            });

            helloWorldTrigger.Emit();

            // Or without the stream operator:
            helloWorldTrigger << Token::value;

            // Or as a function call:
            helloWorldTrigger();

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

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    // An event stream that merges both sources
    namespace v1
    {
        EventSourceT<> leftClick  = MakeEventSource<D>();
        EventSourceT<> rightClick = MakeEventSource<D>();

        EventsT<> anyClick = Merge(leftClick, rightClick);

        void Run()
        {
            cout << "Example 2 - Merging event streams (Merge)" << endl;

            int count = 0;

            Observe(anyClick, [&] (Token) {
                cout << "clicked #" << ++count << endl;
            });

            leftClick.Emit();  // output: clicked #1 
            rightClick.Emit(); // output: clicked #2

            cout << endl;
        }
    }

    // Using overloaded operator | instead of explicit Merge
    namespace v2
    {
        EventSourceT<> leftClick  = MakeEventSource<D>();
        EventSourceT<> rightClick = MakeEventSource<D>();

        EventsT<> anyClick = leftClick | rightClick;

        void Run()
        {
            cout << "Example 2 - Merging event streams (operator)" << endl;

            int count = 0;

            Observe(anyClick, [&] (Token) {
                cout << "clicked #" << ++count << endl;
            });

            leftClick.Emit();  // output: clicked #1
            rightClick.Emit(); // output: clicked #2

            cout << endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 3 - Filtering events
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example3
{
    using namespace std;
    using namespace react;

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<int> numbers = MakeEventSource<D,int>();

    EventsT<int> greater10 = Filter(numbers, [] (int n) {
        return n > 10;
    });

    void Run()
    {
        cout << "Example 3 - Filtering events" << endl;

        Observe(greater10, [] (int n) {
            cout << n << endl;
        });

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

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    // Data types
    enum class ETag { normal, critical };
    using TaggedNum = pair<ETag,int>;

    EventSourceT<int> numbers = MakeEventSource<D,int>();

    EventsT<TaggedNum> tagged = Transform(numbers, [] (int n) {
        if (n > 10)
            return TaggedNum( ETag::critical, n );
        else
            return TaggedNum( ETag::normal, n );
    });

    void Run()
    {
        cout << "Example 4 - Transforming  events" << endl;

        Observe(tagged, [] (const TaggedNum& t) {
            if (t.first == ETag::critical)
                cout << "(critical) " << t.second << endl;
            else
                cout << "(normal)  " << t.second << endl;
        });

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

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    EventSourceT<int> src = MakeEventSource<D,int>();

    void Run()
    {
        cout << "Example 5 - Queuing multiple inputs" << endl;

        Observe(src, [] (int v) {
            cout << v << endl;
        }); // output: 1, 2, 3, 4

        DoTransaction<D>([] {
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

    example2::v1::Run();
    example2::v2::Run();

    example3::Run();

    example4::Run();

    example5::Run();

    return 0;
}