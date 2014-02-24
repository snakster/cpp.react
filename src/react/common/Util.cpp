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
	struct tm  tstruct;
	char       buf[80];
    __time64_t now;

	errno_t err = _localtime64_s(&tstruct, &now); 

    strftime(buf, sizeof(buf), "%Y-%m-%d___%H.%M.%S", &tstruct);

    return buf;
}

}