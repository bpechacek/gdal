// $Revision: /main/5 $ $Modtime: 00/05/26 15:22:30 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  ucmpmeth.cpp                4-Jun-92            D.Lang
//
//  Compression method #0.                          
//
//   COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#include "ucmpmeth.h"
#include "wsitypedefs.h"

//-----------------------------------------------------------------------------

Uncompressed_method::~Uncompressed_method()
{
}

//-----------------------------------------------------------------------------

Compression_method * Uncompressed_method::clone() const
{
    return (Compression_method *)new Uncompressed_method(*this);
}

//-----------------------------------------------------------------------------

Cmp_size Uncompressed_method::uncompressed_length(
        const Byte*,
        Cmp_size in_size) 
{
    return in_size;
}

//-----------------------------------------------------------------------------

Cmp_size Uncompressed_method::uncompress(
        const Byte* in_ptr,
        Cmp_size in_size,
        Byte* out_ptr,
        Cmp_size max_out_size, 
        Cmp_size offset,
        Cmp_size * cmp_bytes_used_ptr)
{          
    //
    //  Figure out how many bytes we can safely copy from the input buffer
    //
    Long num_bytes_to_copy;

    if (max_out_size > (in_size-offset))
    {
        num_bytes_to_copy = in_size-offset;
    }
    else
    {
        num_bytes_to_copy = max_out_size;
    }

    //
    //  Copy the data from the input to the output buffer
    //
    memcpy(out_ptr, in_ptr, num_bytes_to_copy);

    if (cmp_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *cmp_bytes_used_ptr = num_bytes_to_copy;
    }

    return num_bytes_to_copy;
}

//-----------------------------------------------------------------------------

Cmp_size Uncompressed_method::compressed_length(
        const Byte*, 
        Cmp_size in_size,
        Compress_data_description *)  
{
    return in_size;
}

//-----------------------------------------------------------------------------

Cmp_size Uncompressed_method::compress(
        const Byte* in_ptr,
        Cmp_size in_size, 
        Byte* out_ptr,
        Cmp_size max_out_size, 
        Cmp_size offset,
        Cmp_size * raw_bytes_used_ptr,
        Compress_data_description *) 
{
    //
    //  Figure out how many bytes we can safely copy from the input buffer
    //
    Long num_bytes_to_copy;

    if (max_out_size > (in_size-offset))
    {
        num_bytes_to_copy = in_size-offset;
    }
    else
    {
        num_bytes_to_copy = max_out_size;
    }

    //
    //  Copy the data from the input to the output buffer
    //
    memcpy(out_ptr, in_ptr, num_bytes_to_copy);

    if (raw_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *raw_bytes_used_ptr = num_bytes_to_copy;
    }

    return num_bytes_to_copy;
}

