#include <iostream>

#include "react/Signal.h"
//#include "react/EventStream.h"
//#include "react/Operations.h"
//#include "react/ReactiveObject.h"
//
#include "react/propagation/SourceSetEngine.h"
//
using namespace std;
using namespace react;
//
//// Defines a domain.
//// Each domain represents a separate dependency graph, managed by a dedicated propagation engine.
//// Reactives of different domains can not be combined.
REACTIVE_DOMAIN(D, SourceSetEngine);
//
//
void SignalExample1()
{
	cout << "Signal Example 1" << endl;

	auto width = D::MakeVar(60);
	auto height = D::MakeVar(70);
	auto depth = D::MakeVar(8);

	auto area	= width * height;
	auto volume	= area * depth;

	Observe(volume, [] (int v) {
		cout << "Volume changed to: " << v << endl;
	});

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

	auto width = D::MakeVar(60);
	auto height = D::MakeVar(70);
	auto depth = D::MakeVar(8);

	auto src = D::MakeVar(0);

	auto volume = (width,height,depth) >>= [] (int w, int h, int d) {
		return w * h * d;
	};

	// Observe returns an observer handle, which can be used to detach the observer explicitly.
	// This observer handle holds a shared_ptr to the subject, so as long as it exists,
	// the subject will not be destroyed.
	// The lifetime of the observer itself is tied to the subject.
	Observe(src, [&] (int v) {
		cout << "v: " << v << endl;
		if (v < 10)
			src <<= v+1;
	});

	D::DoTransaction([&] {
		width <<= 90;
		depth <<= 80;
	});

	src <<= 1;

	cout << endl;
}

//
//void EventExample1()
//{
//	cout << "Event Example 1" << endl;
//
//	auto numbers1 = D::MakeEventSource<int>();
//	auto numbers2 = D::MakeEventSource<int>();
//
//	auto anyNumber = numbers1 | numbers2;
//
//	Observe(anyNumber, [] (int v) {
//		cout << "Number: " << v << endl;
//	});
//
//	numbers1 << 10 << 20 << 30;
//	numbers2 << 40 << 50 << 60;
//
//	cout << endl;
//}
//
//class Person : public ReactiveObject<D>
//{
//public:
//	VarSignal<int>	Age		= MakeVar(1);
//	Signal<int>		Health	= 100 - Age;
//	Signal<int>		Wisdom  = Age * Age / 100;
//
//	// Note: Initializing them directly uses the same lambda for both signals...
//	// compiler bug?
//	Observer	wisdomObs;
//	Observer	weaknessObs;
//
//	Person()
//	{
//		wisdomObs = Observe(Wisdom > 50, [] (bool isWise)
//		{
//			if (isWise)	cout << "I'll do it next week!" << endl;
//			else		cout << "I'll do it next month!" << endl;
//		});
//
//		weaknessObs = Observe(Health < 25, [] (bool isWeak)
//		{
//			if (isWeak)	cout << ":<" << endl;
//			else		cout << ":D" << endl;
//		});
//	}
//};
//
//void ObjectExample1()
//{
//	cout << "Object Example 1" << endl;
//
//	Person somePerson;
//
//	somePerson.Age <<= 30;
//	somePerson.Age <<= 60;
//	somePerson.Age <<= 90;
//
//	cout << "Health: " << somePerson.Health() << endl;
//	cout << "Wisdom: " << somePerson.Wisdom() << endl;
//
//	cout << endl;
//}
//
//void FoldExample1()
//{
//	cout << "Fold Example 1" << endl;
//
//	auto src = D::MakeEventSource<int>();
//	auto fold1 = Fold(0, src, [] (int v, int d) {
//		return v + d;
//	});
//
//	for (auto i=1; i<=100; i++)
//		src << i;
//
//	cout << fold1() << endl;
//
//	auto charSrc = D::MakeEventSource<char>();
//	auto strFold = Fold(std::string(""), charSrc, [] (std::string s, char c) {
//		return s + c;
//	});
//
//	charSrc << 'T' << 'e' << 's' << 't';
//
//	cout << "Str: " << strFold() << endl;
//}

#include "tbb/tick_count.h"

#include "react/common/Concurrency.h"

int main()
{
	SignalExample1();
	SignalExample2();
	//EventExample1();

	//ObjectExample1();

	//FoldExample1();

	react::BlockingCondition b;
	b.RunIfBlocked([] { printf("Test\n"); });

	//{
	//	int x = 0;

	//	auto t0 = tbb::tick_count::now();

	//	for (int i=0; i<1000000000; i+=3)
	//	{
	//		x += i;
	//	}

	//	auto t1 = tbb::tick_count::now();

	//	auto d = (t1 - t0).seconds();

	//	cout << x << endl;
	//	cout << d << endl;
	//}

	//{
	//	int x = 0;

	//	auto t0 = tbb::tick_count::now();

	//	for (int i=0; i<1000000000; i+=3)
	//	{
	//		auto l = [&x, i] {
	//			x += i;
	//		};
	//		l();
	//	}

	//	auto t1 = tbb::tick_count::now();

	//	auto d = (t1 - t0).seconds();

	//	cout << x << endl;
	//	cout << d << endl;
	//}

	//{
	//	auto x = D::MakeVar(0);

	//	auto t0 = tbb::tick_count::now();

	//	for (int i=0; i<1000000; i+=3)
	//	{
	//		x <<= i;
	//	}

	//	auto t1 = tbb::tick_count::now();

	//	auto d = (t1 - t0).seconds();

	//	cout << d << endl;
	//}

	return 0;
}