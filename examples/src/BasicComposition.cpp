
//          Copyright Sebastian Jeckel 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <utility>

#include "react/state.h"
#include "react/event.h"
#include "react/observer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Reactive class members
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace std;
    using namespace react;

    Group g;

    class Shape
    {
    public:
        StateVar<int> width     = StateVar<int>::Create(g, 0);
        StateVar<int> height    = StateVar<int>::Create(g, 0);

        State<int> size         = State<int>::Create(g, CalcSize, width, height);

        auto GetReactiveMembers() const -> decltype(auto)
            { return std::tie(width, height, size); }

    private:
        static int CalcSize(int w, int h)
            { return w * h; }
    };

    void Run()
    {
        cout << "Example 1 - Reactive class members" << endl;

        auto myShape = ObjectState<Shape>::Create(g, Shape());

        auto obs = Observer::Create([] (const auto& ctx)
            {
                const Shape& shape = ctx.GetObject();
                cout << "Size is " << ctx.Get(shape.size) << endl;
            }, myShape);

        g.DoTransaction([&]
            {
                myShape->width.Set(4);
                myShape->height.Set(4);
            }); // output: Size changed to 16

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 2 - Slots
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example2
{
    using namespace std;
    using namespace react;

    Group g;

    class Company
    {
    public:
        StateVar<string> name;

        Company(const char* name) :
            name( StateVar<string>::Create(g, name) )
        { }

        bool operator==(const Company& other) const
            { return name == other.name; }
    };

    class Employee
    {
    public:
        StateSlot<string> myCompanyName;

        Employee(const Company& company) :
            myCompanyName( StateSlot<string>::Create(company.name) )
        { }

        void SetCompany(const Company& company)
            { myCompanyName.Set(company.name); }
    };

    void Run()
    {
        cout << "Example 2 - Slots" << endl;

        Company company1( "MetroTec" );
        Company company2( "ACME" );

        Employee    alice( company1 );

        auto obs = Observer::Create([] (const string& name)
            {
                cout << "Alice now works for " << name << endl;
            }, alice.myCompanyName);

        company1.name.Set(string( "ModernTec" ));   // output: Alice now works for ModernTec
        alice.SetCompany(company2);                 // output: Alice now works for ACME
        company2.name.Set(string( "A.C.M.E." ));    // output: Alice now works for A.C.M.E.

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