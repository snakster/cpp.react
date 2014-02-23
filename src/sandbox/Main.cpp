#include <iostream>

#include "react/Signal.h"
#include "react/EventStream.h"
#include "react/ReactiveObject.h"

#include "react/propagation/SourceSetEngine.h"

using namespace react;
using namespace std;

// Defines a domain.
// Each domain represents a separate dependency graph, managed by a dedicated propagation engine.
// Reactives of different domains can not be combined.
REACTIVE_DOMAIN(TestDomain, SourceSetEngine);


void SignalExample1()
{
	cout << "Signal Example 1" << endl;

	auto width = TestDomain::MakeVar(60);
	auto height = TestDomain::MakeVar(70);
	auto depth = TestDomain::MakeVar(8);

	auto area	= width * height;
	auto volume	= area * depth;

	cout << "t0" << endl;
	cout << "\tArea: " << area() << endl;
	cout << "\tVolume: " << volume() << endl;

	width <<= 90;
	depth <<= 80;

	cout << "t1" << endl;
	cout << "\tArea: " << area() << endl;
	cout << "\tVolume: " << volume() << endl;

	cout << endl;
}

void SignalExample2()
{
	cout << "Signal Example 2" << endl;

	auto width = TestDomain::MakeVar(60);
	auto height = TestDomain::MakeVar(70);
	auto depth = TestDomain::MakeVar(8);

	auto volume = (width,height,depth) >>= [] (int w, int h, int d) {
		return w * h * d;
	};

	// Observe returns an observer handle, which can be used to detach the observer explicitly.
	// This observer handle holds a shared_ptr to the subject, so as long as it exists,
	// the subject will not be destroyed.
	// The lifetime of the observer itself is tied to the subject.
	Observe(volume, [] (int v) {
		cout << "Volume changed to: " << v << endl;
	});

	{
		TestDomain::ScopedTransaction _;
		width <<= 90;
		depth <<= 80;
	}

	cout << endl;
}

void EventExample1()
{
	cout << "Event Example 1" << endl;

	auto numbers1 = TestDomain::MakeEventSource<int>();
	auto numbers2 = TestDomain::MakeEventSource<int>();

	auto anyNumber = numbers1 | numbers2;

	Observe(anyNumber, [] (int v) {
		cout << "Number: " << v << endl;
	});

	numbers1 << 10 << 20 << 30;
	numbers2 << 40 << 50 << 60;

	cout << endl;
}

class Person : public ReactiveObject<TestDomain>
{
public:
	VarSignal<int>	Age		= MakeVar(1);
	Signal<int>		Health	= 100 - Age;
	Signal<int>		Wisdom  = Age * Age / 100;

	// Note: Initializing them directly uses the same lambda for both signals...
	// compiler bug?
	Observer	wisdomObs;
	Observer	weaknessObs;

	Person()
	{
		wisdomObs = Observe(Wisdom > 50, [] (bool isWise)
		{
			if (isWise)	cout << "I'll do it next week!" << endl;
			else		cout << "I'll do it next month!" << endl;
		});

		weaknessObs = Observe(Health < 25, [] (bool isWeak)
		{
			if (isWeak)	cout << ":<" << endl;
			else		cout << ":D" << endl;
		});
	}
};

void ObjectExample1()
{
	cout << "Object Example 1" << endl;

	Person somePerson;

	somePerson.Age <<= 30;
	somePerson.Age <<= 60;
	somePerson.Age <<= 90;

	cout << "Health: " << somePerson.Health() << endl;
	cout << "Wisdom: " << somePerson.Wisdom() << endl;

	cout << endl;
}

int main()
{
	SignalExample1();
	SignalExample2();
	EventExample1();

	ObjectExample1();

	return 0;
}