/* 
* Thermistor.h
*
* Created: 23/04/2024 08:14:24
* Author: Simon
*/


#ifndef __THERMISTOR_H__
#define __THERMISTOR_H__

#include <math.h>
#include <stdint.h>

/**
 * Thermistor calculation based on float arithmetic.
 */
class ThermistorFloat
{
    const float _rth;
    const float _beta;
    const float _rl;
    const float _offset;
    const bool _invert;
    const int _adcCountMax;
    const float  AZ = 273.15;

public:
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	ThermistorFloat()
        : 
        _adcCountMax(1023),
        _rl(10 * 1000),
        _rth(100 * 1000),
        _beta(3950),
        _offset(0.0),
        _invert(false)
    {
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    float tempFromResistance(float v) const
    {
        auto it = 1 / (25 + AZ) + log(r / _rth) / _beta;
        auto t = 1 / it - AZ;
        return t + _offset;
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    float tempFromCounts(uint16_t c) const
    {
        if (c == 0 || c>_adcCountMax)
            return NAN; // counts can't be 0...

        if (_invert)
            c = _adcCountMax - c;

        float rth = _adcCountMax * _rl / c - _rl;

        return tempFromResistance(rth);
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void dump(class Print& out) const
    {
        out.printf("Rth=%f Beta=%f\r\n", _rth, _beta);
        out.printf("Rload=%f Invert=%d\r\n", _rl, _invert);
        out.printf("Vadc=%f Vdrive=%f offs=%fC\r\n", _vAdcMax, _vDrive, _offset);
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}; //Thermistor

#endif //__THERMISTOR_H__
