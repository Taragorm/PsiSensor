
/// HS1101 cap>humidity
#ifndef _HS1101_table_H
#define _HS1101_table_H

#include <stddef.h>
#include "InterpolatedLookup.h"

class HS1101Lookup 
    : public InterpolatedLookup1DScaledOffset<uint16_t,float, 20, 256, 162 >
{
    static const int16_t _hs1101_table[];

public:
    const size_t _hs1101_table_size = 20;
    
};

#endif
