// $Revision: /main/6 $ $Modtime: 00/06/16 12:03:40 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet3.h                4-Jun-92            D.Lang
//
//  Compression_method #3.                          
//
//    COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#ifndef COMPMET3_H
#define COMPMET3_H

#include "compmeth.h"
#include "wsitypedefs.h"   /* WSI types in private interface */

//-----------------------------------------------------------------------------
//
//  CLASS
//
//    Compression_method3
// 
//  REUSABILITY (Foundation, Framework, Weather Workstation)
//
//    Foundation
//
//  DESCRIPTION
//
//    Compression method #3 class.
//
//-----------------------------------------------------------------------------

class Compression_method3 : public Compression_method
{
  public:

    ~Compression_method3();

    //
    // Standard Virtual Methods
    //
    Compression_method *clone(void) const;

    Cmp_size uncompressed_length(const Byte* in_ptr, Cmp_size in_size);

    Cmp_size uncompress(
            const Byte* in_ptr,
            Cmp_size in_size, 
            Byte* out_ptr,
            Cmp_size max_out_size,
            Cmp_size offset=0,
            Cmp_size * cmp_bytes_used_ptr = (Cmp_size *)NULL_PTR);

    Cmp_size compressed_length(
            const Byte* in_ptr,
            Cmp_size in_size,
            Compress_data_description *data_description = 
                (Compress_data_description *)NULL_PTR);

    Cmp_size compress(
            const Byte* in_ptr,
            Cmp_size in_size, 
            Byte* out_ptr,
            Cmp_size max_out_size,
            Cmp_size offset=0,
            Cmp_size * data_bytes_used_ptr = (Cmp_size *)NULL_PTR,
            Compress_data_description *data_description = 
                (Compress_data_description *)NULL_PTR);

  private:

    //
    // Internal methods and data members
    //
    Short       get_bits(Short width);
    WWBoolean   parse_difference(Short &difference);
    Short       parse_run_length(void);

    const Byte* my_in_ptr;
    Cmp_size    my_in_size;
    Byte*       my_out_ptr;
    Cmp_size    my_max_out_size;
    Cmp_size    my_idx;
    Short       my_bit_idx;
    WWBoolean   my_done;
};

#endif
