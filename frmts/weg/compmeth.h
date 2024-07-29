// $Revision: /main/6 $ $Modtime: 00/06/16 12:08:04 $ $Author: mam6 $
//-----------------------------------------------------------------------------
//
//  compmeth.h                4-Jun-92            D.Lang
//
//  Abstract class to support Compression method Interface.                   
//
//    COPYRIGHT 2000 by WSI Corporation
//
//-----------------------------------------------------------------------------

#ifndef COMPMETH_H
#define COMPMETH_H

#include "compddesc.h"
#include "cpl_port.h"

/*
 *  Define types used in Compression_method interface.
 */
typedef GUInt32 Cmp_size;
typedef GByte Byte;

#define NULL_PTR 0L 
#define BAD_CMP_SIZE ((GUInt32)0L)

//-----------------------------------------------------------------------------
//
//  CLASS
//
//    Compression_methods
// 
//  REUSABILITY (Foundation, Framework, Weather Workstation)
//
//    Foundation
//
//  DESCRIPTION
//
//    Abstract class to support compression methods.
//
//  USAGE
//
//    This is an abstract class and is only of use to define the interface.
//
//-----------------------------------------------------------------------------

class Compression_method 
{
  public:
    Compression_method();

    Compression_method(const Compression_method&);

    Compression_method& operator=(const Compression_method&);

    virtual ~Compression_method();

    virtual Compression_method *clone(void) const = 0;

    virtual Cmp_size uncompressed_length(
            const Byte* in_ptr,
            Cmp_size in_size) = 0;

    virtual Cmp_size uncompress(
            const Byte* in_ptr,
            Cmp_size in_size, 
            Byte* out_ptr, 
            Cmp_size max_out_size,
            Cmp_size offset=0,
            Cmp_size * cmp_bytes_used_ptr = 0) = 0;

    virtual Cmp_size compressed_length(
            const Byte* in_ptr,
            Cmp_size in_size,
            Compress_data_description *data_description = 
                (Compress_data_description *)NULL_PTR) = 0;

    virtual Cmp_size compress(
            const Byte* in_ptr,
            Cmp_size in_size, 
            Byte* out_ptr,
            Cmp_size max_out_size,
            Cmp_size offset=0,
            Cmp_size * data_bytes_used_ptr = 0,
            Compress_data_description *data_description = 
                (Compress_data_description *)NULL_PTR) = 0;

  protected:      

    Compression_method(const Byte* data_ptr, Cmp_size data_size);
};

#endif  /* COMPMETH_H */
