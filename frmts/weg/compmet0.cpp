// $Revision: /main/6 $ $Modtime: 00/05/23 13:42:43 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet0.cpp                4-Jun-92            D.Lang
//
//  Compression method #0.                          
//
//   COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#include "compmet0.h"
#include "wsitypedefs.h"

//-----------------------------------------------------------------------------

Compression_method0::~Compression_method0()
{
}

//-----------------------------------------------------------------------------

Compression_method * Compression_method0::clone() const
{
    return (Compression_method *)new Compression_method0(*this);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method0::uncompressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size) 
{
//-----------------------------------------------------------------------------
    //
    //  Determine length of uncompressed line.
    //
    //  WSI compression scheme #0 (8 bit).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //   (Hex)
    // -Packed Data-      --------Output Data-------------------
    //     nl             If n= 0..C  then output run of hl, length n+1
    //
    //
//-----------------------------------------------------------------------------

    Word     runlength;
    Word     linelength = 0;
    Cmp_size in_idx = 0;

    while (in_idx < in_size)
    {        
        runlength = in_ptr[in_idx++] >> 4;
        linelength += runlength + 1;
    }

    return (linelength);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method0::uncompress(
        const Byte* in_ptr,
        Cmp_size in_size,
        Byte* out_ptr,
        Cmp_size max_out_size, 
        Cmp_size offset, 
        Cmp_size * cmp_bytes_used_ptr)
{          
//-----------------------------------------------------------------------------
    //
    //  Uncompressed line.
    //
    //  WSI compression scheme #0 (8 bit).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //   (Hex)
    // -Packed Data-      --------Output Data-------------------
    //     nl             If n= 0..C  then output run of hl, length n+1
    //
    //
//-----------------------------------------------------------------------------

    Word      runlength;
    Cmp_size  in_idx       = 0;
    Cmp_size  out_idx      = 0;
    Byte      pixel;

    if (offset != 0)
    {
        //
        // TODO: do something useful.
        //
        return BAD_CMP_SIZE;  // RETURN:  Bad compression size.
    }
    while (in_idx < in_size)
    {        
        pixel = in_ptr[in_idx++];
        runlength = pixel >> 4;
        pixel &= 0xf;

        runlength++;

        while (runlength-- != 0 && 
          out_idx < max_out_size) 
        {
            out_ptr[out_idx++] = pixel;
        }                
    }

    if (cmp_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *cmp_bytes_used_ptr = in_idx;
    }

    return (out_idx);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method0::compressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size,
        Compress_data_description *)  
{
//-----------------------------------------------------------------------------
    //
    // Determine length of compressing data using scheme #0.
    //
    // Compression scheme #0 (4bit data).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //
    // -Packed Data-      --------Output Data-------------------
    //     nl             Output run of byte (hl), length n+1
    //
    //
//-----------------------------------------------------------------------------

    Cmp_size  out_idx  = 0;
    Word      count    = 0;
    Byte      pixel;

    while (in_size--)
    {
        pixel = *in_ptr++;
        count = 0;

        while (in_size && *in_ptr == pixel && count < 0xfffe)
        {
            count++;
            in_size--;
            in_ptr++;
        }

        out_idx += (count / 16);
        count %= 16;
        out_idx++;
    }

    return (out_idx);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method0::compress(
        const Byte* in_ptr,
        Cmp_size in_size, 
        Byte* out_ptr,
        Cmp_size max_out_size, 
        Cmp_size offset,
        Cmp_size * raw_bytes_used_ptr,
        Compress_data_description *) 
{
//-----------------------------------------------------------------------------
    //
    // Compress data using 8 bit compression scheme #0.
    //
    // Compression scheme #0 (4bit data).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //
    // -Packed Data-      --------Output Data-------------------
    //     nl             Output run of byte (hl), length n+1
    //
//-----------------------------------------------------------------------------

    Cmp_size  out_idx  = 0;
    Word      count    = 0;
    Byte      pixel;

    if (offset != 0)
    {
        //
        // TODO: do something useful. 
        //
        return BAD_CMP_SIZE;  // RETURN: Bad compression size.
    }
    while (in_size--)
    {
        pixel = *in_ptr++;
        count = 0;

        while (in_size && *in_ptr == pixel && count < 15)
        {
            count++;
            in_size--;
            in_ptr++;
        }

        if (out_idx >= max_out_size) break; // !! BREAK !!
        pixel = (count << 4) | (pixel & 0xf);

        out_ptr[out_idx++] = pixel;
    }

    if (raw_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *raw_bytes_used_ptr = out_idx;
    }

    return (out_idx);
}

