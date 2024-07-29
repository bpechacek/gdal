// $Revision: /main/5 $ $Modtime: 00/06/16 14:36:12 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  ucmpmeth.h                8-Oct-92            D.Lang
//
//  WSI Image Format Uncompressed_method.  
//
//   COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#ifndef UCMPMETH_H
#define UCMPMETH_H

#include "compmeth.h"

class Uncompressed_method : public Compression_method
{
  public:

    ~Uncompressed_method();

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
            Cmp_size * cmp_bytes_used_ptr = 0);

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
            Cmp_size * data_bytes_used_ptr = 0,
            Compress_data_description *data_description = 
                (Compress_data_description *)NULL_PTR);
};

#endif  /* UCMPMETH_H */
