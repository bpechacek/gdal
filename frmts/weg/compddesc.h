// $Revision: /main/5 $ $Modtime: 00/06/16 12:00:57 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compddesc.H                17-Nov-95            PLDurante
//
//  Helper class to support Compress methods.                          
//
//    COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#ifndef COMPDDESC_H
#define COMPDDESC_H

//-----------------------------------------------------------------------------
//
//  CLASS
//
//      Compress_data_description
// 
//  REUSABILITY (Foundation, Framework, Weather Workstation)
//
//      Foundation
//
//  DESCRIPTION
//
//      Helper class to support Compression methods.
//
//  USAGE
//
//      This is a helper class which is used to pass information about 
//      the data being compressed into the compression methods.
//
//-----------------------------------------------------------------------------

class Compress_data_description
{
  public:

    Compress_data_description();

    Compress_data_description(
            long x_dimension, 
            long y_dimension,
            short bits_per_sample);

    Compress_data_description(const Compress_data_description&);

    Compress_data_description& operator=(
            const Compress_data_description&);

    virtual ~Compress_data_description();

    //
    // Accessor Methods
    //
    void x_dimension(const long new_x_dimension);
    long x_dimension() const;

    void y_dimension(const long new_y_dimension);
    long y_dimension() const;

    void bits_per_sample(const short new_bits_per_sample);
    short bits_per_sample() const;

  private:

    long my_x_dimension;
    long my_y_dimension;
    short my_bits_per_sample;
};

//-----------------------------------------------------------------------------
//
//  Inlines
//
//-----------------------------------------------------------------------------

inline void Compress_data_description::x_dimension(const long new_x_dimension)
{
    my_x_dimension = new_x_dimension;
}

//-----------------------------------------------------------------------------

inline long Compress_data_description::x_dimension() const
{
    return(my_x_dimension);
}

//-----------------------------------------------------------------------------

inline void Compress_data_description::y_dimension(const long new_y_dimension)
{
    my_y_dimension = new_y_dimension;
}

//-----------------------------------------------------------------------------

inline long Compress_data_description::y_dimension() const
{
    return(my_y_dimension);
}

//-----------------------------------------------------------------------------

inline void Compress_data_description::bits_per_sample(
        const short new_bits_per_sample)
{
    my_bits_per_sample = new_bits_per_sample;
}

//-----------------------------------------------------------------------------

inline short Compress_data_description::bits_per_sample() const
{
    return(my_bits_per_sample);
}

#endif
