#include <intrin.h>
#include <iostream>
using namespace std;

int main( int argc, char* argv[] )
{
    unsigned long mask = 0x1001;
    unsigned long index;
    unsigned char isNonzero;

    // cout << "Enter a positive integer as the mask: " << flush;
    // cin >> mask;
    isNonzero = _BitScanForward( &index, mask );
    if ( isNonzero )
    {
        cout << "Mask: " << mask << " Index: " << index << endl;
    }
    else
    {
        cout << "No set bits found.  Mask is zero." << endl;
    }
#define SLOT_TO_CHUNK( x ) ( ( x + 63u ) & ~( 63u ) )

    printf( "%#x\n", ~63u );
}
