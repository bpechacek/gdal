// $Revision: /main/6 $ $Modtime: 00/06/16 12:02:05 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet0.h                8-Oct-92            D.Lang
//  Compression_method #0.                          
//    COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#ifndef COMPMET0_H
#define COMPMET0_H

#include "compmeth.h"

//-----------------------------------------------------------------------------
//
//  CLASS
//
//    Compression_method0
// 
//  REUSABILITY (Foundation, Framework, Weather Workstation)
//
//    Foundation
//
//  DESCRIPTION
//
//    Compression method #0 class.
//
//-----------------------------------------------------------------------------

class Compression_method0 : public Compression_method
{
  public:

    ~Compression_method0();

    //
    //  Standard Virtual Methods
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
