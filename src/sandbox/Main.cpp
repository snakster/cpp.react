#include <iostream>

#include "react/Signal.h"
#include "react/propagation/SourceSetEngine.h"

using namespace react;
using namespace std;

REACTIVE_DOMAIN(TestDomain, SourceSetEngine);

void SignalExample()
{
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
}

int main()
{
	SignalExample();

	return 0;
}