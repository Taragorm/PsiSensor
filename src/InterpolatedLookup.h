/** @file
 */
 
 #ifndef THERMISTOR_H
 #define THERMISTOR_H

 #include <stdint.h>

 //-----------------------------------------------
 //-----------------------------------------------
template< typename TI, unsigned TOTALBITS, unsigned RESIDUEBITS>
class BitPartitioner
{
public:
    typedef TI index_t;
    const unsigned totalBits = TOTALBITS;
    const unsigned tableBits = TOTALBITS - RESIDUEBITS;
    const unsigned residueBits = RESIDUEBITS;

    //---------------------------------------------------------
    static void partition(TI v, TI& bucket, TI& residue)
    {
        bucket = v >> RESIDUEBITS;
        residue = v & maxResidue();
    }

    //---------------------------------------------------------
    static constexpr TI maxResidue() { return (1 << RESIDUEBITS) - 1; }
    //---------------------------------------------------------
    static constexpr TI tableSize() { return 1 << (TOTALBITS - RESIDUEBITS); }
    //---------------------------------------------------------
};
//-----------------------------------------------
//-----------------------------------------------
/**
 */
template<typename TI, unsigned BUCKETS, unsigned RESIDUE>
class ScaledPartitioner
{
public:
    typedef TI index_t;

    //---------------------------------------------------------
    static void partition(TI v, TI& bucket, TI& residue)
    {
        bucket = v / RESIDUE;
        residue = v % RESIDUE;
    }

    //---------------------------------------------------------
    static constexpr TI maxResidue() { return RESIDUE - 1; }
    //---------------------------------------------------------
    static constexpr TI tableSize() { return BUCKETS; }
    //---------------------------------------------------------
};
//-----------------------------------------------
//-----------------------------------------------
/**
 */
template<typename TI, unsigned BUCKETS, unsigned RESIDUE, unsigned OFFSET>
class ScaledOffsetPartitioner
{
public:
    typedef TI index_t;

    //---------------------------------------------------------
    static void partition(TI v, TI& bucket, TI& residue)
    {
        v -= OFFSET;
        bucket = v / RESIDUE;
        residue = v % RESIDUE;
    }

