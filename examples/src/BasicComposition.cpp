
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <utility>

#include "react/Domain.h"
#include "react/Signal.h"
#include "react/Event.h"
#include "react/Observer.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 1 - Reactive class members
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example1
{
    using namespace std;
    using namespace react;

    REACTIVE_DOMAIN(D, sequential)

    class Shape
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        VarSignalT<int>     Width   = MakeVar<D>(0);
        VarSignalT<int>     Height  = MakeVar<D>(0);

        SignalT<int>        Size    = Width * Height;

        EventSourceT<>      HasMoved = MakeEventSource<D>();
    };

    void Run()
    {
        cout << "Example 1 - Reactive class members" << endl;

        Shape myShape;

        Observe(myShape.Size, [] (int newValue) {
            cout << "Size changed to " << newValue << endl;
        });

        DoTransaction<D>([&] {
            myShape.Width  <<= 4;
            myShape.Height <<= 4; 
        }); // output: Size changed to 16

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 2 - Signals of references
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example2
{
    using namespace std;
    using namespace react;

    REACTIVE_DOMAIN(D, sequential)

    class Company
    {
    public:
        const char* Name;

        Company(const char* name) :
            Name( name )
        {}

        // Note: To be used as a signal value type,
        // values of the type must be comparable
        bool operator==(const Company& other) const
        {
            return this == &other;
        }
    };

    class Employee
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        VarSignalT<Company&> MyCompany;

        Employee(Company& company) :
            MyCompany( MakeVar<D>(ref(company)) )
        {}
    };

    void Run()
    {
        cout << "Example 2 - Signals of references" << endl;

        Company     company1( "MetroTec" );
        Company     company2( "ACME" );

        Employee    bob( company1 );

        Observe(bob.MyCompany, [] (const Company& company) {
            cout << "Bob works for " << company.Name << endl;
        });

        bob.MyCompany <<= ref(company2); // output: Bob now works for ACME

        cout << endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
/// Example 3 - Dynamic signal references
///////////////////////////////////////////////////////////////////////////////////////////////////
namespace example3
{
    using namespace std;
    using namespace react;

    REACTIVE_DOMAIN(D, sequential)

    class Company
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        VarSignalT<string> Name;

        Company(const char* name) :
            Name( MakeVar<D>(string( name )) )
        {}

        bool operator==(const Company& other) const
        {
            return this == &other;
        }
    };

    class Employee
    {
    public:
        USING_REACTIVE_DOMAIN(D)

        VarSignalT<Company&> MyCompany;

        Employee(Company& company) :
            MyCompany( MakeVar<D>(ref(company)) )
        {}
    };

    void Run()
    {
        cout << "Example 3 - Dynamic signal references" << endl;

        Company     company1( "MetroTec" );
        Company     company2( "ACME" );

        Employee    alice( company1 );

        auto obs = Observe(
            REACTIVE_REF(alice.MyCompany, Name),
            [] (const string& name) {
                cout << "Alice now works for " << name << endl;
            });

        company1.Name   <<= string( "ModernTec" );  // output: Alice now works for ModernTec
        alice.MyCompany <<= ref(company2);          // output: Alice now works for ACME
        company2.Name   <<= string( "A.C.M.E." );   // output: Alice now works for A.C.M.E.

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

    return 0;
}