#include <iostream>

#include "react/Signal.h"
#include "react/propagation/SourceSetEngine.h"

using namespace react;
using namespace std;

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

	auto customFunc = (width,height,depth) >>= [] (int w, int h, int d) {
		return w * h * d * 1000;
	};

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

	// Note: Observer must be detached explicitly.
	// This is likely to change in the future.
}

int main()
{
	SignalExample1();
	SignalExample2();
	EventExample1();

	return 0;
}