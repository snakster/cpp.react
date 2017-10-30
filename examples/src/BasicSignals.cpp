
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "react/state.h"
#include "react/observer.h"

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

    void PrintFunc(const string& s)
        { cout  << s << endl; }

    // Defines a group.
    // Each group represents a separate dependency graph.
    // Reactives from different groups can not be mixed.
    Group group;
    
    // The two words
    StateVar<string> firstWord = StateVar<string>::Create(group, string("Change"));
    StateVar<string> secondWord = StateVar<string>::Create(group, string("me!"));

    // A signal that concatenates both words
    State<string> bothWords = State<string>::Create(ConcatFunc, firstWord, secondWord);

    void Run()
    {
        auto obs = Observer::Create(PrintFunc, bothWords);

        cout << "Example 1 - Hello world" << endl;

        firstWord.Set(string("Hello"));
        secondWord.Set(string("World"));

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

    Group group;

    StateVar<int> x = StateVar<int>::Create(group, 1);

    State<int> xAbs = State<int>::Create([] (int v) { return abs(v); }, x);

    void Run()
    {
        cout << "Example 2 - Reacting to value changes" << endl;

        auto obs = Observer::Create([] (int newValue)
            { cout << "xAbs changed to " << newValue << endl; }, xAbs);

                    // initially x is 1
        x.Set(2);   // output: xAbs changed to 2
        x.Set(-3);  // output: xAbs changed to 3
        x.Set(3);   // no output, xAbs is still 3

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

    Group group;

    StateVar<int> a = StateVar<int>::Create(group, 1);
    StateVar<int> b = StateVar<int>::Create(group, 1);

    State<int> x = State<int>::Create(sumFunc, a, b);
    State<int> y = State<int>::Create(sumFunc, a, b);
    State<int> z = State<int>::Create(sumFunc, x, y);

    void Run()
    {
        cout << "Example 3 - Changing multiple inputs" << endl;

        auto obs = Observer::Create([] (int newValue)
            { cout << "z changed to " << newValue << endl; }, z);

        a.Set(2); // output: z changed to 6
        b.Set(2); // output: z changed to 8

        group.DoTransaction([&]
            {
                a.Set(4);
                b.Set(4); 
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

    Group group;

    StateVar<vector<string>> data = StateVar<vector<string>>::Create(group);

    void Run()
    {
        cout << "Example 4 - Modifying signal values in place" << endl;

        data.Modify([] (vector<string>& data)
            { data.push_back("Hello"); });

        data.Modify([] (vector<string>& data)
            { data.push_back("World"); });

        auto obs = Observer::Create([] (const vector<string>& data)
            {
                for (const auto& s : data)
                    cout << s << " ";
            }, data);


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

    Group group;

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
    StateVar<int> a = StateVar<int>::Create(group, 1);
    StateVar<int> b = StateVar<int>::Create(group, 2);

    // The expression vector
    State<ExprVectType> expressions = State<ExprVectType>::Create([] (int a, int b)
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

        auto obs = Observer::Create(PrintExpressions, expressions);

        a.Set(50);
        b.Set(60);

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