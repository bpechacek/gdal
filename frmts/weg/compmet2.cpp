// $Revision: /main/7 $ $Modtime: 00/05/23 13:43:17 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet2.cpp                4-Jun-92            D.Lang
//
//  WSI Image Format Compression method #2.                          
//
//   COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#include "compmet2.h"
#include "wsitypedefs.h"

//-----------------------------------------------------------------------------

Compression_method2::~Compression_method2()
{
}

//-----------------------------------------------------------------------------

Compression_method * Compression_method2::clone() const
{
    return (Compression_method *)new Compression_method2(*this);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method2::uncompressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size) 
{
//-----------------------------------------------------------------------------
    //
    //  Determine length of uncompressed line.
    //
    //  WSI compression scheme #2 (8 bit).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //   (Hex)
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     El, nn          Output run of hl, length nn+1
    //     nl             If n= 0..C  then output run of hl, length n+1
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
            if (runlength == 0x0e)
            {                
                if (in_idx >= in_size) 
                { 
                    //
                    // We have got problems.
                    //
                    return BAD_CMP_SIZE;  // RETURN:  Bad compression size.
                }

                runlength = in_ptr[in_idx++];
            }

            linelength += runlength + 1;
        }
    }

    return (linelength);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method2::uncompress(
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
    //  WSI compression scheme #2 (8 bit).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //   (Hex)
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     El, nn          Output run of hl, length nn+1
    //     nl             If n= 0..C  then output run of hl, length n+1
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
            if (runlength == 0x0e)
            {
                if (in_idx >= in_size)
                { 
                    //
                    // We have got problems. 
                    //
                    return BAD_CMP_SIZE;  // RETURN: Bad compression size
                }

                runlength = in_ptr[in_idx++];
            }

            runlength++;
            pixel = (pixel & 0x0f) | high_nibble;

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

Cmp_size Compression_method2::compressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size,
        Compress_data_description *)  
{
//-----------------------------------------------------------------------------
    //
    // Determine length of compressing data using scheme #2.
    //
    // Compression scheme #2 (8bit data).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     El, nn          Output run of hl,  length nn+1
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

        if (count >= 256)
        {
            out_idx += (count / 256 * 2);
            count %= 256;
        }    

        if (count >= 14)
        {
            out_idx += 2;
        }
        else
        {
            out_idx++;
        }
    }

    return (out_idx);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method2::compress(
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
    // Compress data using 8 bit compression scheme #2.
    //
    // Compression scheme #2 (8bit data).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     El, nn          Output run of hl,  length nn+1
    //     nl             Output run of byte (hl), length n+1
    //
//-----------------------------------------------------------------------------

    Cmp_size      out_idx    = 0;
    const Byte *  in_org_ptr = in_ptr;
    const Byte *  end_ptr    = in_ptr + in_size;
    Word          count      = 0;
    Byte          high       = 0xff;    // force an initial high nibble mismatch.
    Byte          pixel;

    if (offset != 0)
    {
        //
        // TODO: do something useful.
        //
        return BAD_CMP_SIZE;  // RETURN: Bad compression size.
    }
    while (in_ptr != end_ptr)
    {
        pixel = *in_ptr++;

        while (in_ptr != end_ptr && *in_ptr == pixel && count < 255)
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

        if (count >= 14)     // Handle byte run-length encoding
        {
            if (out_idx >= max_out_size-1) break; // !! BREAK !!
            pixel = 0xe0 | (pixel & 0x0f);       // Flag as byte runlength
            out_ptr[out_idx++] = pixel;
            out_ptr[out_idx++] = (Byte)count;   // Output byte runlength
        }
        else  // Handle Cmp_size runlength encoding.
        {
            if (out_idx >= max_out_size) break; // !! BREAK !!
            pixel = (count << 4) | (pixel & 0x0f);

            out_ptr[out_idx++] = pixel;
        }

        count = 0;
    }

    if (raw_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *raw_bytes_used_ptr = (in_ptr - in_org_ptr) - count;
    }

    return (out_idx);
}

