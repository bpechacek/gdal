// $Revision: /main/4 $ $Modtime: 00/05/23 13:42:19 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compddesc.cpp                17-Nov-95            PLDurante
//
//  Compress data description class.                          
//
//   COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#include "compddesc.h"

//-----------------------------------------------------------------------------
//
// Null constructor:
//
//-----------------------------------------------------------------------------

Compress_data_description::Compress_data_description() : 
    my_x_dimension(0),
    my_y_dimension(0),
    my_bits_per_sample(0)
{
}

//-----------------------------------------------------------------------------
//
// Standard constructor:
// 
//-----------------------------------------------------------------------------

Compress_data_description::Compress_data_description(
        long x_dimension, 
        long y_dimension,
        short bits_per_sample) :
    my_x_dimension(x_dimension),
    my_y_dimension(y_dimension),
    my_bits_per_sample(bits_per_sample)
{
}

//-----------------------------------------------------------------------------
//
// Destructor
//
//-----------------------------------------------------------------------------

Compress_data_description::~Compress_data_description()
{
}

//-----------------------------------------------------------------------------
//
// Copy constructor
//
//-----------------------------------------------------------------------------

Compress_data_description::Compress_data_description(
        const Compress_data_description& other) :
    my_x_dimension(other.my_x_dimension),
    my_y_dimension(other.my_y_dimension),
    my_bits_per_sample(other.my_bits_per_sample)
{
}

//-----------------------------------------------------------------------------
//
// Assignment
//
//-----------------------------------------------------------------------------

Compress_data_description& Compress_data_description::operator=(
        const Compress_data_description& other)
{
    if (this != &other)
    {
        my_x_dimension      = other.my_x_dimension;
        my_y_dimension      = other.my_y_dimension;
        my_bits_per_sample  = other.my_bits_per_sample;
    }

    return *this;
}
