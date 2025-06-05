/*
* Thermistor.cpp
*
* Created: 23/04/2024 08:14:23
* Author: Simon


  ------- VDrive
  |
 | | Rth
  |
  ------- Vsense
  |
 | | Rl
  |
  ------- 0V

*/

#define TRACE 0

#include "Thermistor.h"
//#include "io.h"
#include <math.h>
//#include <Print.h>
#include <stdint.h>

const uint32_t M1 = 0x1234fff;
const uint32_t M2 = ~M1;
const float  AZ = 273.15;

const char * FILENAME = "/cal.bin";
//----------------------------------------------------
// default constructor
Thermistor::Thermistor()
:   _majick1(M1),
    _rth(100.0*1000),
    _beta(3950),
    _rl(10.0*1000),
    _vAdcMax(1.0),
    _vDrive(3.3),
    _offset(0.0),
    _invert(false),
    _adcCountMax(1023),
    _majick2(M2)
{

}
//----------------------------------------------------
float Thermistor::voltsFromCounts(int c) const
{
    auto v =  _vAdcMax * c / _adcCountMax;
#if TRACE
    Serial.printf("counts %d -> %fV\r\n", c, v);
#endif
    return v;
}
//----------------------------------------------------
float Thermistor::tempFromResistance(float r) const
{
    auto it = 1/(25+AZ) + log(r/_rth)/_beta;
    auto t = 1/it - AZ;
#if TRACE
    Serial.printf("T = %fC\r\n", t);
#endif
    return t + _offset;
}
//----------------------------------------------------
float Thermistor::tempFromVolts(float v) const
{
    float rth;
    if(_invert)
    {
        // therm pulls down
        rth = _rl / (_vDrive/v -1);
    }
    else
    {
        // therm pulls up
        rth = _rl * (_vDrive/v -1);
    }

#if TRACE
    Serial.printf("Rth = %f\r\n", rth);
#endif
    return tempFromResistance(rth);
}
//----------------------------------------------------
bool Thermistor::isOk() const
{
    return _majick1 == M1 && _majick2 == M2;
}
//----------------------------------------------------
void Thermistor::dump(Print& out)
{
    //out.printf("Rth=%f Beta=%f\r\n", _rth, _beta);
    //out.printf("Rload=%f Invert=%d\r\n", _rl, _invert);
    //out.printf("Vadc=%f Vdrive=%f offs=%fC\r\n", _vAdcMax, _vDrive, _offset);
}
//----------------------------------------------------
float Thermistor::tempFromCounts(int c) const
{
    auto v = voltsFromCounts(c);
    auto t = tempFromVolts(v);
    return t;
}
//----------------------------------------------------
