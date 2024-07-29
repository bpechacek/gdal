// $Revision: /main/5 $ $Modtime: 00/05/23 13:43:39 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet3.cpp                4-Jun-92            D.Lang
//
//  WSI Image Format Compression method #3.                          
//
//   COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#include "compmet3.h"
#include "wsitypedefs.h"

//-----------------------------------------------------------------------------

Short extract_bits(const Byte* bptr, Short bit_idx, Short field_width)
{
    // ********************************
    // This will not work!!!!!!!!!
    // ********************************
    //    
    //   field width       0  1  2  3  4  5  6  7 8
    //
    static Short mask[] = {0, 1, 3, 7, 15, 31, 63, 127, 255};

    Short bits = (*(const Short*)bptr) >> bit_idx;    

    if (field_width < 0 || field_width > 8)
    {
        return BAD_CMP_SIZE;  // RETURN: Bad compression size
    }
    else
    {
        return bits & mask[field_width];  // RETURN
    }        
}

//-----------------------------------------------------------------------------

Compression_method3::~Compression_method3()
{
}

//-----------------------------------------------------------------------------

Compression_method * Compression_method3::clone() const
{
    return (Compression_method *)new Compression_method3(*this);
}

//-----------------------------------------------------------------------------

Short Compression_method3::get_bits(Short bit_count)
{
    //
    // This routine peels off the specified number of bits.
    //
    Short field;

    if (my_idx + (my_bit_idx + bit_count) / 8 > my_in_size)
    {
        my_done = TRUE;
        return (0);  // RETURN
    }  

    field = extract_bits(my_in_ptr+my_idx, my_bit_idx, bit_count);

    my_bit_idx += bit_count;
    while (my_bit_idx >= 8)
    {
        my_bit_idx -= 8;
        my_idx++;
    }  

    return field;
}

//-----------------------------------------------------------------------------

WWBoolean Compression_method3::parse_difference(Short &difference)
{
    //
    // This routine parses out a difference code word.
    //
    Short sign_bit, one_bits;

    //
    // First count the 1's in the difference code word. 
    //
    one_bits = 0;
    while (get_bits(1) == 1)
        one_bits++;

    //
    // Deal with data exhaustion. 
    //
    if (my_done) return (FALSE);  // RETURN:  Data exhausted.
    //
    // If there are no 1's, deal with it. 
    //
    if (one_bits == 0)
    {
        if (get_bits(1) == 0)
            difference = 0;
        else
        {
            if (get_bits(1) == 0)
                difference = 1;
            else
                difference = -1;
        }  // else 
    }  // if one_bits 
    else
    {
        sign_bit = get_bits(1);
        if (one_bits < 1 || one_bits > 7)
        {
            return FALSE;  // RETURN
        }

        difference =  1 << one_bits;    // 2, 4, 8, 16, 32, 64, 128
        difference += get_bits(one_bits);

        if (sign_bit == 1)
            difference = -difference;
    }  // else 

    return (TRUE);
}

//-----------------------------------------------------------------------------

