##
# Use bisection to create a humidity lookup table
import math 
import datetime

## Thermistor Sresistor
RTH = 100000

## Thermnistor Beta
BTH = 3950

# Count period in seconds
P = 1.0 # let's just use Hz

SCALE = 256

TCOEFF = 0.04 # pf/C

LO = 162
HI = 202
STEP = 2
COUNT = (HI-LO)/STEP

AZ=273.15
T1=AZ+25



when = datetime.datetime.utcnow

#-----------------------------------------------------------    
def capAtTemp(temp):    
    ccomp = temp * TCOEFF # 
    return 180 + ccomp

##
# return C for q given rh
#def func(rh,tv):
#    return 180*(1.25e-7*rh*rh*rh -1.36e-5*rh*rh+2.19e-3*rh+9.0e-1) - tv
 
#===============================================================
#
class Sensor:
    def __init__(self, temp, value):
        self.temp = temp
        self.c = capAtTemp(temp)
        self.value = value
    
    def __call__(self, rh):
        return self.c*(1.25e-7*rh*rh*rh -1.36e-5*rh*rh+2.19e-3*rh+9.0e-1) - self.value
#===============================================================

def samesign(a, b):
    return a * b > 0

def bisect(func, low, high):
    'Find root of continuous function where f(low) and f(high) have opposite signs'

    #print(tv, func(low,tv), func(high,tv))
    
    assert not samesign(func(low), func(high)), "{0} {1}".format(func(low), func(high))

    for i in range(54):
        midpoint = (low + high) / 2.0
        if samesign(func(low), func(midpoint)):
            low = midpoint
        else:
            high = midpoint

    return midpoint
    
    
def rhFromCapTemp(cap,temp):
    return  bisect(Sensor(temp,cap), -200, 200)
    
def uinject(s, u):
    if "." in s:
        return s.replace('.',u)
    else:
        s.append(u)

def fmt(r):
    if r>1e6:
        return uinject("{:.1f}".format(r/1e6), "M")
    if r>1e3:
        return uinject("{:.1f}".format(r/1e3), "k")
    if r>10:
        return uinject("{:.1f}".format(r), "R")
    if r>1:
        return uinject("{:.1f}".format(r), "R")
    
    # subohmic
    return uinject("{:.2f}".format(r), "R")

#-----------------------------------------------------------    
class Info:
    def __init__(self, value, comment):
        self.value = value
        self.comment = comment
        self.comma = ", "

    def v(self,sc=100):
        return "{.8d}".format(int(self.value*sc+0.5))
#-----------------------------------------------------------    
def merge(*dict_args):
    result = {}
    for dictionary in dict_args:
        result.update(dictionary)
    return result
