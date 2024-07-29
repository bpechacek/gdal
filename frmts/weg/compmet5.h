// $Revision: /main/7 $ $Modtime: 00/06/16 12:04:44 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmet5.h                8-Oct-92            D.Lang
//
//  WSI Image Format Compression_method #5 (WSI Mayer compression)
//
//    COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#ifndef COMPMET5_H
#define COMPMET5_H

#include "compmeth.h"
#include "wsitypedefs.h"  /* WSI types in private interface */

//
//  Forward declarations
//
class Comp_info;  
class Ucomp_info;

class Compression_method5 : public Compression_method
{
  public:

    ~Compression_method5();

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
    // Uncompress support methods
    //
    Short getbits(const Byte *str, Short n, Ucomp_info &);
    Short parsee(const Byte *str, Ucomp_info &);
    Short parsez(const Byte *str, Ucomp_info &);

    //
    // Compress support methods.
    //
    void putbit(Short n, Short x, Comp_info &);
    void putbitn(Short n, Short x, Comp_info &);
    void codee(Short delta, Comp_info &);
    void codez(Short delta, Comp_info &);
};

#endif
