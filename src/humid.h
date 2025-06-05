#ifndef HUMID_H
#define HUMID_H

#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>

//=============================================
/**
 * Facet class for IotStation.
 * members are static, so we can use an ISR if
 * we want/need to later on.
 
   - Uses events, TCA0 and RTC/PIT
   - RTC is set to use 32kHz Xtal, with 8k prescaler -> 250ms ticks
 */
class HumidATtiny3216
{
    static bool _xtalWasEnabled;
    
    //-----------------------------------------
    static void initEvSys();
    //-----------------------------------------
    static void initTCA0();
    //-----------------------------------------
    static void initRTC();
    //-----------------------------------------
public:
    static void selectAccurateClock();
    static void selectInaccurateClock();
    //-----------------------------------------
    static void init();
    //-----------------------------------------
    /**
        Begin event counting.
        @note Uses PIT
    */
    static void beginCounting();
    //-----------------------------------------
    /**
     * @return true if sampling is (still) underway                                                                     
     */
    static bool isCounting();
    //-----------------------------------------
    /**
     * End event count.
       - PIT is left disabled
       
       @return number of events
     */
    static uint16_t endCounting();
    //-----------------------------------------
    static void wake();
    static void sleep();
    //-----------------------------------------
};
//=============================================

#endif