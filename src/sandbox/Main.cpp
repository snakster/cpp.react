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

int main()
{
	SignalExample1();
	SignalExample2();

	return 0;
}