Short Compression_method3::parse_run_length(void)
{
    //
    // This routine parses out a run length code word.
    //
    Short dcd_word;

    switch (get_bits(2))
    {
        case 0:
            dcd_word = 1;
            break;

        case 1:
            if (get_bits(1) == 0)
                dcd_word = get_bits(1) + 2;
            else
                dcd_word = get_bits(2) + 4;
            break;

        case 2:
            if (get_bits(1) == 0)
                dcd_word = get_bits(3) + 8;
            else
                dcd_word = get_bits(4) + 16;
            break;

        case 3:
            switch (get_bits(2))
            {
                case 0:
                    dcd_word = get_bits(5) + 32;
                    break;

                case 1:
                    dcd_word = get_bits(6) + 64;
                    break;

                case 2:
                    dcd_word = get_bits(7) + 128;
                    break;

                case 3:
                    if (get_bits(1) == 0)
                        dcd_word = get_bits(8) + 256;
                    else
                        dcd_word = get_bits(9) + 512;
                    break;
            } 

            break;
    }  

    return (dcd_word);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method3::uncompressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size) 
{
    //
    // This routine decodes differentially encoded 8 bit data.
    //
    Short    difference;
    Short    level     = 0;
    Cmp_size out_count = 0;

    my_in_ptr       = in_ptr;
    my_in_size      = in_size;
    my_idx          = 0;
    my_bit_idx      = 0;
    my_done         = FALSE;

    //
    // Go fetch difference code word. 
    //
    while (parse_difference(difference))
    {
        //
        // Check for special case where difference indicates a run length of
        // zeroes is next.
        if (difference == -4)
        {
            //
            // Go fetch run length code word. 
            //
            out_count += parse_run_length();
            continue;
        }  

        //
        // Adjust difference to allow for run length item special case. 
        //
        if (difference < -4)
            difference++;

        level += difference;

        //
        // Separate the handling of imagery data from background data.        
        //
        if (level == 0)
        {
            out_count++;
        } 
        else if (level > 0)
        {
            out_count++;
        }  
        else
        {
            out_count += parse_run_length();
        }           
    }  

    return (out_count);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method3::uncompress(
        const Byte* in_ptr,
        Cmp_size in_size,
        Byte* out_ptr,
        Cmp_size max_out_size, 
        Cmp_size offset,
        Cmp_size * cmp_bytes_used_ptr)
{
    //
    // This routine decodes differentially encoded 8 bit data.
    //
    Short    difference, run_length_count;
    Short    background_level;
    Short    level     = 0;
    Cmp_size out_count = 0;

    if (offset != 0)
    {
        //
        // TODO: do something useful.
        //
        return BAD_CMP_SIZE;  // RETURN:  Bad compression size.
    }

    my_in_ptr       = in_ptr;
    my_in_size      = in_size;
    my_out_ptr      = out_ptr;
    my_max_out_size = max_out_size;
    my_idx          = 0;
    my_bit_idx      = 0;
    my_done         = FALSE;

    //
    // Go fetch difference code word. 
    //
    while (parse_difference(difference))
    {
        //
        // Check for special case where difference indicates a run length of 
        // zeroes is next.
        // 
        if (difference == -4)
        {
            //
            // Go fetch run length code word. 
            //
            run_length_count = parse_run_length();

            level = 0;
            while (run_length_count > 0)
            {
                if (out_count < my_max_out_size)
                {
                    my_out_ptr[out_count] = level;
                }    

                out_count++;
                run_length_count--;
            }  // while run_length_count 

            continue;
        }  // if difference 

        //
        // Adjust difference to allow for run length item special case. 
        //
        if (difference < -4)
            difference++;

        level += difference;

        //
        // Separate the handling of imagery data from background data. 
        //
        if (level == 0)
        {
            if (out_count < my_max_out_size)
                my_out_ptr[out_count] = 0;
            out_count++;
        } 
        else if (level > 0)
        {
            if (out_count < my_max_out_size)
                my_out_ptr[out_count] = level + 19;
            out_count++;
        }  
        else
        {
            if (level < -10)
            {
                background_level = 0;
            }
            else
            {
                static Byte back_levels[10] = {7, 6, 5, 8, 9, 4, 3, 0, 2, 1};
                background_level = back_levels[10+level];
            }

            //
            // Go fetch run length code word. 
            //
            run_length_count = parse_run_length();

            while (run_length_count > 0)
            {
                if (out_count < my_max_out_size)
                    my_out_ptr[out_count] = background_level;
                out_count++;

                run_length_count--;
            }  
        }           
    }  

    if (cmp_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *cmp_bytes_used_ptr = my_idx;        // ???
    }

    return (out_count);
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method3::compressed_length(
        const Byte* in_ptr, 
        Cmp_size in_size,
        Compress_data_description *) 
{
    my_in_ptr       = in_ptr;
    my_in_size      = in_size;    

    return BAD_CMP_SIZE;        
}

//-----------------------------------------------------------------------------

Cmp_size Compression_method3::compress(
        const Byte* in_ptr,
        Cmp_size in_size, 
        Byte* out_ptr,
        Cmp_size max_out_size, 
        Cmp_size offset,
        Cmp_size * raw_bytes_used_ptr,
        Compress_data_description *) 
{
    //
    // This routine encodes differentially encoded 8 bit data.
    //
    my_in_ptr       = in_ptr;
    my_in_size      = in_size;    
    my_out_ptr      = out_ptr;
    my_max_out_size = max_out_size;

    if (offset != 0)
    {
        //
        // TODO: do something useful. 
        //
        return BAD_CMP_SIZE;  // RETURN:  Bad compression size.
    }

    if (raw_bytes_used_ptr != (Cmp_size *)NULL_PTR)
    {
        *raw_bytes_used_ptr = 0;
    }

    return BAD_CMP_SIZE;
}

