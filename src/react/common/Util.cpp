
//          Copyright Sebastian Jeckel 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "react/common/Util.h"

namespace react {

//assumes little endian
void PrintBits(size_t const size, void const* const p)
{
    unsigned char *b = (unsigned char*) p;
    unsigned char byte;

    for (int i=size-1; i>=0; i--)
    {
        for (int j=7; j>=0; j--)
        {
            byte = b[i] & (1<<j);
            byte >>= j;
            printf("%u", byte);
			if (!(j & 0x1))
				printf(" ");
        }
    }
}

const std::string CurrentDateTime()
{
	char       buf[80];
	time_t     now = time(0);
	struct tm  tstruct = *localtime(&now);
 
	strftime(buf, sizeof(buf), "%Y-%m-%d___%H.%M.%S", &tstruct);
 
	return buf;
}

}