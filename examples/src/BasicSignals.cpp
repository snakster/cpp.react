
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "react/Domain.h"
#include "react/Signal.h"
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
    // Now we can use VarSignalT instead of D::VarSignalT.
    USING_REACTIVE_DOMAIN(D)

    // The concat function
    string concatFunc(string first, string second) {
        return first + string(" ") + second;
    }

    // A signal that concatenates both words
    namespace v1
    {
        // The two words
        VarSignalT<string>  firstWord   = MakeVar<D>(string("Change"));
        VarSignalT<string>  secondWord  = MakeVar<D>(string("me!"));

        SignalT<string>  bothWords = MakeSignal(With(firstWord,secondWord), concatFunc);

        void Run()
        {
            cout << "Example 1 - Hello world (MakeSignal)" << endl;

            // Imperative imperative value access
            cout  << bothWords.Value() << endl;

            // Imperative imperative change
            firstWord  <<= string("Hello");

            cout  << bothWords.Value() << endl;

            secondWord <<= string("World");

            cout  << bothWords.Value() << endl;

            cout << endl;
        }
    }

    // Using overloaded operator + instead of explicit MakeSignal
    namespace v2
    {
        // The two words
        VarSignalT<string>  firstWord   = MakeVar<D>(string("Change"));
        VarSignalT<string>  secondWord  = MakeVar<D>(string("me!"));

        SignalT<string> bothWords = firstWord + string(" ") + secondWord;

        void Run()
        {
            cout << "Example 1 - Hello world (operators)" << endl;

            cout  << bothWords.Value() << endl;

            firstWord  <<= string("Hello");

            cout  << bothWords.Value() << endl;

            secondWord <<= string("World");

            cout  << bothWords.Value() << endl;

            cout << endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 2 - Reacting to value changes 
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example2
{
    using namespace std;
    using namespace react;

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    VarSignalT<int> x    = MakeVar<D>(1);
    SignalT<int>    xAbs = MakeSignal(x, [] (int x) { return abs(x); });

    void Run()
    {
        cout << "Example 2 - Reacting to value changes" << endl;

        Observe(xAbs, [] (int newValue) {
            cout << "xAbs changed to " << newValue << endl;
        });

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

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    VarSignalT<int> a = MakeVar<D>(1);
    VarSignalT<int> b = MakeVar<D>(1);

    SignalT<int>     x = a + b;
    SignalT<int>     y = a + b;
    SignalT<int>     z = x + y;

    void Run()
    {
        cout << "Example 3 - Changing multiple inputs" << endl;

        Observe(z, [] (int newValue) {
            std::cout << "z changed to " << newValue << std::endl;
        });

        a <<= 2; // output: z changed to 6
        b <<= 2; // output: z changed to 8

        DoTransaction<D>([] {
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

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    VarSignalT<vector<string>> data = MakeVar<D>(vector<string>{ });

    void Run()
    {
        cout << "Example 4 - Modifying signal values in place" << endl;

        data.Modify([] (vector<string>& data) {
            data.push_back("Hello");
        });

        data.Modify([] (vector<string>& data) {
            data.push_back("World");
        });

        for (const auto& s : data.Value())
            cout << s << " ";
        cout << endl;
        // output: Hell World

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

    REACTIVE_DOMAIN(D, sequential)
    USING_REACTIVE_DOMAIN(D)

    // Helpers
    using ExprPairT = pair<string,int>;
    using ExprVectT = vector<ExprPairT>;

    string makeExprStr(int a, int b, const char* op)
    {
        return to_string(a) + string(op) + to_string(b);
    }

    ExprPairT makeExprPair(const string& s, int v)
    {
        return make_pair(s, v);
    }

    void printExpressions(const ExprVectT& expressions)
    {
        cout << "Expressions: " << endl;
        for (const auto& p : expressions)
            cout << "\t" << p.first << " is " << p.second << endl;
    }

    // Version 1 - Intermediate signals
    namespace v1
    {
        // Input operands
        VarSignalT<int> a = MakeVar<D>(1);
        VarSignalT<int> b = MakeVar<D>(2);

        // Calculations
        SignalT<int> sum  = a + b;
        SignalT<int> diff = a - b;
        SignalT<int> prod = a * b;

        using std::placeholders::_1;
        using std::placeholders::_2;

        // Stringified expressions
        SignalT<string> sumExpr =
            MakeSignal(With(a,b), bind(makeExprStr, _1, _2, "+"));

        SignalT<string> diffExpr =
            MakeSignal(With(a,b), bind(makeExprStr, _1, _2, "-"));

        SignalT<string> prodExpr =
            MakeSignal(With(a,b), bind(makeExprStr, _1, _2, "*"));

        // The expression vector
        SignalT<ExprVectT> expressions = MakeSignal(
            With(
                MakeSignal(With(sumExpr, sum),   &makeExprPair),
                MakeSignal(With(diffExpr, diff), &makeExprPair),
                MakeSignal(With(prodExpr, prod), &makeExprPair)
            ),
            [] (const ExprPairT& sumP, const ExprPairT& diffP, const ExprPairT& prodP) {
                return ExprVectT{ sumP, diffP, prodP};
            });

        void Run()
        {
            cout << "Example 5 - Complex signals (v1)" << endl;

            Observe(expressions, printExpressions);

            a <<= 10;
            b <<= 20;

            cout << endl;
        }
    }

    // Version 2 - Intermediate signals in a function
    namespace v2
    {
        SignalT<ExprVectT> createExpressionSignal(const SignalT<int>& a, const SignalT<int>& b)
        {
            using std::placeholders::_1;
            using std::placeholders::_2;

            // Inside a function, we can use auto
            auto sumExpr =
                MakeSignal(With(a,b), bind(makeExprStr, _1, _2, "+"));

            auto diffExpr =
                MakeSignal(With(a,b), bind(makeExprStr, _1, _2, "-"));

            auto prodExpr =
                MakeSignal(With(a,b), bind(makeExprStr, _1, _2, "*"));

            return MakeSignal(
                With(
                    MakeSignal(With(sumExpr,  a + b), &makeExprPair),
                    MakeSignal(With(diffExpr, a - b), &makeExprPair),
                    MakeSignal(With(prodExpr, a * b), &makeExprPair)
                ),
                [] (const ExprPairT& sumP, const ExprPairT& diffP, const ExprPairT& prodP) {
                    return ExprVectT{ sumP, diffP, prodP };
                });
        }

        // Input operands
        VarSignalT<int> a = MakeVar<D>(1);
        VarSignalT<int> b = MakeVar<D>(2);

        // The expression vector
        SignalT<ExprVectT> expressions = createExpressionSignal(a, b);

        void Run()
        {
            cout << "Example 5 - Complex signals (v2)" << endl;

            Observe(expressions, printExpressions);

            a <<= 30;
            b <<= 40;

            cout << endl;
        }
    }

    // Version 3 - Imperative function
    namespace v3
    {
        // Input operands
        VarSignalT<int> a = MakeVar<D>(1);
        VarSignalT<int> b = MakeVar<D>(2);

        // The expression vector
        SignalT<ExprVectT> expressions = MakeSignal(With(a,b), [] (int a, int b) {
            ExprVectT result;

            result.push_back(
                make_pair(
                    makeExprStr(a, b, "+"),
                    a + b));

            result.push_back(
                make_pair(
                    makeExprStr(a, b, "-"),
                    a - b));

            result.push_back(
                make_pair(
                    makeExprStr(a, b, "*"),
                    a * b));

            return result;
        });

        void Run()
        {
            cout << "Example 5 - Complex signals (v3)" << endl;

            Observe(expressions, printExpressions);

            a <<= 50;
            b <<= 60;

            cout << endl;
        }
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

    example5::v1::Run();
    example5::v2::Run();
    example5::v3::Run();

    return 0;
}