
//          Copyright Sebastian Jeckel 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "react/Signal.h"
#include "react/Observer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Hello world
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace std;
    using namespace react;

    // The concat function
    string ConcatFunc(string first, string second)
        { return first + string(" ") + second; }

    // Defines a group.
    // Each domain represents a separate dependency graph, managed by a dedicated propagation engine.
    // Reactives of different domains can not be combined.
    ReactiveGroup<> group;
    
    // The two words
    VarSignal<string> firstWord( string("Change"), group );
    VarSignal<string> secondWord( string("me!"), group );

    // A signal that concatenates both words
    Signal<string> bothWords(ConcatFunc, firstWord, secondWord);

    void Run()
    {
        cout << "Example 1 - Hello world" << endl;

        cout  << bothWords.Value() << endl;

        firstWord <<= string("Hello");

        cout << bothWords.Value() << endl;

        secondWord <<= string("World");

        cout << bothWords.Value() << endl;

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 2 - Reacting to value changes 
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example2
{
    using namespace std;
    using namespace react;

    ReactiveGroup<> group;

    VarSignal<int> x( 1, group );

    Signal<int> xAbs( [] (int v) { return abs(v); }, x);

    void Run()
    {
        cout << "Example 2 - Reacting to value changes" << endl;

        Observer<> obs(
            [] (int newValue) { cout << "xAbs changed to " << newValue << endl; },
            xAbs );

                    // initially x is 1
        x <<=  2;   // output: xAbs changed to 2
        x <<= -3;   // output: xAbs changed to 3
        x <<=  3;   // no output, xAbs is still 3

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 3 - Changing multiple inputs
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example3
{
    using namespace std;
    using namespace react;

    int sumFunc(int a, int b)
        { return a + b; }

    ReactiveGroup<> group;

    VarSignal<int> a( 1, group );
    VarSignal<int> b( 1, group );

    Signal<int> x( sumFunc, a, b );
    Signal<int> y( sumFunc, a, b );
    Signal<int> z( sumFunc, x, y );

    void Run()
    {
        cout << "Example 3 - Changing multiple inputs" << endl;

        Observer<> obs(
            [] (int newValue) { cout << "z changed to " << newValue << endl; },
            z );

        a <<= 2; // output: z changed to 6
        b <<= 2; // output: z changed to 8

        group.DoTransaction([&]
        {
            a <<= 4;
            b <<= 4; 
        }); // output: z changed to 16

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 4 - Modifying signal values in place
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example4
{
    using namespace std;
    using namespace react;

    ReactiveGroup<> group;

    VarSignal<vector<string>> data( group );

    void Run()
    {
        cout << "Example 4 - Modifying signal values in place" << endl;

        data.Modify([] (vector<string>& data)
            { data.push_back("Hello"); });

        data.Modify([] (vector<string>& data)
            { data.push_back("World"); });

        for (const auto& s : data.Value())
            cout << s << " ";
        cout << endl;
        // output: Hello World

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 5 - Complex signals
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example5
{
    using namespace std;
    using namespace react;

    ReactiveGroup<> group;

    // Helpers
    using ExprPairType = pair<string, int>;
    using ExprVectType = vector<ExprPairType>;

    string MakeExprStr(int a, int b, const char* op)
    {
        return to_string(a) + string(op) + to_string(b);
    }

    void PrintExpressions(const ExprVectType& expressions)
    {
        cout << "Expressions: " << endl;
        for (const auto& p : expressions)
            cout << "\t" << p.first << " is " << p.second << endl;
    }

    // Input operands
    VarSignal<int> a( 1, group );
    VarSignal<int> b( 2, group );

    // The expression vector
    Signal<ExprVectType> expressions(
        [] (int a, int b)
        {
            ExprVectType result;
            result.push_back(make_pair(MakeExprStr(a, b, "+"), a + b));
            result.push_back(make_pair(MakeExprStr(a, b, "-"), a - b));
            result.push_back(make_pair(MakeExprStr(a, b, "*"), a * b));
            return result;
        }, a, b );

    void Run()
    {
        cout << "Example 5 - Complex signals (v3)" << endl;

        Observer<> obs(PrintExpressions, expressions);

        a <<= 50;
        b <<= 60;

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
    example3::Run();
    example4::Run();
    example5::Run();

    return 0;
}