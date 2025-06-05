/* 
* Thermistor.h
*
* Created: 23/04/2024 08:14:24
* Author: Simon
*/


#ifndef __THERMISTOR_H__
#define __THERMISTOR_H__

#include <stdint.h>

struct Thermistor
{
    uint32_t _majick1;
    float _rth;
    float _beta;
    float _rl;
    float _vAdcMax;
    float _vDrive;
    float _offset;
    bool _invert;
    int _adcCountMax;
    uint32_t _majick2;
    
	Thermistor();

    float voltsFromCounts(int c) const;
    float tempFromResistance(float v) const;
    float tempFromVolts(float v) const;
    float tempFromCounts(int c) const;
    
    
    bool isOk() const;
    void dump(class Print& out);
    
}; //Thermistor

#endif //__THERMISTOR_H__
