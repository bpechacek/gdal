// $Revision: /main/6 $ $Modtime: 00/05/23 13:44:19 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet5.cpp                8-Oct-92            D.Lang
//
//  WSI Image Format Compression method #5 (WSI Mayer compression)
//
//   COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#include "compmet5.h"
#include "wsitypedefs.h"

//-----------------------------------------------------------------------------

const  Short ZEROS  = -4;
static Short MASK[]  = {0, 1, 3, 7, 15, 31, 63, 127, 255, 511};
static Short EETAB[] = {0, 2, 4, 8, 16, 32, 64, 128};

const Short          MAXCBITS = 32;      // Number of bits in a "cbits"
//-----------------------------------------------------------------------------
class Ucomp_info  // Uncompress Bit field support
{
  public:

    Short          have_bits;
    UShort bits;
    Cmp_size       in_size;
    Cmp_size       in_idx;
};

//-----------------------------------------------------------------------------
class Comp_info  // Compression Bit field support
{
  public:

    ULong         bits;     
    Short         empty_bits;
    Byte*         out_ptr;
    Cmp_size      out_idx;
    Cmp_size      max_out_size;
};

//-----------------------------------------------------------------------------

Compression_method5::~Compression_method5()
{
}

//-----------------------------------------------------------------------------

Compression_method * Compression_method5::clone() const
{
    return (Compression_method *)new Compression_method5(*this);
}

//-----------------------------------------------------------------------------
// Get a bit field from the byte array.
//-----------------------------------------------------------------------------

Short Compression_method5::getbits(
        const Byte *in_ptr,
        Short width,
        Ucomp_info  &uinfo)
{
    if (width <= uinfo.have_bits)
    {
        uinfo.have_bits -= width;
        return ((uinfo.bits >> uinfo.have_bits) & MASK[width]);  // RETURN
    }    

    Short byte;

    while (width > uinfo.have_bits)
    {
        if (uinfo.in_idx < uinfo.in_size)
        {
            byte = in_ptr[uinfo.in_idx++];
        }  
        else
        {
            return (0);  // RETURN: No more data.
        }  

        uinfo.bits <<= 8;
        uinfo.bits |= byte;
        uinfo.have_bits += 8;
    }  

    return ((uinfo.bits >> (uinfo.have_bits -= width)) & MASK[width]);
}

//-----------------------------------------------------------------------------

Short Compression_method5::parsee(const Byte *in_ptr, Ucomp_info &uinfo)
{
    Short nn, sign, value;

    nn = 0;
    while (nn < 7 && getbits(in_ptr, 1, uinfo))
    { 
        nn++;
    }

    if (nn == 0)
    {
        if (getbits(in_ptr, 1, uinfo) == 0)
        {
            return (0);  // RETURN
        }    
        else
        {
            return (getbits(in_ptr, 1, uinfo) == 0 ? 1 : -1);  // RETURN
        }            
    } 

    sign  = getbits(in_ptr, 1, uinfo);
    value = EETAB[nn] + getbits(in_ptr, nn, uinfo);

    return (sign == 0 ? value : -value);
}

//-----------------------------------------------------------------------------

