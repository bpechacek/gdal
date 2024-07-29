// $Revision: /main/6 $ $Modtime: 00/06/16 12:03:05 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet2.h                8-Oct-92            D.Lang
//
//  Compression_method #2.                          
//
//    COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#ifndef COMPMET2_H
#define COMPMET2_H

#include "compmeth.h"

//-----------------------------------------------------------------------------
//
//  CLASS
//
//    Compression_method2
// 
//  REUSABILITY (Foundation, Framework, Weather Workstation)
//
//    Foundation
//
//  DESCRIPTION
//
//    Compression method #2 class.
//
//-----------------------------------------------------------------------------

class Compression_method2 : public Compression_method
{
  public:

    ~Compression_method2();

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
};

#endif
