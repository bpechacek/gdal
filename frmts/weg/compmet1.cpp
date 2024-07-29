// $Revision: /main/6 $ $Modtime: 00/05/23 13:42:58 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet1.cpp                4-Jun-92            D.Lang
//
//  WSI Image Format Compression method #1.                          
//
//   COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#include "compmet1.h"
#include "wsitypedefs.h"

//-----------------------------------------------------------------------------

Compression_method1::~Compression_method1()
{
}

//-----------------------------------------------------------------------------

Compression_method * Compression_method1::clone() const
{
    return (Compression_method *)new Compression_method1(*this);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method1::uncompressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size) 
{
//-----------------------------------------------------------------------------
    //
    //  Determine length of uncompressed line.
    //
    //  WSI compression scheme #1 (8 bit).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //   (Hex)
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     nl             If n= 0..E  then output run of hl, length n+1
    //
//-----------------------------------------------------------------------------

    Word      runlength;
    Word      linelength = 0;
    Cmp_size  in_idx = 0;

    while (in_idx < in_size)
    {        
        runlength = in_ptr[in_idx++];
        runlength >>= 4;

        if (runlength != 0x0f)
        {
            linelength += runlength + 1;
        }
    }

    return (linelength);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method1::uncompress(
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
    //  WSI compression scheme #1 (8 bit).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //   (Hex)
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     nl             If n= 0..E  then output run of hl, length n+1
    //
//-----------------------------------------------------------------------------

    Word      runlength;
    Cmp_size  in_idx       = 0;
    Cmp_size  out_idx      = 0;
    Byte      high_nibble  = 0;
    Byte      pixel;

    if (offset != 0)
    {
        //
        // TODO: do something useful.
        //
        return BAD_CMP_SIZE;  // RETURN: Bad compression size.
    }
    while (in_idx < in_size)
    {        
        runlength = pixel = in_ptr[in_idx++];
        runlength >>= 4;

        if (runlength != 0x0f)
        {
            runlength++;
            pixel = (pixel & 0xf) | high_nibble;

            while (runlength-- != 0 && 
              out_idx < max_out_size) 
            {
                out_ptr[out_idx++] = pixel;
            }                
        }
        else
        {
            high_nibble = pixel << 4;
        }
    }

    if (cmp_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *cmp_bytes_used_ptr = in_idx;
    }

    return (out_idx);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method1::compressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size,
        Compress_data_description *) 
{
//-----------------------------------------------------------------------------
    //
    // Determine length of compressing data using scheme #1.
    //
    // Compression scheme #1 (8bit data).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     nl             Output run of byte (hl), length n+1
    //
//-----------------------------------------------------------------------------

    Cmp_size  out_idx  = 0;
    Word      count    = 0;
    Byte      high     = 0xff;    // force an initial high nibble mismatch.
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

        if ((pixel >> 4) != high)
        {
            high = pixel >> 4;
            out_idx++;
        }

        if (count >= 14)
        {
            out_idx += (count / 15);
            count %= 15;
        }    

        out_idx++;
    }

    return (out_idx);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method1::compress(
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
    // Compress data using 8 bit compression scheme #1.
    //
    // Compression scheme #1 (8bit data).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     nl             Output run of byte (hl), length n+1
    //
//-----------------------------------------------------------------------------

    Cmp_size        out_idx    = 0;
    const Byte *    in_org_ptr = in_ptr;
    const Byte *    end_ptr    = in_ptr + in_size;
    Word            count      = 0;
    Byte            high       = 0xff;    // force an initial high nibble mismatch.
    Byte            pixel;

    if (offset != 0)
    {
        //
        // TODO: do something useful.
        //
        return BAD_CMP_SIZE;  // RETURN:  Bad compression size.
    }
    while (in_ptr != end_ptr)
    {
        pixel = *in_ptr++;

        while (in_ptr != end_ptr && *in_ptr == pixel && count < 14)
        {
            count++;
            in_ptr++;
        }

        if ((pixel >> 4) != high)
        {
            if (out_idx >= max_out_size) break;     // !! BREAK !!
            high = pixel >> 4;
            out_ptr[out_idx++] = high | 0xf0;
        }

        if (out_idx >= max_out_size) break; // !! BREAK !!
        pixel = (count << 4) | (pixel & 0x0f);

        out_ptr[out_idx++] = pixel;
        count = 0;
    }

    if (raw_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *raw_bytes_used_ptr = (in_ptr - in_org_ptr) - count;
    }

    return (out_idx);
}