#-----------------------------------------------------------    
class Generator:
    def __init__(self, **kwargs):
        self.name = None
        self.tmin = -10
        self.tmax = 110
        self.tstep = 10
        self.tscale = 127
        self.tstepsc = self.tstep*self.tscale
        self.ttable = []
        self.tadcbits = 10
        self.tresiduebits = 5
        self.hscale=256
        self.hmaxraw = self.hscale*100
        self.rsense = 100000
        self.rth= 100000
        self.beta = 3950
        self.volts = 3.3
        self.ROsc=402700 # 10kHz nominal
        self.cStrayPf = 0

        for k, v in kwargs.items():
            #assert( k in self.__class__.__allowed )
            setattr(self, k, v)

        # computed
        self.tadcmax = (1<<self.tadcbits)-1

        rth = fmt(self.rth)
        rsen = fmt(self.rsense)
        tlo = "{}".format(self.tmin).replace("-","_")
        stray = ""
        if self.cStrayPf:
            stray = "Cstray{}".format(self.cStrayPf).replace(".","_")

        if not self.name:
            self.name = "HS1101Rt{rth}Rs{rsen}Tl{tlo}Th{tmax}{stray}".format(**merge(vars(self),globals(),locals()))

    #---------------------------------------------------------------------------------------------------------------------------    
    ##
    # turn counts into a capacitance in pF
    def computeC(self,counts):
        f = counts/P    
        effC = 0.725/(self.ROsc*f) 
        
        return effC*1e12 - self.cStrayPf # make pF

    #-----------------------------------------------------------    
    ##
    # compensate cap to C25 from current temperature
    # @param cap in pf
    # @param temp in C
    def compensateT(self, cap, temp):
        return cap - TCOEFF*(temp-25)
    #-----------------------------------------------------------    
    # #
    # compensate temp to C25
    # @param cap in pf
    # @param freq in Hz
    #def compensateF(cap, freq):
    #    return cap / (1.027 - math.log(freq/1000))
    #-----------------------------------------------------------    
    ##
    # Work out RH% from frequency capture and sensor
    # temperature
    def computeRH(self, freq, temp):
        cap = self.computeC(freq)
        rh = rhFromCapTemp(cap, temp)
        print(freq, temp, cap, rh)
        return rh
    #-----------------------------------------------------------    
    def freqForRhTemp(self, rh, temp):
        sensor = Sensor(temp,0)
        cap = (sensor(rh)+ self.cStrayPf)*1e-12 
        f = 0.725/(self.ROsc*cap)
        return f

    #---------------------------------------------------------------------------------------------------------------------------    
    def resForCounts(self, ac):
        vfrac = ac/self.tadcmax        
        rth = self.rsense*(1/vfrac-1)
        return rth
    #---------------------------------------------------------------------------------------------------------------------------    
    ##
    # Temperatue given the actual ADC count
    #
    def tempForCounts(self, ac):            
        rth = self.resForCounts(ac)
        #print(ac, vfrac, rth)
        return self.tempForRes(rth)
    #---------------------------------------------------------------------------------------------------------------------------    
    ##
    # Compute thermistor resistance at temp
    def resAtTemp(self, t):
        x = (1.0/T1-1.0/(t+AZ))
        return self.rth/math.exp(self.beta*x)        
    #---------------------------------------------------------------------------------------------------------------------------    
    ##
    # Compute temp for given resistance
    #
    def tempForRes(self, r):
        t2 = 1/(1/T1- math.log(self.rth/r)/self.beta)
        return t2-AZ        
    #---------------------------------------------------------------------------------------------------------------------------    
    ##
    # Actual ADC count for any given temp
    #
    def countsAtTemp(self, t):
        vfrac = self.rsense/(self.rsense+self.resAtTemp(t))    
        cfrac = self.tadcmax*vfrac    
        return int(cfrac+0.5)
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def genTempTable(self):
        # we don't need all the buckets, so we just
        # make enough to hold min/max
        lc = self.countsAtTemp(self.tmin)
        self.tlowbucket = lc >> self.tresiduebits
        hc = self.countsAtTemp(self.tmax)
        self.thibucket = (hc>> self.tresiduebits)+1
        for b in range(self.tlowbucket, self.thibucket+1):
            c = b<<self.tresiduebits
            r = self.resForCounts(c)
            t = self.tempForCounts(c)
            v = (self.volts*c)/self.tadcmax
            print("bucket", b)
            i = Info(int(t*self.tscale+0.5), 
                     " // [{2:2d}]{0:6.2f}°C {1:6d}cts {3:.3f}V res={4:.1f}k"
                        .format(t, c, b-self.tlowbucket, v, r/1000)
                     )
            self.ttable.append(i)
        i.comma = "  "        
        self.tsize = self.thibucket-self.tlowbucket+1
        self.tlocount = self.tlowbucket<<self.tresiduebits
        self.thicount = self.thibucket<<self.tresiduebits
        self.trmask = (1<<self.tresiduebits)-1
        self.tscalf = 1.0/self.tscale
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def emitTempTable(self):
        self.cf.write("""const int16_t {0}::_tampTable = {{\n""".format(self.name))

        for i in self.ttable:
            self.cf.write(""" {0}{1}{2}\n""".format(i.value, i.comma, i.comment))

        self.cf.write("""}} // {0}\n""".format(self.name))
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def initH(self):

        self.hf = open(self.name + ".h","w", encoding='utf-8')
        self.hf.write("""/** @file
  @@brief HS1101 humidity sensor and thermistor
  Generated by H!1101.py {when}
  */
#ifndef _{name}_table_H
#define _{name}_table_H

#include <stdint.h>
#include "HS1101.h"

//=========================================================================================================================
/** @brief
 * Data class for HS1101
 */     
class {name}Data 
{{
public:
    static const uint16_t _therm_table_size    = {tsize:5d}; ///< Entries in thermistor table
    static const uint16_t _therm_table_scale   = {tscale:5d}; ///< Thermistor table values are multiplied by this value
    static const uint16_t _therm_table_locount = {tlocount:5d}; ///< ADC count for lowest bucket
    static const uint16_t _therm_table_hicount = {thicount:5d}; ///< ADC count for highest bucket
    static const uint16_t _therm_table_rbits   = {tresiduebits:5d}; ///< Bits in the residue
    static const uint16_t _therm_table_rmask   = {trmask:5d}; ///< Mask for the residue


    static const uint16_t _humid_table_sizeT   = {tcsize:5d}; ///< Entries in dim0 of Humidity table (temp index)
    static const int16_t  _humid_table_tmin    = {tmin:5d}; ///< low temp in table (temp for first row)
    static const int16_t  _humid_table_tmax    = {tmax:5d}; ///< hi temp in table (temp for last row)
    static const int16_t  _humid_table_tminsc  = {tminsc:5d}; ///< low temp, scaled as per thermistor table (x{tscale})
    static const int16_t  _humid_table_tmaxsc  = {tmaxsc:5d}; ///< hi temp scaled as per thermistor table (x{tscale})
    static const int16_t  _humid_table_stepT   = {tstep:5.0f}; ///< Temperature distance between two rows
    static const int16_t  _humid_table_stepTsc = {tstepsc:5.0f}; ///< Temperature distance between two rows, scaled (x{tscale})
                      
    static const uint16_t _humid_table_sizeH   = {fcsize:5d}; ///< Entries in dim1 of Humidity table (freq indexed)
    static const uint16_t _humid_table_locount = {fcmin:5d}; ///< Offset of first bucket (counts in interval)
    static const uint16_t _humid_table_hicount = {fcmax:5d}; ///< Offset of last bucket (counts in interval)
    static const int16_t  _humid_table_stepH   = {fcstep:5.0f}; ///< # counts between column values
    static const uint16_t _humid_table_scale   = {hscale:5d}; ///< Humidity table values are multiplied by this value
    static const int16_t _humid_max_raw        = {hmaxraw:5d}; ///< Humidity table values are multiplied by this value
    
    // CStray =  {cStrayPf}Pf

    /// Scale a raw temp to °C
    constexpr static double scaleTemp(int16_t raw) {{ return raw * {tscalf}; }}
                      
    /// Scale a raw RH to RH% 
    constexpr static double scaleHumid(int16_t raw) {{ return raw * {hscalf}; }}
                      
protected:                      
    static const int16_t  _therm_table[_therm_table_size];
    static const int16_t  _hs1101_table[_humid_table_sizeT][_humid_table_sizeH];

    
}};

//=========================================================================================================================
/**
 Concrete HS1101 class with logic included
 */
class {name} : public HS1101<{name}Data>
{{
}};
//=========================================================================================================================
                      
#endif
""".format(**merge(vars(self),globals())))
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def finishH(self):
        self.hf.close()

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def initCPP(self):
        self.cf = open(self.name + ".cpp","w", encoding='utf-8')
        self.cf.write("""/// HS1101 cap>humidity

#include "{name}.h"

""".format(**merge(vars(self),globals())))    

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def finishCPP(self):
        self.cf.close()

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def genHumidTable(self):
        self.htable = []
        for t in range(self.tmin, self.tmax+self.tstep, self.tstep):
            data = []
            ix = len(self.htable)
            self.htable.append(data)
            for c in range(self.fcmin, self.fcmax+self.fcstep, self.fcstep):
                rh = self.computeRH(c,t)
                ix2 = len(data)
                cap = (1.8e-6/c)*1e12
                i = Info( int(rh*self.hscale+0.5), " // [{ix},{ix2}] {rh:6.2f}RH% {c:5d}cts {cap:5.2f}pF @{t:6.2f}°C".format(**vars()) )
                data.append(i)
            self.tcsize = len(self.htable)
            self.fcsize = len(data)
            self.hscalf = 1.0/self.hscale
            self.tminsc = self.tmin*self.tscale
            self.tmaxsc = self.tmax*self.tscale # scale from temp table
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def emitTemp(self):
        self.cf.write("""
const int16_t {name}Data::_therm_table[_therm_table_size] = 
{{
""".format(**merge(vars(self),globals())))   
        for i in self.ttable:
            self.cf.write( "  {0:6d},{1}\n".format(i.value, i.comment) )
        self.cf.write("};\n")
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def emitHumid(self):
        self.cf.write("""
const int16_t {name}Data::_hs1101_table[_humid_table_sizeT][_humid_table_sizeH] = 
{{
""".format(**merge(vars(self),globals())))   
        for it in self.htable:
            self.cf.write("  {\n")
            for i in it:
                self.cf.write( "    {0:6d},{1}\n".format(i.value, i.comment) )
            self.cf.write("  },")
        self.cf.write("\n};\n")
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def generate(self):
        self.genTempTable()
        print("counts0@-10", self.freqForRhTemp(0,-10))
        #print("counts0@0", freqForRhTemp(0,0))
        #print("counts0@50", freqForRhTemp(0,50))
        print("counts0@100", self.freqForRhTemp(0,100))
              
        #print("counts50@-10", freqForRhTemp(50,-10))
        #print("counts50@0", freqForRhTemp(50,0))
        #print("counts50@50", freqForRhTemp(50,50))
        #print("counts50@100", freqForRhTemp(50,100))
              
        print("counts100@-10", self.freqForRhTemp(100,-10))
        #print("counts100@0", freqForRhTemp(100,0))
        #print("counts100@50", freqForRhTemp(100,50))
        print("counts100@100", self.freqForRhTemp(100,100))

        #
        # Based on the output of the above
        self.fcmin =  8500 # counts100@100 3637.3339381412734
        self.fcmax = 10800 # counts0@-10   4597.754930227302
        self.fcstep = 100
        self.genHumidTable()

        self.initH()
        self.finishH()
        self.initCPP()
        self.emitTemp()
        self.emitHumid()
        self.finishCPP()
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#-----------------------------------------------------------    
if __name__ == "__main__":
    g = Generator(rsense=150000)
    print(
            "R25=", g.resAtTemp(25), 
            " T100k=", g.tempForRes(100000),
            " rfc=", g.resForCounts(614)
         )

    g.generate()
    g = Generator()
    g.generate()

    g = Generator(rsense=150000+2500, tmax=50, cStrayPf=7.2)
    g.generate()
