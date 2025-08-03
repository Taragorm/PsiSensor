#ifndef _HS1101
#define _HS1101

#include <stdint.h>
#include <Telemetry.h>

//========================================================================
/**
    Handle HS1101 Humidity sensor; this bit just does the computation.

    @tparam T   Thermistor and Humidity sensor base class
 */
template<typename T>
class HS1101 : public T
{

public:
    enum class Status
    {
        Ok,
        HumidityLow, HumidityHigh,
        TempLow, TempHigh
    };
    //--------------------------------------------------------------------
    HS1101()
    {}
    //--------------------------------------------------------------------
    void setup()
    {}
    //--------------------------------------------------------------------
    /**
     * Get the raw (scaled) temperature from the temp lookup
     * table.
     */
    int16_t rawTemp(uint16_t adc) const
    {
        if(adc<=T::_therm_table_locount)
            return T::_therm_table[0];
        else if(adc>=T::_therm_table_hicount)
            return T::_therm_table[T::_therm_table_size-1];

        auto adc0 = adc - T::_therm_table_locount;
        auto ix = adc0 >> T::_therm_table_rbits;
        auto res = adc0 & T::_therm_table_rmask; // 0..31, 5bit

        auto bc = T::_therm_table+ix;
        auto lb = *bc++;
        auto hb = *bc;
        auto diff = (hb-lb)*res;
        auto adj = diff >> T::_therm_table_rbits;

        return lb + adj;
    }
    //--------------------------------------------------------------------
    int16_t interpol(const int16_t* row, uint16_t bucket, int16_t residue)
    {
        auto rh00 = row[bucket];
        auto rh01 = row[bucket + 1];

        auto rd = residue*(rh01-rh00)/T::_humid_table_stepH;

        return rh00+rd;
    }
    //--------------------------------------------------------------------
    /**
     * Do the main compute, a bilinear interpolation
     *
     * @param       countsHumid Humidity oscillator counts for sampling period
     * @param       tempRaw     Temp as from lookup table i.e. scaled
     * @param[out]  humidRaw    Humidity in RH%, scaled
     * @return                  Conversion status
     */
    Status computeRH(uint16_t countsHumid, int16_t tempRaw, int16_t& humidRaw)
    {
        //
        // can just do
        if(countsHumid <= T::_humid_table_locount)
        {
            humidRaw = 0;
            return Status::HumidityLow;
        }
        if(countsHumid >=T::_humid_table_hicount)
        {
            humidRaw = 100*T::_humid_table_scale;
            return Status::HumidityHigh;
        }

        Status status = Status::Ok;

        if(tempRaw <= T::_humid_table_tminsc)
        {
            tempRaw = T::_humid_table_tminsc;  // silly low temp
            status = Status::TempLow;
        }
        else if(tempRaw >= T::_humid_table_tmaxsc)
        {
            tempRaw = T::_humid_table_tmaxsc; // silly high temp
            status = Status::TempHigh;
        }


        auto tadj = tempRaw - T::_humid_table_tminsc;
        auto tb0  = tadj / T::_humid_table_stepTsc;
        auto tres = tadj % T::_humid_table_stepTsc;

        auto hrow = T::_hs1101_table[tb0++];
        auto hrow2 = T::_hs1101_table[tb0];

        auto fadj = countsHumid - T::_humid_table_locount;
        auto fb0  = fadj / T::_humid_table_stepH;
        auto fres = fadj % T::_humid_table_stepH;

        auto rh = interpol(hrow, fb0, fres);   // interpolate on row <= temp

        if(tres!=0)
        {
            // have a residue in temp, so do an interpolation
            // on the next temp row
            auto rh1 = interpol(hrow2, fb0, fres); // interpolate on row > temp

            // and then use that
            // to interpolate
            auto rhd = rh1 - rh;
            auto rhadj = tres*rhd/T::_humid_table_stepTsc;
            rh += rhadj;
        }
        humidRaw = rh;

        if(humidRaw <0)
            humidRaw =0;
        else if(humidRaw > T::_humid_max_raw)
            humidRaw =T::_humid_max_raw;

        return status;
    }
    //--------------------------------------------------------------------
};
//========================================================================



#endif