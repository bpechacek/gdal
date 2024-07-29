// $Revision: /main/8 $ $Modtime: 00/06/16 12:04:08 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet4.h                8-Oct-92            D.Lang
//
//  WSI Image Format Compression_method #4.                          
//
//    COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#ifndef COMPMET4_H
#define COMPMET4_H

#include "compmeth.h"
#include "wsitypedefs.h"  /* WSI types in private interface */

class Compression_method4 : public Compression_method
{
  public:

    ~Compression_method4();

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

    typedef Byte (* Extraction_cb)(void* cb_data, Byte pixel, Long column);

    //
    //  The following "extract_..." methods are specialized for
    //  images with imbedded annotation.
    //
    Cmp_size extract_compressed_nowrad(
            const Byte* in_ptr,
            Cmp_size in_size, 
            Extraction_cb,
            void*);

    Cmp_size extract_compressed_nowradplus(
            const Byte* in_ptr,
            Cmp_size in_size, 
            Extraction_cb,
            void*);
};

#endif