    //---------------------------------------------------------
    static constexpr TI maxResidue() { return RESIDUE - 1; }
    //---------------------------------------------------------
    static constexpr TI tableSize() { return BUCKETS; }
    //---------------------------------------------------------
};
//-----------------------------------------------
//-----------------------------------------------
class ILookup
{
public:
    virtual int32_t lookupRaw(int32_t v) const = 0;
    virtual int32_t lookupScale() const = 0;
    virtual float lookupScaleFactor() const = 0;
};
//-----------------------------------------------
//-----------------------------------------------
 /**
     1D lookup w

     @tparam TT Table intrinsic type
     @tparam TR Table real type; i.e. type table uses for storage
     @tparam TP Partitioner type

  */
 template<typename TT, typename TR, typename TP>
 class InterpolatedLookup1D : public ILookup
 {
 protected:
     const TT* _table;
     const TR _scale;
     const float _scaleFactor;
 public:
     //----------------------------------------------------------
     InterpolatedLookup1D(
         const TT* table,
         TR scale
     )
         :  _table(table),
            _scale(scale),
            _scaleFactor(1.0/scale)
     {}
     //----------------------------------------------------------
     /// Get raw value in first entry - lower limit of reasonably accurate values
     TT loRaw() const { return _table[0]; }
     //----------------------------------------------------------
     /// Get raw value in last entry - upper limit of reasonably accurate values
     TT hiRaw() const { return _table[ TP::tableSize() - 1]; }
     //----------------------------------------------------------
     /**
         @param tv Value to scale
         @return scalked raw value
      */
     TR scale(TT tv) const { return _scale * tv; }
     //----------------------------------------------------------
     /// Get scaled value in first entry - lower limit of reasonably accurate values
     TR lo() const { return scale(_table[0]); }
     //----------------------------------------------------------
     /// Get scaled value in last entry - upper limit of reasonably accurate values
     TR hi() const { return scale(_table[TP::tableSize() - 1]); }
     //----------------------------------------------------------
     static TT interpol(TT t0, TT t1, typename TP::index_t residue)
     {
         auto diff = t1 - t0;
         return t0 + ((diff * residue + TP::maxResidue() / 2) / TP::maxResidue() );
     }
     //----------------------------------------------------------    
     static TT interpol(const TT* row, typename TP::index_t index, typename TP::index_t residue)
     {
         row += index;
         auto t0 = *row++;
         auto t1 = *row;
         return interpol(t0, t1, residue);
     }
     //----------------------------------------------------------    
     /**
         Get the raw value for a given count.
         @param count ADC counts
         @return Interpolated value in table units
      */
     TT raw(typename TP::index_t count) const
     {
         typename TP::index_t bucket, residue;

         TP::partition(count, bucket, residue);


         if (bucket < 1)
             return loRaw();

         if (bucket >= TP::tableSize() )
             return hiRaw();

         auto bp = _table + bucket;
         auto lv = *bp++;
         auto hv = *bp;
         auto diff = hv - lv; // diff in table units
         auto corr = residue * diff / TP::maxResidue();
         return lv + corr;
     }
     //----------------------------------------------------------
     /**
         Get the scaled value for a given count.
         @param count ADC counts
         @return Interpolated value in table units
      */
     TR value(uint16_t count) const
     {
         return scale(raw(count));
     }
     //----------------------------------------------------------
     /**
     * Ilookup method
     */
     virtual int32_t lookupRaw(int32_t v) const { return raw(v); }
     /**
     * Ilookup method
     */
     virtual int32_t lookupScale() const { return _scale;  }
     virtual float lookupScaleFactor() const { return _scaleFactor; }

 };
 //-----------------------------------------------
 //-----------------------------------------------
 /**
     1D lookup with a table built around a number of bits

     @tparam TT Table intrinsic type
     @tparam TR Table real type
  */
 template<typename TT, typename TR, unsigned TOTALBITS, unsigned RESIDUEBITS>
 class InterpolatedLookup1DBits
     : public InterpolatedLookup1D<
     TT,
     TR,
     BitPartitioner<uint16_t, TOTALBITS, RESIDUEBITS>
     >
 {
     typedef InterpolatedLookup1D<
         TT,
         TR,
         BitPartitioner<uint16_t, TOTALBITS, RESIDUEBITS>
     > base_t;


 public:
     //----------------------------------------------------------
     InterpolatedLookup1DBits(
         const TT* table,
         TR scale
     )
         : base_t(table, scale)
     {}
     //----------------------------------------------------------

 };

 //-----------------------------------------------
 //-----------------------------------------------
  /**
      1D lookup with a table built around a scale

      @tparam TT Table intrinsic type
      @tparam TR Table real type
   */
 template<typename TT, typename TR, unsigned BUCKETS, unsigned RESIDUE>
 class InterpolatedLookup1DScaled
     : public InterpolatedLookup1D<
     TT,
     TR,
     ScaledPartitioner<uint16_t, BUCKETS, RESIDUE>
     >
 {
     typedef InterpolatedLookup1D<
         TT,
         TR,
         ScaledPartitioner<uint16_t, BUCKETS, RESIDUE>
     > base_t;


 public:
     //----------------------------------------------------------
     InterpolatedLookup1DScaled(
         const TT* table,
         TR scale
     )
         : base_t(table, scale)
     {}
     //----------------------------------------------------------

 };

 //-----------------------------------------------
 //-----------------------------------------------
   /**
       1D lookup with a table built around a scale

       @tparam TT Table intrinsic type
       @tparam TR Table real type
    */
 template<typename TT, typename TR, unsigned BUCKETS, unsigned RESIDUE, unsigned OFFSET>
 class InterpolatedLookup1DScaledOffset
     : public InterpolatedLookup1D<
     TT,
     TR,
     ScaledOffsetPartitioner<uint16_t, BUCKETS, RESIDUE, OFFSET>
     >
 {
     typedef InterpolatedLookup1D<
         TT,
         TR,
         ScaledOffsetPartitioner<uint16_t, BUCKETS, RESIDUE, OFFSET>
     > base_t;


 public:
     //----------------------------------------------------------
     InterpolatedLookup1DScaledOffset(
         const TT* table,
         TR scale
     )
         : base_t(table, scale)
     {}
     //----------------------------------------------------------

 };



#endif
 