Short Compression_method5::parsez(const Byte *in_ptr, Ucomp_info &uinfo)
{
    switch (getbits(in_ptr, 2, uinfo))
    {
      case 0:
        return (1);  // RETURN

     case 1:
       if (getbits(in_ptr, 1, uinfo) == 0)
           return (2 + getbits(in_ptr, 1, uinfo));  // RETURN
       else
           return (4 + getbits(in_ptr, 2, uinfo));  // RETURN

     case 2:
       if (getbits(in_ptr, 1, uinfo) == 0)
           return (8  + getbits(in_ptr, 3, uinfo));  // RETURN
       else
           return (16 + getbits(in_ptr, 4, uinfo));  // RETURN

     case 3:
       switch (getbits(in_ptr, 2, uinfo))
       {
         case 0:
           return (32  + getbits(in_ptr, 5, uinfo));  // RETURN
         case 1:
           return (64  + getbits(in_ptr, 6, uinfo));  // RETURN
         case 2:
           return (128 + getbits(in_ptr, 7, uinfo));  // RETURN
         case 3:
           if (getbits(in_ptr, 1, uinfo) == 0)
               return (256 + getbits(in_ptr, 8, uinfo));  // RETURN
           else
               return (512 + getbits(in_ptr, 9, uinfo));  // RETURN
        } 
    }  

    return BAD_CMP_SIZE;
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method5::uncompressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size) 
{
//-----------------------------------------------------------------------------
    //
    //  Determine length of uncompressed line.
    //
    //  WSI compression scheme #5 (8 bit).
    //
//-----------------------------------------------------------------------------

    Cmp_size   out_idx = 0;

    Ucomp_info uinfo;

    uinfo.have_bits = 0;
    uinfo.in_idx    = 0;
    uinfo.in_size   = in_size;

    while (uinfo.in_idx < in_size || uinfo.have_bits != 0)
    {
        if (parsee(in_ptr, uinfo) == ZEROS)
        {
            out_idx += parsez(in_ptr, uinfo);
        }  
        else
        {
            out_idx++;
        }  
    }  

    return out_idx;
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method5::uncompress(
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
    //  WSI compression scheme #5 (8 bit).
    //
//-----------------------------------------------------------------------------

    Short    count;
    Cmp_size out_idx  = 0;
    Byte     prev_out = 0;

    Ucomp_info uinfo;

    uinfo.have_bits = 0;
    uinfo.bits      = 0; 
    uinfo.in_size   = in_size;
    uinfo.in_idx    = 0; 

    if (offset != 0)
    {
        // 
        // TODO: do something useful. 
        //
        return BAD_CMP_SIZE;  // RETURN:  Bad compression size.
    }
    while ((uinfo.in_idx < uinfo.in_size || 
            uinfo.have_bits != 0) && 
            out_idx < max_out_size)
    {
        count = parsee(in_ptr, uinfo);
        if (count == ZEROS)
        {
            count = parsez(in_ptr, uinfo);
            while (count-- > 0 && out_idx < max_out_size)
            {
                out_ptr[out_idx++] = 0;
            }    

            prev_out = 0;
        }  
        else
        {
            if (count < ZEROS)
                count++;

            prev_out += count;
            out_ptr[out_idx] = prev_out;
            out_idx++;      
        }  
    }  

    if (cmp_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *cmp_bytes_used_ptr = uinfo.in_idx; 
    }

    return out_idx;
}

//-----------------------------------------------------------------------------
//
//  putbitn - Writes a specified number of bits.  It only adjusts queue and bit
//  count to indicate number of bits left.
//
//-----------------------------------------------------------------------------

void Compression_method5::putbitn(Short n, Short x, Comp_info &cinfo)
{
    //
    // n - number of bits; x - value of bits 
    //
    // While there are whole bytes to be written, 
    // write them to standard output. 
    //
    while (cinfo.empty_bits <= MAXCBITS - 8)
    {
        if (cinfo.out_idx < cinfo.max_out_size)
        {
            cinfo.out_ptr[cinfo.out_idx] = 
              (Byte)(cinfo.bits >> (MAXCBITS - 8 - cinfo.empty_bits));
        }            

        cinfo.out_idx++;
        cinfo.empty_bits += 8;
    }

    //
    //  Adjust contents of queue, leaving left over bits. 
    //
    cinfo.bits = (cinfo.bits << n) | x;
    cinfo.empty_bits -= n;
}   

//-----------------------------------------------------------------------------
//
// Macro which writes "n" bits to standard output.  "x" is the value to be  
// written.  The macro waits until it has complete bytes before it actually 
// does the writing. 
//
//-----------------------------------------------------------------------------

inline void Compression_method5::putbit(Short n, Short x, Comp_info &cinfo) 
{
    if (n < cinfo.empty_bits) 
        cinfo.bits = (cinfo.bits << (n)) | (x), cinfo.empty_bits -= (n);
    else        
        putbitn(n, x, cinfo);
}    

//-----------------------------------------------------------------------------
//
//  codee - This function encodes a single pixel as a difference.  It also
//  encodes special flag (ZEROS: indicates a run length of zero's follows).
//
//  The encoding of the delta is done according to the following table.
//
//      delta                coding
//      -----                ------
//          0                00
//      +/- 1                01s
//      +/- 2 - 3            10s.
//      +/- 4 - 7            110s..
//      +/- 8 - 15           1110s...
//      +/- 16 - 31          11110s....
//      +/- 32 - 63          111110s.....
//      +/- 64 - 127         1111110s......
//      +/- 128 - 255        1111111s.......
//
//  "s" is the sign bit.  The "."'s represent the difference between
//  the base value of the delta and the actual value.  For example,
//  a delta of 40 would be encoded as: 111110001000
//                                     ------+-----
//                                       |   |  |
//                                 flag -+ sign +- 8 = 40 - 32
//
//-----------------------------------------------------------------------------

void Compression_method5::codee(Short delta, Comp_info &cinfo)
{
    Short sign;             // keeps track of sign of delta 
    //
    //  Check to see if delta is 0.  Encode it. 
    //
    if (delta == 0)
    {
        putbit(2, 0, cinfo);
        return;  // RETURN.
    }

    //
    // Check to see if the delta is greater than or less than 0.  
    // Set flag to indicate sign (1 - negative; 0 - positive).  
    // Make delta absolute. 
    //
    if (delta < 0)
    {
        delta = -delta;
        sign = 1;
    }
    else
    {
        sign = 0;
    }

    //
    //  Encode delta. 
    //
    if (delta == 1)
    {
        putbit(3, 2 + sign, cinfo);
    }
    else if (delta < 4)
    {
        putbit(3, 4 + sign, cinfo);
        putbit(1, delta - 2, cinfo);
    }
    else if (delta < 8)
    {
        putbit(4, 12 + sign, cinfo);
        putbit(2, delta - 4, cinfo);
    }
    else if (delta < 16)
    {
        putbit(5, 28 + sign, cinfo);
        putbit(3, delta - 8, cinfo);
    }
    else if (delta < 32)
    {
        putbit(6, 60 + sign, cinfo);
        putbit(4, delta - 16, cinfo);
    }
    else if (delta < 64)
    {
        putbit(7, 124 + sign, cinfo);
        putbit(5, delta - 32, cinfo);
    }
    else if (delta < 128)
    {
        putbit(8, 252 + sign, cinfo);
        putbit(6, delta - 64, cinfo);
    }
    else
    {
        putbit(8, 254 + sign, cinfo);
        putbit(7, delta - 128, cinfo);
    }
}    

//-----------------------------------------------------------------------------
//
//  codez - This function encodes the run length of a run of zero's. 
//  The encoding is done according the the followin table. 
//
//      run length      coding
//      ----------      ------
//      1 - 1           00
//      2 - 3           01.
//      4 - 7           011..
//      8 - 15          100...
//      16 - 31         101....
//      32 - 63         1100.....
//      64 - 127        1101......
//      128 - 255       1110.......
//      256 - 511       11110........
//      312 - 1023      11111.........
//
//  The "."'s following the coding flag is the difference between the 
//  base run length and the actual value.  For example a run of 24 would
//  be endcoded as: 1011000
//                  ---====
//                   |   |
//             flag -+   +- 8 = 24 - 16
//
//-----------------------------------------------------------------------------

void Compression_method5::codez(Short delta, Comp_info &cinfo)
{
    //
    //  Encode the run length. 
    //
    if (delta == 1)
      putbit(2, 0, cinfo);                        // run length = 1 
    else if (delta < 4)
      putbit(4, (2 * 2 - 2) + delta, cinfo);       // run length = 2 to 3 
    else if (delta < 8)
      putbit(5, (3 * 4 - 4) + delta, cinfo);       // run length = 4 to 7 
    else if (delta < 16)
      putbit(6, (4 * 8 - 8) + delta, cinfo);       // run length = 8 to 15 
    else if (delta < 32)
      putbit(7, (5 * 16 - 16) + delta, cinfo);     // run length = 16 to 31 
    else if (delta < 64)             
      putbit(9, (12 * 32 - 32) + delta, cinfo);    // run length = 32 to 63 
    else if (delta < 128)
      putbit(10, (13 * 64 - 64) + delta, cinfo);   // run length = 64 to 127 
    else if (delta < 256)
      putbit(11, (14 * 128 - 128) + delta, cinfo); // run length = 127 to 255 
    else if (delta < 512)
      putbit(13, (30 * 256 - 256) + delta, cinfo); // run length = 256 to 511 
    else
      putbit(14, (31 * 512 - 512) + delta, cinfo); // run length = 512 to 1023 
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method5::compressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size,
        Compress_data_description *)  
{
    //
    // Determine length of compressing data using scheme #5.
    //
    Short n          = 0;
    Short c          = 0;
    Short lastc      = 0;    
    Short outc       = 0;    
    Long  in_idx     = 0;
    Long  signed_in_size = (Long)in_size;  /* Avoid compiler warning below*/

    Comp_info cinfo;

    cinfo.bits         = 0; 
    cinfo.empty_bits   = MAXCBITS;
    cinfo.out_ptr      = (Byte *)NULL_PTR;
    cinfo.out_idx      = 0;
    cinfo.max_out_size = 0;

    while (in_idx < signed_in_size)
    {
        c = in_ptr[in_idx++];

        //
        // Find the difference between the current pixel and the last one. 
        //
        outc  = c - lastc;
        lastc = c;

        //
        //  Check if color is zero, and count it. 
        //
        if (c == 0)
        {
            n++;
        }
        else
        {
            //
            // Check to see if there was a run of zero's and encode it.
            //
            if (n > 0)
            {
                codee(ZEROS, cinfo);
                codez(n, cinfo);
                n = 0;
            }

            //
            // Encode the new pixel as a difference between it and 
            // the previous one. 
            // Check for ZEROS and adjust if it matches.
            //
            codee(outc <= ZEROS ? outc - 1 : outc, cinfo);
        }

        if (in_idx >= signed_in_size)
        {
            //
            // Encode the last run of zeros. 
            //
            if (n > 0)
            {
                codee(ZEROS, cinfo);
                codez(n, cinfo);
            }

            break;
        }
    }

    //
    // Encode the end of line marker 
    // Flush the queue 
    //
    putbit(7, 0, cinfo);
    putbitn(0, 0, cinfo);

    return cinfo.out_idx;
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method5::compress(
        const Byte* in_ptr,
        Cmp_size in_size, 
        Byte* out_ptr,
        Cmp_size max_out_size, 
        Cmp_size offset,
        Cmp_size * raw_bytes_used_ptr,
        Compress_data_description *)  
{
    //
    // Compress data using 8 bit compression scheme #5.
    //
    Short n          = 0;
    Short c          = 0;
    Short lastc      = 0;
    Short outc       = 0;
    Long  in_idx     = 0;
    Long  signed_in_size = (Long)in_size;  /* Avoid compiler warning below*/

    Comp_info cinfo;

    cinfo.bits         = 0; 
    cinfo.empty_bits   = MAXCBITS;
    cinfo.out_ptr      = out_ptr;
    cinfo.out_idx      = 0;
    cinfo.max_out_size = max_out_size;

    if (offset != 0)
    {
        //
        // TODO: do something useful. 
        //
        return BAD_CMP_SIZE;  // RETURN:  Bad compression size.
    }
    while (in_idx < signed_in_size)
    {
        c = in_ptr[in_idx++];

        //
        // Find the difference between the current pixel and the last one. 
        //
        outc  = c - lastc;
        lastc = c;

        //
        // Check if color is zero, and count it. 
        //
        if (c == 0)
        {
            n++;
        }
        else
        {
            //
            // Check to see if there was a run of zero's and encode it.
            //
            if (n > 0)
            {
                codee(ZEROS, cinfo);
                codez(n, cinfo);
                n = 0;
            }

            //
            // Encode the new pixel as a difference between it and 
            // the previous one. 
            // Check for ZEROS and adjust if it matches.
            //
            codee(outc <= ZEROS ? outc - 1 : outc, cinfo);
        }

        if (in_idx >= signed_in_size)
        {
            //
            // Encode the last run of zeros. 
            //
            if (n > 0)
            {
                codee(ZEROS, cinfo);
                codez(n, cinfo);
            }

            break;
        }
    }

    //
    // Encode the end of line marker 
    // Flush the queue 
    //     
    putbit(7, 0, cinfo);
    putbitn(0, 0, cinfo);

    if (raw_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *raw_bytes_used_ptr = in_idx; 
    }

    return cinfo.out_idx;
}

