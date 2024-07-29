// $Revision: /main/11 $ $Modtime: 00/05/23 13:43:56 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet4.cpp                4-Jun-92            D.Lang
//
//  WSI Image Format Compression method #4.                          
//
//   COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#include <string.h>

#include "compmet4.h"
#include "wsitypedefs.h"

//-----------------------------------------------------------------------------

Compression_method4::~Compression_method4()
{
}

//-----------------------------------------------------------------------------

Compression_method * Compression_method4::clone() const
{
    return (Compression_method *)new Compression_method4(*this);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method4::uncompressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size) 
{
//-----------------------------------------------------------------------------
    //
    //  Determine length of uncompressed line.
    //
    //  WSI compression scheme #4 (8 bit).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //   (Hex)
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     El, nn          Output run of hl, length nn+1
    //     Dl, ll, hh       Output run of hl, length (hhll)+1
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
                if (in_idx >= in_size) return BAD_CMP_SIZE;  // RETURN
                runlength = in_ptr[in_idx++];
            }
            else if (runlength == 0x0d)
            {
                if (in_idx >= in_size) return BAD_CMP_SIZE;  // RETURN
                runlength = in_ptr[in_idx++];
                if (in_idx >= in_size) return BAD_CMP_SIZE;  // RETURN
                runlength += in_ptr[in_idx++] * 256;
            }

            linelength += runlength + 1;
        }
    }

    return (linelength);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method4::uncompress(
        const Byte * in_ptr, 
        Cmp_size in_size,
        Byte * out_ptr, 
        Cmp_size max_out_size,
        Cmp_size offset, 
        Cmp_size * cmp_bytes_used_ptr)
{   
//-----------------------------------------------------------------------------
    //
    //  Uncompressed line.
    //
    //  WSI compression scheme #4 (8 bit).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //   (Hex)
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     El, nn          Output run of hl, length nn+1
    //     Dl, ll, hh       Output run of hl, length (hhll)+1
    //     nl             If n= 0..C  then output run of hl, length n+1
    //
//-----------------------------------------------------------------------------

    Word   runlength;
    Byte   high_nibble  = 0;
    Byte   pixel;
    const Byte * start_in_ptr  = in_ptr;
    const Byte * end_in_ptr    = in_ptr + in_size;
    Byte * start_out_ptr = out_ptr;
    Byte * end_out_ptr   = out_ptr + max_out_size;

    if (offset != 0)
    {
        //
        //  TODO:  Do something useful. 
        //
        return BAD_CMP_SIZE;  // RETURN: Bad compression size.
    }
    while (in_ptr < end_in_ptr)
    {        
        runlength = pixel = *in_ptr++;
        runlength >>= 4;

        if (runlength != 0x0f)
        {
            if (runlength == 0x0e)
            {
                if (in_ptr >= end_in_ptr) 
                {
                    return BAD_CMP_SIZE;  // RETURN: Bad compression size.
                }

                runlength = *in_ptr++;
            }
            else if (runlength == 0x0d)
            {
                if (in_ptr >= end_in_ptr) 
                {
                    return BAD_CMP_SIZE;  // RETURN: Bad compression size.
                }

                runlength = *in_ptr++;

                if (in_ptr >= end_in_ptr) 
                {
                    return BAD_CMP_SIZE;  // RETURN: Bad compression size.
                }

                runlength += *in_ptr++ * 256;
            }

            runlength++;
            pixel = (pixel & 0x0f) | high_nibble;

            //
            //  Check if it will fit.  If so, set the value in the output
            //  buffer.
            //
            if (out_ptr + runlength <= end_out_ptr)
            {
                ::memset(out_ptr, pixel, runlength);
            }

            out_ptr += runlength;
        }
        else
        {
            high_nibble = pixel << 4;
        }
    }

    if (cmp_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *cmp_bytes_used_ptr = in_ptr - start_in_ptr;
    }

    return out_ptr - start_out_ptr;
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method4::compressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size,
        Compress_data_description *)  
{
//-----------------------------------------------------------------------------
    //
    // Determine length of compressing data using scheme #4.
    //
    // Compression scheme #4 (8bit data).
    //
    //  Format:   (hl = high/low nibble of data byte)
    //
    // -Packed Data-      --------Output Data-------------------
    //     Fh             Set high nibble to h
    //     El, nn          Output run of hl,  length nn+1
    //     Dl, ll, hh       Output run of hl,  length hhll+1
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

        if (count >= 256)       // Handle word run-length encoding
        {
            out_idx += 3;
        }
        else if (count >= 13)   // Handle byte run-length encoding
        {
            out_idx += 2;
        }
        else                    // Handle Cmp_size runlength encoding.
        {
            out_idx++;
        }
    }

    return (out_idx);
}

//-----------------------------------------------------------------------------
//
// Compress data using 8 bit compression scheme #4.
//
// Compression scheme #4 (8bit data).
//
//  Format:   (hl = high/low nibble of data byte)
//
// -Packed Data-      --------Output Data-------------------
//     Fh             Set high nibble to h
//     El, nn          Output run of hl,  length nn+1
//     Dl, ll, hh       Output run of hl,  length hhll+1
//     nl             Output run of byte (hl), length n+1
//
//-----------------------------------------------------------------------------

