
#include "humid.h"
#include <clocks.h>
#include <tca.h>
#include <Arduino.h>
#include <avr/sleep.h>

bool HumidATtiny3216::_xtalWasEnabled;

/// Set whiile we're sampling
volatile static bool _sampling;

/// Number of counts, updated at end of sampling
volatile static uint16_t _counts;

//-----------------------------------------
void HumidATtiny3216::initEvSys()
{
    //SYNCCH0 PORTC_PIN3;
    EVSYS.SYNCCH0 = 0xA;

    //SYNCUSER0 SYNCCH0;
    EVSYS.SYNCUSER0 = 0x1;
}

//-----------------------------------------
void HumidATtiny3216::initTCA0()
{
    
    TCA0Control::reset( /*force*/ false );
    TCA0Control::clockSelect(TcaClock::Div1028);
    TCA0Control::eventAction(EventAction::PosEdge);
    TCA0Control::eventCountEnable(true);
}

//-----------------------------------------
void HumidATtiny3216::initRTC()
{
    RtcControl::prescale(RtcControl::Prescale::DIV4K); // 125ms / 8Hz RTC
    RtcControl::enableOvfInterrupt(true);
    RtcControl::runInSleep(true);

}
//-----------------------------------------
void HumidATtiny3216::init()
{
    initEvSys();
    initTCA0();
    initRTC();
    ClockControl::waitForXtal();
    RtcControl::clockXT32k();
}
//-----------------------------------------
void HumidATtiny3216::beginCounting()
{
    //Serial.write("4\n");
    TCA0Control::count(0);

    RtcControl::enable(false);
    RtcControl::clearInterruptFlags();
    RtcControl::clearPrescaler();
    RtcControl::period(7); //  1s
    RtcControl::enable(true);
    TCA0Control::enable(true);
    _sampling = true;
}
//-----------------------------------------
bool HumidATtiny3216::isCounting()
{
    return _sampling;
}
//----------------------------------------_
uint16_t HumidATtiny3216::endCounting()
{
    set_sleep_mode(SLEEP_MODE_IDLE);
    cli();

    if(!_sampling)
    {
        sei();
        return _counts;
    }

    sleep_enable();
    sei();
    do
    {
        sleep_cpu();
    } while(_sampling); //  possible some other interrupt woke us

    sleep_disable();

    if(_xtalWasEnabled)
        ClockControl::disableXtal();

    //Serial.printf("counts=%u\r\n", _counts);
    return _counts;
}
//-----------------------------------------
void HumidATtiny3216::wake()
{
    ClockControl::enableXtal(true, true);
}
    //-----------------------------------------
void HumidATtiny3216::sleep()
{
    ClockControl::disableXtal();
}
//-----------------------------------------

//----------------------------------------------------------------------------------
ISR(RTC_CNT_vect)
{
//    digitalWriteFast(Pins::LED,!digitalReadFast(Pins::LED));

    if ( (RTC.INTCTRL & RTC_OVF_bm) && (RTC.INTFLAGS & RTC_OVF_bm) )
    {
        if(_sampling)
        {
            _counts = TCA0Control::count();
            _sampling = false;
            RtcControl::enable(false);
            TCA0Control::enable(false);
        }
    }

    if ( (RTC.INTCTRL & RTC_CMP_bm) && (RTC.INTFLAGS & RTC_CMP_bm) )
    {
    }


    RTC.INTFLAGS = (RTC_OVF_bm | RTC_CMP_bm);
}
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------