Cmp_size Compression_method4::compress(
        const Byte* in_ptr,
        Cmp_size in_size, 
        Byte* out_ptr,
        Cmp_size max_out_size, 
        Cmp_size offset,
        Cmp_size * raw_bytes_used_ptr,
        Compress_data_description *) 
{
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
        return BAD_CMP_SIZE;  // RETURN:  Bad compression size.
    }
    while (in_ptr != end_ptr)
    {
        pixel = *in_ptr++;

        while (in_ptr != end_ptr && *in_ptr == pixel && count < 0xfffe)
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

        if (count >= 256)     // Handle word run-length encoding
        {
            if (out_idx >= max_out_size-2) break; // !! BREAK !!
            pixel = 0xd0 | (pixel & 0x0f);       // Flag as byte runlength
            out_ptr[out_idx++] = pixel;
            out_ptr[out_idx++] = count & 0xff;  // Output low byte
            out_ptr[out_idx++] = count >> 8;    // Output high byte
        }
        else if (count >= 13)     // Handle byte run-length encoding
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

//-----------------------------------------------------------------------------
//
//  Remove NOWrad annotation encoding and fire callback.
//
//  WSI compression scheme #4 (8 bit).
//
//  Format:   (hl = high/low nibble of data byte)
//   (Hex)
// -Packed Data-      --------Output Data-------------------
//     Fh             Set high nibble to h
//     El, nn         Output run of hl, length nn+1
//     Dl, ll, hh     Output run of hl, length (hhll)+1
//     nl             If n= 0..C  then output run of hl, length n+1
//
//  For each byte that has a runlength of 1, fire callback.
//
//-----------------------------------------------------------------------------

Cmp_size Compression_method4::extract_compressed_nowrad(
        const Byte* in_ptr,
        Cmp_size in_size, 
        Extraction_cb extraction_cb, 
        void * cb_data)
{
    Byte*       in_out_ptr = (Byte *)in_ptr;

    Word      runlength;
    Word      linelength = 0;
    Cmp_size  in_idx = 0;

    while (in_idx < in_size)
    {        
        runlength = in_out_ptr[in_idx];
        runlength >>= 4;

        if (runlength != 0x0f)
        {
            if (runlength == 0x0e)
            {                
                in_idx++;
                if (in_idx >= in_size) return BAD_CMP_SIZE;  // RETURN
                runlength = in_out_ptr[in_idx];
            }
            else if (runlength == 0x0d)
            {
                in_idx++;
                if (in_idx >= in_size) return BAD_CMP_SIZE;  // RETURN
                runlength = in_out_ptr[in_idx++];
                if (in_idx >= in_size) return BAD_CMP_SIZE;  // RETURN
                runlength += in_out_ptr[in_idx] * 256;
            }
            else if (runlength == 0x00)
            {
                in_out_ptr[in_idx] = 
                    extraction_cb(cb_data, in_out_ptr[in_idx], linelength);
            }

            linelength += runlength + 1;
        }

        in_idx++;
    }

    return (linelength);
}

//-----------------------------------------------------------------------------
//
//  Remove NOWradPLUS annotation encoding and fire callback.
//
//  Determine length of uncompressed line.
//
//  WSI compression scheme #4 (8 bit).
//
//  Format:   (hl = high/low nibble of data byte)
//   (Hex)
// -Packed Data-      --------Output Data-------------------
//     Fh             Set high nibble to h
//     El, nn          Output run of hl, length nn+1
//     Dl, ll, hh       Output run of hl, length (hhll)+1
//     nl             If n= 0..C  then output run of hl, length n+1
//
//  For each byte that is a high nibble conversion.
//
//-----------------------------------------------------------------------------

Cmp_size Compression_method4::extract_compressed_nowradplus(
        const Byte* in_ptr,
        Cmp_size in_size, 
        Extraction_cb extraction_cb, 
        void * cb_data)
{
    Byte*       in_out_ptr = (Byte *)in_ptr;

    Word      runlength;
    Word      linelength = 0;
    Cmp_size  in_idx = 0;

    while (in_idx < in_size)
    {        
        runlength = in_out_ptr[in_idx];

        if (runlength > 0xf0)
        {
            in_out_ptr[in_idx] = extraction_cb(cb_data, runlength, linelength);
        }

        in_idx++;
        runlength >>= 4;

        if (runlength != 0x0f)
        {
            if (runlength == 0x0e)
            {                
                if (in_idx >= in_size) return BAD_CMP_SIZE;  // RETURN
                runlength = in_out_ptr[in_idx++];
            }
            else if (runlength == 0x0d)
            {
                if (in_idx >= in_size) return BAD_CMP_SIZE;  // RETURN
                runlength = in_out_ptr[in_idx++];
                if (in_idx >= in_size) return BAD_CMP_SIZE;  // RETURN
                runlength += in_out_ptr[in_idx++] * 256;
            }

            linelength += runlength + 1;
        }
    }

    return (linelength);
}

