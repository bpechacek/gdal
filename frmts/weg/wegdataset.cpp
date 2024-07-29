/******************************************************************************
 * $Id: wsiifdataset.cpp,v 1.0 2007/03/15 15:54:48 smarshall Exp $
 *
 * Project:  WSI Express Graphics (WEG) format reader
 * Purpose:  Codec for image format specific to Weather Services International 
 * Author:   Steve Marshall, smarshall@wsi.com
 *
 ******************************************************************************
 * Copyright (c) 2007,  Steve Marshall <smarshall@wsi.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 * 
 */

#include "gdal_pam.h"
#include "ogr_spatialref.h"   /* For OGRSpatialReference */

/* Include WSI Express Graphics compression methods */
#include "ucmpmeth.h"
#include "compmet0.h"
#include "compmet1.h"
#include "compmet2.h"
#include "compmet3.h"
#include "compmet4.h"
#include "compmet5.h"

CPL_CVSID("$Id: $");

CPL_C_START
void	GDALRegister_WEG(void);
CPL_C_END

/*
 *  Define some macros for use in debugging.
 */
/*
#define DEBUG_WEG 1
#define DEBUG_IREADBLOCK 1
*/
#ifdef DEBUG_WEG
#define DEBUG0(s)                fprintf(stderr, s)
#define DEBUG1(s,x1)             fprintf(stderr, s, x1)
#define DEBUG2(s,x1,x2)          fprintf(stderr, s, x1, x2)
#define DEBUG3(s,x1,x2,x3)       fprintf(stderr, s, x1, x2, x3)
#define DEBUG4(s,x1,x2,x3,x4)    fprintf(stderr, s, x1, x2, x3, x4)
#define DEBUG5(s,x1,x2,x3,x4,x5) fprintf(stderr, s, x1, x2, x3, x4, x5)
#else
#define DEBUG0(s)             
#define DEBUG1(s,x1)             
#define DEBUG2(s,x1,x2)          
#define DEBUG3(s,x1,x2,x3)       
#define DEBUG4(s,x1,x2,x3,x4)    
#define DEBUG5(s,x1,x2,x3,x4,x5) 
#endif

/* Macros to strip most significant bit from a bit value */ 
#define REMOVE_MSB(x) ((x) & (~128))
#define MSB_IS_ON(x)  (((x) & (128)) ? 1 : 0) 

#define BUFFSIZE 1048576
//#define BUFFSIZE 4194304
//#define BUFFSIZE 33554432
//#define BUFFSIZE 262144
//#define BUFFSIZE 67108864
/*
 *  Define the WSI flag and control byte values
 */
const char *        WSI_FLAG           = "\x00\xf0";
const unsigned char WSI_FLAG_F0        = 0xf0;
const unsigned char WSI_FLAG_00        = 0x00;
const unsigned char WSI_CTL_NONE       = 0x00;
const unsigned char WSI_CTL_RAW_LINE   = 0x01;
const unsigned char WSI_CTL_END        = 0x02;
const unsigned char WSI_CTL_LABEL80    = 0x03;
const unsigned char WSI_CTL_CMP1_LINE  = 0x04;
const unsigned char WSI_CTL_CMP2_LINE  = 0x05;
const unsigned char WSI_CTL_TEXT_MSG   = 0x06;
const unsigned char WSI_CTL_CMP3_LINE  = 0x07;
const unsigned char WSI_CTL_RGB_COLORS = 0x08;
const unsigned char WSI_CTL_HEADER     = 0x09;
const unsigned char WSI_CTL_SIZE       = 0x0a;
const unsigned char WSI_CTL_PROJECTION = 0x0b;
const unsigned char WSI_CTL_CMP4_LINE  = 0x0c;
const unsigned char WSI_CTL_CMP5_LINE  = 0x0d;         //  Ultrasat ?
const unsigned char WSI_CTL_VALID_TIME = 0x12;
const unsigned char WSI_CTL_TILE_SIZE  = 0x21;
const unsigned char WSI_CTL_RAW_TILE   = 0x22;
const unsigned char WSI_CTL_CMP4_TILE  = 0x23;
const unsigned char WSI_CTL_CMP5_TILE  = 0x24;
const unsigned char WSI_CTL_TASC_TILE  = 0x25;

/*  These are not currently used */
const unsigned char WSI_CTL_DATA_LEVELS= 0x11; // (new) color table data levels 
const unsigned char WSI_CTL_TRANSPARENT= 0x13; // (new) transparency table.
const unsigned char WSI_CTL_LEGEND     = 0x14; // (new) legend
const unsigned char WSI_CTL_TILE_DEF   = 0x15; // (new) tile sector definition
const unsigned char WSI_CTL_QUALITY    = 0x16; // (new) image quality indicators
const unsigned char WSI_CTL_VERSION    = 0x17; // (new) version # section
const unsigned char WSI_CTL_ID         = 0x18; // (new) identification section
const unsigned char WSI_CTL_SOURCE     = 0x19; // (new) data source description
const unsigned char WSI_CTL_TILE_DATA  = 0x1B; // (new) tiled data
const unsigned char WSI_CTL_NAV_DATA   = 0x1D; // (new) navigational data
const unsigned char WSI_CTL_OBJECTS    = 0x30; // (new) geometric objects.

//
//  Define WEG image start codes.
//
const unsigned char WSI_MED4BIT   = 0x11;  //  240 lines, 384 pixels, 4bit
const unsigned char WSI_HIGH4BIT  = 0x12;  //  480 lines, 768 pixels, 4bit
const unsigned char WSI_MED8BIT   = 0x14;  //  240 lines, 384 pixels, 8bit
const unsigned char WSI_HIGH8BIT  = 0x15;  //  480 lines, 768 pixels, 8bit
const unsigned char WSI_WIDE4BIT  = 0x16;  //  240 lines, 768 pixels, 4bit
const unsigned char WSI_WIDE8BIT  = 0x17;  //  240 lines, 768 pixels, 8bit
const unsigned char WSI_TILED8BIT = 0x18;  //  Variable resolution, 8bit

/*
 *  Define geophysical constants.
 *  For angle conversions, we use the GDAL value given by SRS_UA_DEGREE_CONV.
 */
static const double DEG_PER_RAD = 1.0 / 0.0174532925199433;
static const double WEG_EARTH_RADIUS_METERS = 6367451.5;  

/*
 *  Define static compression method objects.  This avoids the need to keep
 *  recreating them.  These objects carry no state information, so we can
 *  reuse them on several images without worry about side-effects. 
 */
static Uncompressed_method UNCOMPRESSED_METHOD;
static Compression_method0 COMPRESSION_METHOD0;
static Compression_method1 COMPRESSION_METHOD1;
static Compression_method2 COMPRESSION_METHOD2;
static Compression_method3 COMPRESSION_METHOD3;
static Compression_method4 COMPRESSION_METHOD4;
static Compression_method5 COMPRESSION_METHOD5;

#ifdef _WIN32
inline double round(double number)
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}
#endif

/************************************************************************/
/*                            WEGScaleDistance()                        */
/************************************************************************/

static void WEGScaleDistance(double & distance, double wsiNavScale)
{
    /*
     *  Convert dimensionless 'projection units' into meters.
     *  Round value to no more than mm precision.
     */
    distance *= wsiNavScale;
    distance = 1e-3 * round(distance * 1e3);
}

/************************************************************************/
/*                            WEGConvertAngle()                         */
/************************************************************************/

static void WEGConvertAngle(double & angle)
{
    /*  Convert radians to degrees.*/
    angle *= DEG_PER_RAD;

    /* Round to avoid excessive precision in map projection parameters */
    angle = 1e-4 * round(1e4 * angle); 
}

/************************************************************************/
/*                            WEGGetStartCode()                         */
/************************************************************************/

static short WEGGetStartCode(unsigned char start_byte)
{
    switch (start_byte)
    {
      case WSI_MED4BIT:  case WSI_HIGH4BIT: 
      case WSI_MED8BIT:  case WSI_HIGH8BIT:
      case WSI_WIDE4BIT: case WSI_WIDE8BIT: 
      case WSI_TILED8BIT:
        return (short) start_byte;
      default:
        return -1;
    }
}

/************************************************************************/
/*                            WEGGetControlByte()                       */
/************************************************************************/

static int WEGGetControlByte(unsigned char cb)
{
    /* 
     *  Check that the following byte has a supported value.  Here we mask
     *  off the most significant bit, which signifies whether or not 
     *  checksums are present at the end of a section.
     */
    int masked_cb = REMOVE_MSB(cb); 

    if ((masked_cb >= '\x01' && masked_cb <= '\x0d') || 
        masked_cb == '\x10' || masked_cb == '\x11' || masked_cb == '\x1d' || 
        (masked_cb >= '\x21' && masked_cb <= '\x25') || masked_cb == '\x30')
    {
        return masked_cb;  
    }
    else
    {
        return -1; 
    }
}

/************************************************************************/
/*                            WEGSetDefaultGeogCS()                     */
/************************************************************************/
static OGRErr WEGSetDefaultGeogCS(OGRSpatialReference * poSRS)
{
    return poSRS->SetGeogCS("WSI Geographic CS",
            "WSI Custom Datum",
            "WSI Sphere",
            WEG_EARTH_RADIUS_METERS, 0.0);
}

/************************************************************************/
/*                            WEGGetProjParams()                        */
/************************************************************************/
static const char * WEGGetProjParams(
        const char * pGNPos,
        const char * projName,
        int          nParams,
        double *     adfParams)
{
    int i;
    const char * pParamText = pGNPos;  /* Save for use in error msg*/
    char * pEnd = NULL;

    for (i = 0; i < nParams; ++i)
    {
        double dval = strtod(pGNPos, &pEnd);
        if (pEnd == pGNPos) /* No conversion made */    
        {
            CPLError(CE_Failure, CPLE_AppDefined, 
                "Could not extract %s projection parameters #%d from "
                "GeoNav text '%s'", projName, i + 1, pParamText); 
            return NULL;
        }
      
        pGNPos = pEnd;        /* Skip over text we parsed */
        adfParams[i] = dval;  /* Save val in array to return */
    }  /* end of loop over expected parameters */
   
    return pGNPos;  /* Return pointer past the text we parsed */
}

/************************************************************************/
/* ==================================================================== */
/*				WEGDataset                                              */
/* ==================================================================== */
/************************************************************************/

class WEGRasterBand;

class WEGDataset : public GDALPamDataset
{
    friend class WEGRasterBand;

    VSILFILE	*fpL;
    short       startCode;           /* Type of image */
    int         nBitDepth;           /* Number of bits per pixel */
    int         nTransHdrSegOff;     /* Offset to transmission header */
    int         nImageStartSegOff;   /* Offset to Image Start code */
    int         nImageSizeSegOff;    /* Offset to Image Size segment */
    int         nTileSizeSegOff;     /* Offset to Tile Size segment */
    int         nLabelSegOff;        /* Offset to Label Segment */
    int         nGeoNavSegOff;       /* Offset to GeoNav Segment */
    int         nColorTableSegOff;   /* Offset to Color Table segment */
    int         nStartDataOff;       /* Offset to first data segment */
    const char *pszGeoNavText;       /* Pointer into GeoNav text in image */
    const char *pszLabelText;        /* Pointer into Label text in image */
    const char *pszValidTimeText;    /* Pointer into valid time text in img*/
    const GByte *pszDataLevels;       /* Pointer into Data Levels segment*/
    int         nHeaderSize;         /* Number of bytes in header */
    GByte	   *pabyHeader;          /* Header of image held in memory */
    OGRSpatialReference oSRS;        /* Spatial Coordinate System for image */
    char       *pszProjection;       /* Well-Known Text for projection */
    int         bGeoTransformValid;  /* True if transform matrix populated */
    double      adfGeoTransform[6];  /* GeoTransform matrix */
    GDALColorTable *poColorTable;    /* Color table for this image */
    int         bHaveNoData;
    double      dfNoDataValue;

    /*
     *  Private methods.
     */
    WEGDataset();
    CPLErr DecodeColorTableSegment(int hasCheckSum);
    CPLErr DecodeGeoNavSegment();
    CPLErr DecodeDataLevels();
    const char * DecodeStereographicProjection(const char * pGNPos);
    const char * DecodeLambertConformalProjection(const char * pGNPos);
    const char * DecodeMercatorProjection(const char * pGNPos);
    const char * DecodePerfectSatelliteProjection(const char * pGNPos);
    const char * DecodeCylindricalEquidistantProjection(const char * pGNPos);

  public:
	~WEGDataset();
    
    static GDALDataset *Open( GDALOpenInfo * );
    static int Identify(GDALOpenInfo *);

    CPLErr 	GetGeoTransform( double * padfTransform );
    const char *GetProjectionRef();

private:
    static int IdentifyInternal(GDALOpenInfo *, short& startCode, int& trans_hdr_len);
};

/************************************************************************/
/* ==================================================================== */
/*                            WEGRasterBand                             */
/* ==================================================================== */
/************************************************************************/

class WEGRasterBand : public GDALPamRasterBand
{
    friend class WEGDataset;
    int nLastDataOff;
    int nLastLine;
    int nRecordSize;
    GByte * pszRecord;

    int DecodeLineNumber(GByte *& pPos, int nLineNumWidth);
    
  public:

    ~WEGRasterBand();
   	WEGRasterBand( WEGDataset *, int );
   
    /*  Framework methods overriden in this class */
    virtual GDALColorInterp GetColorInterpretation();
    virtual GDALColorTable *GetColorTable();
    virtual double GetNoDataValue( int *pbSuccess );
    virtual CPLErr IReadBlock( int, int, void * );

};

/************************************************************************/
/*                           ~WEGRasterBand()                           */
/************************************************************************/

WEGRasterBand::~WEGRasterBand()
{
    if (pszRecord != NULL)
    {
        CPLFree(pszRecord);
    }
}

/************************************************************************/
/*                           WEGRasterBand()                            */
/************************************************************************/

WEGRasterBand::WEGRasterBand( WEGDataset *poDS, int nBand )

{
    this->poDS = poDS;
    this->nBand = nBand;
    
    eDataType = GDT_Byte;

    /*  TODO - set block sizes differently for tiled images */
    /*  The block size should be the tile size */

    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = poDS->GetRasterYSize();

    /*  Set members specific to this class */
    nLastDataOff = poDS->nStartDataOff;
    nLastLine    = 0;

    /*
     *  Define buffer for reading data from the data store 
     */
    nRecordSize = BUFFSIZE;
    pszRecord = (GByte *) CPLMalloc(nRecordSize);
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

GDALColorInterp WEGRasterBand::GetColorInterpretation()
{
    WEGDataset  *poWDS = (WEGDataset *) poDS;

    if (poWDS->poColorTable == NULL)
    {
        return GCI_GrayIndex;
    }
    else
    {
        return GCI_PaletteIndex;
    }
}

/************************************************************************/
/*                           GetColorTable()                            */
/************************************************************************/

GDALColorTable *WEGRasterBand::GetColorTable()

{
    WEGDataset  *poWDS = (WEGDataset *) poDS;
    return poWDS->poColorTable;
}

/************************************************************************/
/*                           GetNoDataValue()                           */
/************************************************************************/

double WEGRasterBand::GetNoDataValue( int *pbSuccess )

{
    WEGDataset *poWDS = (WEGDataset *) poDS;

    if( poWDS->bHaveNoData )
    {
        if( pbSuccess != NULL )
            *pbSuccess = poWDS->bHaveNoData;
        return poWDS->dfNoDataValue;
    }
    else  /* Call up to parent class */
    {
        return GDALPamRasterBand::GetNoDataValue( pbSuccess );
    }
}


/************************************************************************/
/*  WEGRasterBand::DecodeLineNumber()                                   */
/*     private method used in IReadBlock.                               */
/************************************************************************/
int WEGRasterBand::DecodeLineNumber(GByte *& pPos, int nLineNumWidth)
{
    int nLine = -1; 
    if (nLineNumWidth == 2)
    {
        /*  Read 2-byte integer, LSB first */
        nLine = ((pPos[1] << 8) | *pPos);
    }
    else    /* Single byte used to encode line numbers. */
    {
        nLine = (unsigned int) pPos[0];

        DEBUG2("Comparing single byte line number %d to lastLine %d\n",
                nLine, nLastLine);
        if (nLine < nLastLine)  /* Adjust actual line number if necessary*/
        {
            int nBase = (nLastLine + 1) / 256;
            nLine += (nBase * 256); 
            DEBUG1("Adjusted line number to %d\n", nLine);
        }
    }

    /* Skip over line number */
    pPos += nLineNumWidth;

    /* Update the last line number to this line number and return */
    nLastLine = nLine;
    return nLine;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr WEGRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pRealImage )

{
#ifdef DEBUG_IREADBLOCK
    DEBUG2("WEGRasterBand::IReadBlock: getting BLOCK %d %d\n",
            nBlockXOff,  nBlockYOff);
#endif
    WEGDataset *poWDS = (WEGDataset *) poDS;
    int nPixelSize = 1;   /* TODO - determine from pixel type */

    if (poWDS->nStartDataOff < 0)
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "WEG Data segment not found");
        return CE_Failure;
    }

    /*
     *  Seek to the begining of the raster data in the file
     */
    if (VSIFSeekL(poWDS->fpL, poWDS->nStartDataOff, SEEK_SET) < 0)
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "WEG error trying to seek to offset %d",
                  poWDS->nStartDataOff);
        return CE_Failure;
    }
    
    /*
     *  Read the first buffer of data from the file
     */
    int nRead = VSIFReadL(pszRecord, nPixelSize, nRecordSize, poWDS->fpL);
    if (nRead <= 0)
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "WEG Read error trying to read %d bytes",
                  nRead, nRecordSize * nPixelSize);
        return CE_Failure;
    }

#ifdef DEBUG_IREADBLOCK
        DEBUG3("Read %d bytes from offset %d into record of size %d\n",
            nRead * nPixelSize, nLastDataOff, nRecordSize);  
#endif

    // Initialize the pointer to the current position in the image buffer
    GByte * pImage = (GByte*)pRealImage;
    GByte * pPos = pszRecord;
    GByte * pEnd = pszRecord + nRead;
    GByte * pEndLastLine = pszRecord;
    
    bool bFalseFlags = false;
    
    // Uncompress the raster data line by line and copy it into the image buffer
    for (int i = 1; i <= poDS->GetRasterYSize(); i++)
    {
        GByte * pSegStart = NULL; 
        GByte * pSegEnd   = NULL;
        Compression_method * poCmpMeth = NULL;
        int nLineNumWidth = 1;
   
        /*
         *  Find start of first segment in data we read 
         */
        while (pPos < pEnd - 3)
        {
            /* 
             *  Skip until we get to a start of segment flag.
             */
            if (memcmp(pPos, WSI_FLAG, 2) != 0)
            {
                pPos++;
                continue;
            }
            if (pPos[2] == WSI_CTL_NONE)  /* False flag within binary data */
            {
                pPos += 3;
                continue;
            }
    
            pSegStart = pPos;  /* Remember where segment started */
            /* 
             *  Skip a few extra bytes, assuming the minimum scanline must
             *  be at least a few bytes long, including the overhead.
             *  This avoids a problem with images where a false WEG flag 
             *  sequence was sometimes found at the beginning of a scanline.
             *  This includes images with a trans header "family" code starting
             *  with "PRE", i.e. precip accumulations, and STRIKE TWC images.
             */
            pPos += 6;
            break;
        }

        if (pSegStart == NULL)   /* Handle case where start not found */
        {
/*            CPLError(CE_Failure, CPLE_AppDefined,
                    "Error finding start of segment for line %d", i);*/
            // Copy the exra bytes to the front of the buffer
            int nBytesLeft = pEnd - pEndLastLine;
            /*CPLError( CE_Failure, CPLE_AppDefined, 
                          "Bytes left %d",
                          nBytesLeft);*/
            GByte * pRecordBeg = pszRecord;
            GByte * pEndLast = pEndLastLine;
            while (pEndLast < pEnd)
            {
                *pRecordBeg = *pEndLast;
                pRecordBeg++;
                pEndLast++;
            }
            // Read in and fill the rest of the buffer
            nRead = VSIFReadL(pszRecord + nBytesLeft, nPixelSize, nRecordSize - nBytesLeft, poWDS->fpL);
            if (nRead <= 0)
            {
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "WEG Read error trying to read %d bytes",
                          nRead, nRecordSize * nPixelSize);
                return CE_Failure;
            }
            #ifdef DEBUG_IREADBLOCK
                DEBUG2("Read %d bytes into record of size %d\n",
                nRead * nPixelSize, nRecordSize);  
            #endif
            pPos = pszRecord;
            pEnd = pszRecord + nRead + nBytesLeft;
            pEndLastLine = pszRecord;
            i--;
            continue;
        }

        /*
         *  Find start of next segment in data.  This is the marker for 
         *  the end of the segment we found in the previous code.
         */
        while (pPos < pEnd - 2)
        {
            /* 
             *  Skip until we get the flag for the next segment.
             */
            if (memcmp(pPos, WSI_FLAG, 2) != 0)
            {
                pPos++;
                continue;
            }
            if (pPos[2] == WSI_CTL_NONE)  /* False flag within binary data */
            {
                bFalseFlags = true;
                pPos += 3;
                continue;
            }

            /*  Set a pointer at the end of this segment. */
            pSegEnd = pPos;
            break;
        }

        /*
         *  We found a complete segment in the data we read.
         */
        if (pSegEnd == NULL)
        {
/*            CPLError(CE_Failure, CPLE_AppDefined,
                    "Error finding end of segment for line %d", i);*/
            // Copy the exra bytes to the front of the buffer
            int nBytesLeft = pEnd - pEndLastLine;
            /*CPLError( CE_Failure, CPLE_AppDefined, 
                          "Bytes left %d",
                          nBytesLeft);*/
            GByte * pRecordBeg = pszRecord;
            GByte * pEndLast = pEndLastLine;
            while (pEndLast < pEnd)
            {
                *pRecordBeg = *pEndLast;
                pRecordBeg++;
                pEndLast++;
            }
            // Read in and fill the rest of the buffer
            nRead = VSIFReadL(pszRecord + nBytesLeft, nPixelSize, nRecordSize - nBytesLeft, poWDS->fpL);
            if (nRead <= 0)
            {
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "WEG Read error trying to read %d bytes",
                          nRead, nRecordSize * nPixelSize);
                return CE_Failure;
            }
            #ifdef DEBUG_IREADBLOCK
                DEBUG2("Read %d bytes into record of size %d\n",
                nRead * nPixelSize, nRecordSize);  
            #endif
            pPos = pszRecord;
            pEnd = pszRecord + nRead + nBytesLeft;
            pEndLastLine = pszRecord;
            i--;
            continue;
        }
        
        /* Go through the scanline again and remove all false flags */
        if (bFalseFlags == true)
        {
            bFalseFlags = false;
            pPos = pSegStart + 6;
            while (pPos < pSegEnd)
            {
                /* 
                 *  Skip until we get the flag for the next segment.
                 */
                if (memcmp(pPos, WSI_FLAG, 2) != 0)
                {
                    pPos++;
                    continue;
                }
                if (pPos[2] == WSI_CTL_NONE)  /* False flag within binary data */
                {
                    pSegEnd--;
                    /* Delete the control character and shift all data forward */
                    for(GByte * iter = &pPos[2]; iter < pSegEnd; iter++)
                    {
                        *iter = *(iter+1);
                    }
                    pPos += 2;
                    continue;
                }
            }
        }        
        
        
        /*
         *  Restart parsing at the start of the segment, and 
         *  determine what kind of control byte we have.
         */         
        pPos = pSegStart + 2;
        GByte ctlByte = REMOVE_MSB(*pPos);
        int hasCheckSum = MSB_IS_ON(*pPos);
#ifdef DEBUG_IREADBLOCK
        DEBUG1("In IReadBlock: Found control byte %02X\n", ctlByte);
#endif
       
        poCmpMeth = NULL;
        nLineNumWidth = 1;
        switch (ctlByte)
        {
          case WSI_CTL_NONE:  /* False flag sequence in data -- skip it */
            pPos++;
            break;
          case WSI_CTL_END:  /* end of image */
            pPos++; 
            break;

          /*  Tile segment - we don't currently handle these */ 
          case WSI_CTL_RAW_TILE:
          case WSI_CTL_CMP4_TILE:
          case WSI_CTL_CMP5_TILE:
          case WSI_CTL_TASC_TILE:
            CPLError(CE_Failure, CPLE_AppDefined,
                    "Found tile data segment %02X in image: "
                    "tiled images are not currently supported", ctlByte);
            return CE_Failure;

          /* Scanline segment */
          case WSI_CTL_RAW_LINE:
            poCmpMeth = &UNCOMPRESSED_METHOD;
            nLineNumWidth = 1;
            break;
          case WSI_CTL_CMP1_LINE:
            /* May use method 0 or 1, depending on bits-per-pixel */
            if (poWDS->nBitDepth == 4)
                poCmpMeth = &COMPRESSION_METHOD0;
            else
                poCmpMeth = &COMPRESSION_METHOD1;
            nLineNumWidth = 1;
            break;
          case WSI_CTL_CMP2_LINE:
            poCmpMeth = &COMPRESSION_METHOD2;
            nLineNumWidth = 1;
            break;
          case WSI_CTL_CMP3_LINE:
            poCmpMeth = &COMPRESSION_METHOD3;
            nLineNumWidth = (poDS->GetRasterYSize() < 256) ? 1 : 2;
            break;
          case WSI_CTL_CMP4_LINE:
            poCmpMeth = &COMPRESSION_METHOD4;
            nLineNumWidth = (poDS->GetRasterYSize() < 256) ? 1 : 2;
            break;
          case WSI_CTL_CMP5_LINE:
            poCmpMeth = &COMPRESSION_METHOD5;
            nLineNumWidth = (poDS->GetRasterYSize() < 256) ? 1 : 2;
            break;

          default:
            CPLError(CE_Failure, CPLE_AppDefined,
                    "Found unsupported control byte %02X in image header",
                    ctlByte);
            return CE_Failure;
        }
    
        if (poCmpMeth != NULL)  /* We found a scanline or tile segment */
        {
            /*
             *   Skip over control bytes and line number before passing the
             *   data to the Compression_method object for decompression.
             */
            pPos++;                   /* Skip over control byte */ 
            if (hasCheckSum) pPos++;  /* Skip over optional checksum */
            
            /*  Decode and skip over line number */
#ifdef DEBUG_IREADBLOCK
            DEBUG1("In DecodeLineNumber at file offset %d\n",
                   (int)(pPos - pszRecord) + nLastDataOff);
#endif
            int nLine = DecodeLineNumber(pPos, nLineNumWidth);
#ifdef DEBUG_IREADBLOCK
            DEBUG2("Out DecodeLineNumber #%d at file offset %d\n", nLine,
                   (int)(pPos - pszRecord) + nLastDataOff);
#endif
            /*
             *  Check line number to see if this is the one we want. 
             *  Note: WEG line number start at 1; nBlockYOff starts at 0.
             */
            if (i == nLine) 
            {
                /* We found the line we want ! */
                /*  Figure out how long this line is */
                int nCompLen = (pSegEnd - pPos);
#ifdef DEBUG_IREADBLOCK
                DEBUG3("Found target line %d i %d of length %d\n", 
                        nLine, i, nCompLen);
#endif
                if (nCompLen <= 0)
                {
                    CPLError(CE_Failure, CPLE_AppDefined,
                        "Found zero length segment in image");
                    return CE_Failure;
                }

                int nActualWidth = poCmpMeth->uncompress(pPos, nCompLen, 
                        (GByte*)pImage, poDS->GetRasterXSize());

#ifdef DEBUG_IREADBLOCK
                DEBUG1("Decompressed line to actual width %d\n",
                        nActualWidth);
#endif
                
                /*  Deal with extra "margin" data that may be found in line*/
                int nMargin = (poDS->GetRasterXSize() - nActualWidth) / 2;
                if (nMargin > 0)
                {
                    //
                    //  Center Short image.  Perform safe memcpy
                    //  with overlapping regions.
                    //
                    GByte * pDst = (GByte*)pImage + nMargin + nActualWidth;
                    GByte * pSrc = (GByte*)pImage + nActualWidth;

                    while (pSrc != pImage)
                    {
                        *--pDst = *--pSrc;
                    }
                    memset(pImage, 0, (size_t)nMargin);
                }  /* end of code to deal with margin */

                /*  Set position for next line */
                pPos = pEndLastLine = pSegEnd;
                
            }  /* end of processing the line that we want */
            else
            {
                CPLError(CE_Failure, CPLE_AppDefined,
                    "Error finding traget line; nline: %d, i: %d", nLine, i);
            }
        }  /* end of logic to check and possibly decompress a line */
        pImage += poDS->GetRasterXSize();
    }  /* end of loop over reading from dataset line by line*/

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*          				WEGDataset	                    			*/
/* ==================================================================== */
/************************************************************************/

WEGDataset::WEGDataset()
 : fpL(NULL)
 , startCode(0)
 , nBitDepth(8)
 , nTransHdrSegOff(-1)
 , nImageStartSegOff(-1) 
 , nImageSizeSegOff(-1) 
 , nTileSizeSegOff(-1) 
 , nLabelSegOff(-1)
 , nGeoNavSegOff(-1)
 , nColorTableSegOff(-1)
 , nStartDataOff(-1) 
 , pszGeoNavText(NULL)
 , pszLabelText(NULL)
 , pszValidTimeText(NULL)
 , pszDataLevels(NULL)
 , nHeaderSize(0)
 , pabyHeader(NULL)
 , oSRS()
 , pszProjection(NULL)
 , bGeoTransformValid(FALSE)   
 , adfGeoTransform()
 , poColorTable(NULL)
 , bHaveNoData(FALSE)
 , dfNoDataValue(-1)
{
    for (int i = 0; i < 6; ++i)
    {
        adfGeoTransform[i] = 0.0;
    }
}

/************************************************************************/
/*                            ~WEGDataset()                             */
/************************************************************************/

WEGDataset::~WEGDataset()

{
    FlushCache(true);
    if (fpL != NULL)
        VSIFCloseL(fpL);
    if (pabyHeader != NULL)
        CPLFree(pabyHeader);
    if (pszProjection != NULL)
        CPLFree(pszProjection);
    if (poColorTable != NULL)
        delete poColorTable;
}

/************************************************************************
 *  Private methods in WEGDataset for decoding projection parameters.
 *  These methods skip past the text they process and return a pointer
 *  to the text that follows.  On error, the generate an error message
 *  and return NULL.
 ************************************************************************/
const char * 
WEGDataset::DecodeStereographicProjection(const char * pGNPos)
{
    /*
     *  Parse and skip past the projection-specific parameters.
     *  WEG parameters for Stereographic projection:
     *    0 = Origin latitude (radians)
     *    1 = Origin longitude (radians)
     */
    static const char * projName = SRS_PT_STEREOGRAPHIC; 
    const int nExpParams = 2;
    double projParams[nExpParams];
    const char * pEnd = NULL;

    pEnd = WEGGetProjParams(pGNPos, projName, nExpParams, projParams);
    if (pEnd == NULL)
    {
        return NULL;  /* Parsing failed; Error was already issued */
    }

    /*
     *  Convert parameters into the form needed for OGR Stereographic
     *  projection.
     */
    WEGConvertAngle(projParams[0]); /* Convert lat radians to degrees */
    WEGConvertAngle(projParams[1]); /* Convert lon radians to degrees */

    /*
     *  Set the spatial reference system of the dataset to use the
     *  appropriate Stereographic projection.
     *  The false northing and easting are always 0.0 in WEG,
     *  and the scale factor is always 1.0.
     */
    oSRS.SetProjCS(projName);
    WEGSetDefaultGeogCS(&oSRS);
    oSRS.SetStereographic(projParams[0], projParams[1], 1.0, 0.0, 0.0);

    return pEnd; 
}

const char * 
WEGDataset::DecodeLambertConformalProjection(const char * pGNPos)
{
    /*
     *  Parse and skip past the projection-specific parameters.
     *  WEG parameters for Lambert Conformal Conic projection:
     *    0 = Northern-most standard parallel (radians)
     *    1 = Southern-most standard parallel (radians)
     *    2 = Origin latitude (radians)
     *    3 = Origin longitude, i.e. central meridian (radians)
     */
    static const char * projName = SRS_PT_LAMBERT_CONFORMAL_CONIC_2SP; 
    const int nExpParams = 4;
    double projParams[nExpParams];
    const char * pEnd = NULL;

    pEnd = WEGGetProjParams(pGNPos, projName, nExpParams, projParams);
    if (pEnd == NULL)
    {
        return NULL;  /* Parsing failed; Error was already issued */
    }

    /*
     *  Convert parameters into the form needed for OGR LCC projection
     */
    WEGConvertAngle(projParams[0]); /* Convert stdlat1 radians to degrees */
    WEGConvertAngle(projParams[1]); /* Convert stdlat2 radians to degrees */
    WEGConvertAngle(projParams[2]); /* Convert lat0 radians to degrees */
    WEGConvertAngle(projParams[3]); /* Convert lon0 radians to degrees */

    /*
     *  Set the spatial reference system of the dataset to use the
     *  appropriate Lambert Conformal Conic (LCC) projection.
     *  The false northing and easting are always 0.0 in WEG.
     */
    oSRS.SetProjCS(projName);
    WEGSetDefaultGeogCS(&oSRS);
    oSRS.SetLCC(projParams[0], projParams[1], projParams[2], 
            projParams[3], 0.0, 0.0);  
    return pEnd; 
}

const char * 
WEGDataset::DecodeMercatorProjection(const char * pGNPos)
{
    /*
     *  Parse and skip past the projection-specific parameters.
     *  WEG parameters for Mercator projection:
     *    0 = Origin Longitude (radians)
     */
    static const char * projName = SRS_PT_MERCATOR_1SP;
    const int nExpParams = 1;
    double projParams[nExpParams];
    const char * pEnd = NULL;

    pEnd = WEGGetProjParams(pGNPos, projName, nExpParams, projParams);
    if (pEnd == NULL)
    {
        return NULL;  /* Parsing failed; Error was already issued */
    }

    /*
     *  Convert parameters into the form needed for OGRSpatialReference.
     */
    WEGConvertAngle(projParams[0]); /* Convert lon radians to degrees */

    /*
     *  Set the spatial reference system of the dataset to use the
     *  appropriate Mercator projection.
     *  The center latitude, false northing and easting are always 0.0 
     *  in WEG, and the scale factor is always 1.0.
     */
    oSRS.SetProjCS(projName);
    WEGSetDefaultGeogCS(&oSRS);
    oSRS.SetMercator(0.0, projParams[0], 1.0, 0.0, 0.0); 
    return pEnd; 
}

const char * 
WEGDataset::DecodePerfectSatelliteProjection(const char * pGNPos)
{
    /*
     *  Parse and skip past the projection-specific parameters.
     *  WEG parameters for Perfect Satellite projection:
     *    0 = Latitude of satellite subpoint (radians)
     *    1 = Longitude of satellite subpoint (radians)
     *    2 = Distance between satellite and center of earth (km).
     */
    static const char * projName = SRS_PT_GEOSTATIONARY_SATELLITE;
    const int nExpParams = 3;
    double projParams[nExpParams];
    const char * pEnd = NULL;

    pEnd = WEGGetProjParams(pGNPos, projName, nExpParams, projParams);
    if (pEnd == NULL)
    {
        return NULL;  /* Parsing failed; Error was already issued */
    }

    /*
     *  Convert parameters into the form needed for OGR GEOS projection
     */
    WEGConvertAngle(projParams[0]); /* Convert lat radians to degrees */
    WEGConvertAngle(projParams[1]); /* Convert lon radians to degrees */
  
    /*
     *  Convert elevation from distance from CENTER of earth to distance
     *  from SURFACE of earth.  Also convert units from km to meters.
     */
    projParams[2] *= 1000.;
    projParams[2] -= WEG_EARTH_RADIUS_METERS;

    /*
     *  Check for cases that cannot be supported by OGR subsystem.
     */
    if (! CPLIsEqual(projParams[0], 0.0))
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
            "OGR GEOS projection does not allow satellite subpoint "
            "at non-zero latitude %g", projParams[0]);
        return NULL;
    }

    /*
     *  Set the spatial reference system of the dataset to use the
     *  appropriate Geostationary Satellite (GEOS) projection.
     *  The false northing and easting are always 0.0 in WEG.
     */
    oSRS.SetProjCS(projName);
    WEGSetDefaultGeogCS(&oSRS);
    oSRS.SetGEOS(projParams[1], projParams[2], 0.0, 0.0);  

    return pEnd; 
}

const char * 
WEGDataset::DecodeCylindricalEquidistantProjection(const char * pGNPos)
{
    /*
     *  Parse and skip past the projection-specific parameters.
     *  WEG parameters for Cylindrical Equidistant projection:
     *    0 = Origin Longitude (radians)
     *  Note: In GDAL, this projection is called EQUIRECTANGULAR.
     */
    static const char * projName = SRS_PT_EQUIRECTANGULAR; 
    const int nExpParams = 1;
    double projParams[nExpParams];
    const char * pEnd = NULL;

    pEnd = WEGGetProjParams(pGNPos, projName, nExpParams, projParams);
    if (pEnd == NULL)
    {
        return NULL;  /* Parsing failed; Error was already issued */
    }

    /*
     *  Convert parameters into the form needed for OGRSpatialReference.  
     */
    WEGConvertAngle(projParams[0]); /* Convert lon radians to degrees */

    /*
     *  Set the spatial reference system of the dataset to use the
     *  appropriate Equirectangular projection.  The center latitude,
     *  false northing, and false easting are always 0.0 in WEG.
     */
    oSRS.SetProjCS(projName);
    WEGSetDefaultGeogCS(&oSRS);
    oSRS.SetEquirectangular(0.0, projParams[0], 0.0, 0.0);

    return pEnd; 
}

/*
 *  Private method to decode GeoNav segment on demand;
 */
CPLErr WEGDataset::DecodeGeoNavSegment()
{
    if (pszGeoNavText == NULL)  /* GeoNav segment not found in Open() */
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                "Failed to find GeoNav information");
        return CE_Failure;
    }

    DEBUG1("Decoding GEONAV text '%s'\n", pszGeoNavText);

    const char * pGNPos = pszGeoNavText;
    while (*pGNPos == ' ') ++pGNPos;  /* Skip initial spaces */
    
    char projType = toupper(*pGNPos);
    pGNPos++;
    while (*pGNPos == ' ') ++pGNPos;

    switch (projType)
    {
      case 'A':  /* Azimuthal Stereographic */
        pGNPos = DecodeStereographicProjection(pGNPos);
        break;
      case 'L':  /* Lambert Conformal Conic */
        pGNPos = DecodeLambertConformalProjection(pGNPos);
        break;
      case 'M':  /* Mercator */
        pGNPos = DecodeMercatorProjection(pGNPos);
        break;
      case 'P':  /* Perfect Satellite */
        pGNPos = DecodePerfectSatelliteProjection(pGNPos);
        break;
      case 'C':  /* Cylindrical Equidistant */
        pGNPos = DecodeCylindricalEquidistantProjection(pGNPos);
        break;
      default:
        CPLError(CE_Failure, CPLE_AppDefined, 
                "Found unsupported projection type %c", projType);
        return CE_Failure;
    }

    if (pGNPos == NULL) /* Check for failure of Decode...Projection method */
    {
        return CE_Failure;  /* Error should have already been issued... */
    }

    /*
     *  Decode the geotransformation parameters, which affect the image's
     *  navigation within its coordinate system.  These parameters are 
     *  the same for all WSI images.
     */
    DEBUG1("Decoding navigation parameters '%s'\n", pGNPos);
    double yUL, xUL, yRes, xRes;
    if (sscanf(pGNPos, "%lg %lg %lg %lg", &yUL, &xUL, &yRes, &xRes) != 4)
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
            "Failed to parse 4 navigation parameters from GeoNav text '%s'", 
            pGNPos);
        return CE_Failure;
    }

    /*
     *  The navigation parameters are in so-called "projection units", 
     *  which are dimensionless.  These need to be scaled into projected
     *  coordinates in units of meters.  The scaling factor varies with
     *  projection.
     */
    double wsiNavScale = WEG_EARTH_RADIUS_METERS;
    if (projType == 'P') /* Perfect Satellite */
    {
        double satHgt = oSRS.GetProjParm(SRS_PP_SATELLITE_HEIGHT, 0.0); 
        if (satHgt == 0.0)
        {
            CPLError(CE_Failure, CPLE_AppDefined, 
                "Failed to retrieve proj parm %s for use in scale factor",
                SRS_PP_SATELLITE_HEIGHT);
            return CE_Failure;
        } 
        wsiNavScale = satHgt; 
    }

    /*
     *  Scale the navigation values so they have units of meters, which is
     *  what is expected in the OGR GeoTransform matrix.
     */
    WEGScaleDistance(yUL,  wsiNavScale);
    WEGScaleDistance(xUL,  wsiNavScale);
    WEGScaleDistance(yRes, wsiNavScale);
    WEGScaleDistance(xRes, wsiNavScale);

    /*
     *   Determine the transformation matrix used to navigate the image
     *   within its projected coordinate system via these equations:
     *
     *     Xgeo = GT(0) + Xpixel*GT(1) + Yline*GT(2)
     *     Ygeo = GT(3) + Xpixel*GT(4) + Yline*GT(5)
     *
     *   In case of north up images, the GT(2) and GT(4) coefficients are zero, 
     *   and the GT(1) is pixel width, and GT(5) is pixel height. 
     *   The (GT(0),GT(3)) position is the top left corner of the top left 
     *   pixel of the raster.
     */
    adfGeoTransform[0] = xUL;   /* projected x-coord of upper left corner */
    adfGeoTransform[1] = xRes;  /* Pixel width(positive=left-to-right order)*/
    adfGeoTransform[2] = 0.0;   /* Unused */
    adfGeoTransform[3] = yUL;   /* projected y-coord of upper left corner */
    adfGeoTransform[4] = 0.0;   /* Unused */ 
    adfGeoTransform[5] = -yRes; /* Pixel height(negative=top-to-bottom order)*/

    return CE_None;  /* Good return, no error */
}

/*
 *  Private method to decode Color Table segment on demand;
 */
CPLErr WEGDataset::DecodeColorTableSegment(int hasCheckSum)
{
    /*  Get a pointer to the start of the color table. */
    GByte * poPos = pabyHeader + nColorTableSegOff;
    poPos += (hasCheckSum ? 4 : 3);

    /*
     *  The color table will have a tuple of RGB values (1 byte each)
     *  for each possible pixel value in the image.  Thus for 4 bit imagery,
     *  there will be 48 (16 x 3) values, and for 8 bit imagery, there will
     *  be 768 (256 x 3) values.  Currently there are no other combinations.
     */
    poColorTable = new GDALColorTable(); /* Create new color table (member)*/
    GDALColorEntry oEntry;

    int nColorCount = (nBitDepth == 4) ? 16 : 256;
    for (int iColor = 0; iColor < nColorCount; ++iColor)
    {
        oEntry.c1 = *poPos++;   /* Red */
        oEntry.c2 = *poPos++;   /* Green */
        oEntry.c3 = *poPos++;   /* Blue */
        oEntry.c4 = 255;        /* Alpha - transparency */
        poColorTable->SetColorEntry(iColor, &oEntry); 
    }

    return CE_None;
}

/*
 *  Decode information about the physical meanings of pixel
 *  values.  This is most typically used with radar data.
 */
CPLErr WEGDataset::DecodeDataLevels()
{
    if (pszDataLevels == NULL)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
            "Error decoding data levels segment: segment is NULL");
        return CE_Failure;
    }

    struct WEGDataLevels
    {
        int           segment_size;         /* 4 bytes */
        unsigned char num_levels;           /* 1 byte */
        char          product_units[20];    /* 20 bytes */
        unsigned char level_prefix[16];     /* 16 bytes */
        int           data_levels[16];      /* 64 bytes */
    };

#ifdef DEBUG_WEB
    const struct WEGDataLevels * psDataLevels = 
            (const struct WEGDataLevels *) pszDataLevels;
#endif

#ifdef OUTPUT_BINARY_DATA_LEVELS
    for (unsigned int j = 0; j < sizeof(struct WEGDataLevels); ++j)
    {
        fprintf(stderr, "%x ", (int) pszDataLevels[j]);
    }
    fprintf(stderr, "\n");
#endif

#ifdef DEBUG_WEG
    /*
     *  The GDAL data models does not give us a good place to 
     *  save this information.  For now, just dump it out 
     *  if we are in debug mode.
     */
    fprintf(stderr, "Segment size = %d\n",
            CPL_LSBWORD32(psDataLevels->segment_size));
    int nLevels = (int) psDataLevels->num_levels;
    fprintf(stderr, "Data Levels, count %d, units %s\n",
            nLevels, psDataLevels->product_units);


    static const char * DL_PREFIXES[] = {
        "  ",   /* 0 = blank */
        "TH",   /* 1 = */
        "ND",   /* 2 = No data */
        "RF",   /* 3 = Range folding */
        "> ",   /* 4 */
        "< ",   /* 5 */
        "+ ",   /* 6 */
        "- ",   /* 7 */
    };
    static const int NPREFIXES = sizeof(DL_PREFIXES)/sizeof(DL_PREFIXES[0]);
        
    for (int i = 0; i < 16; ++i)
    {
        if (psDataLevels->level_prefix[i] <= NPREFIXES)
        {
            fprintf(stderr, "%3d %s %d\n", i, 
                DL_PREFIXES[(int) psDataLevels->level_prefix[i]],
                CPL_LSBWORD32(psDataLevels->data_levels[i]));
        }
        else
        {
            fprintf(stderr, "%3d %0X %d\n", i, 
                (int) psDataLevels->level_prefix[i],
                CPL_LSBWORD32(psDataLevels->data_levels[i]));
        }
    }
#endif

    return CE_None;
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr WEGDataset::GetGeoTransform( double * padfTransform )

{
    /*
     *  Copy the GeoTransform matrix from member variable into the array 
     *  provided by the caller.
     */
    for (int i = 0; i < 6; ++i)
    {
        padfTransform[i] = adfGeoTransform[i]; 
    }
    return CE_None;
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *WEGDataset::GetProjectionRef()

{
    /*
     *  Return the image's coordinate system in WKT format.
     *  This object is responsible for freeing the text string that 
     *  is returned; the caller does NOT own it.
     *
     *  TODO - We should check that the oSRS got populated and has
     *         valid values.
     */
    if (pszProjection == NULL)
    {
        oSRS.exportToWkt(&pszProjection);
    }
    return pszProjection;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *WEGDataset::Open(GDALOpenInfo * poOpenInfo)
{
    /*
     *  Before trying WEGDataset:Open() we first verify that there is    
     *  likely to be a WSI image due to presence of certain byte patterns. 
     */
    short startCode;
    int trans_hdr_len;
    if (poOpenInfo->fpL == NULL || !IdentifyInternal(poOpenInfo, startCode, trans_hdr_len))
        return NULL;

    /* --------------------------------------------------------------------
     *   We have confirmed that the image is in WSI image format. 
     *   Create a corresponding GDALDataset.                             
     * -------------------------------------------------------------------- */
    WEGDataset *poWDS = new WEGDataset();

    /*  
     *  Give ownership of file pointer to the dataset.
     *  WARNING: This only works for a read-only dataset!
     */
    poWDS->fpL = poOpenInfo->fpL;
    poOpenInfo->fpL = NULL;
    VSIFSeekL( poWDS->fpL, 0, SEEK_SET);

    /* -------------------------------------------------------------------- 
     *  Read the beginning of the image into memory.  We assume we can find
     *  all the metadata in this initial part of the image.
     * -------------------------------------------------------------------- */

    int nHeaderAllocSize = 4096;
    if (poWDS->pabyHeader == NULL)
    {
        poWDS->pabyHeader = (GByte *) CPLCalloc(nHeaderAllocSize, 1);
    }
    poWDS->nHeaderSize = VSIFReadL( 
            poWDS->pabyHeader, 1, nHeaderAllocSize, poWDS->fpL);
    poWDS->startCode = startCode;

    if (trans_hdr_len != 0)
    {
        poWDS->nTransHdrSegOff = 0;
        poWDS->nImageStartSegOff = trans_hdr_len;
    }
    else
    {
        poWDS->nTransHdrSegOff = -1;
        poWDS->nImageStartSegOff = 0;
    }

    /*
     *  Set default image dimensions based on the image start code.
     *  These will be overriden if we find an image size segment.
     *  Tiled images do not have a default image size.
     */
    int nx = -1;
    int ny = -1;
    switch (poWDS->startCode)
    {
      case WSI_MED4BIT:  /* DC1: 4-plane, medium res (384 x 240) */
        nx = 384;
        ny = 240;
        poWDS->nBitDepth = 4;
        break;
      case WSI_MED8BIT:  /* DC4: 8-plane, medium res (384 x 240) */
        nx = 384;
        ny = 240;
        poWDS->nBitDepth = 8;
        break;
      case WSI_HIGH4BIT:  /* DC2: 4-plane, high res (768 x 480) */
        nx = 768;
        ny = 480;
        poWDS->nBitDepth = 4;
        break;
      case WSI_HIGH8BIT:  /* NAK: 8-plane, high res (768 x 480) */
        nx = 768;
        ny = 480;
        poWDS->nBitDepth = 8;
        break;
      case WSI_WIDE4BIT:  /* NOT USED: 240 lines, 768 pixels, 4bit */
        nx = 768;
        ny = 240;
        poWDS->nBitDepth = 4;
        break;
      case WSI_WIDE8BIT:  /* NOT USED: 240 lines, 768 pixels, 8bit */
        nx = 768;
        ny = 240;
        poWDS->nBitDepth = 8;
        break;
      case WSI_TILED8BIT:  /*  Variable resolution, 8bit */
        poWDS->nBitDepth = 8;
        break;
    }

    int hdrOff = trans_hdr_len + 1;
    char * textPtr = NULL;
    while (hdrOff < poWDS->nHeaderSize && poWDS->nStartDataOff == -1)
    {
        DEBUG1("Inspecting segment starting at offset %d\n", hdrOff);
        GByte * poHeader = poWDS->pabyHeader + hdrOff;
        if (memcmp((char *)poHeader, WSI_FLAG, 2) != 0)
        {
            DEBUG1("No flag sequence found at offset %d\n", hdrOff);
            break; 
        }

        GByte ctlByte = REMOVE_MSB(poHeader[2]);
        int hasCheckSum = MSB_IS_ON(poHeader[2]);
         
        DEBUG2("Found ctl byte %02X, hasCheckSum = %d\n", ctlByte, hasCheckSum);
        switch (ctlByte)
        {
          /* Metadata cases */
          case WSI_CTL_NONE: /* False flag sequence in data; skip over this */
            break;
          case WSI_CTL_SIZE:  /* Image size segment */
            poWDS->nImageSizeSegOff = hdrOff;
            textPtr = (char*)poHeader + (hasCheckSum ? 4 : 3);
            if (sscanf(textPtr, "%u %u", &ny, &nx) != 2)
            {
                CPLError(CE_Failure, CPLE_AppDefined,
                    "Failed to find image dimensions in segment data '%s'",
                    (char*)poHeader + 3);
                return NULL;
            }
            break;
          case WSI_CTL_LABEL80:  /* Label segment */
            poWDS->nLabelSegOff = hdrOff;
            poWDS->pszLabelText = 
                    (const char *)(hasCheckSum ? poHeader + 4 : poHeader + 3);
            break;
          case WSI_CTL_PROJECTION: /* GeoNav (projection/sector) segment */
            /*
             *  Save a pointer into the GeoNav data; then decode it.
             */
            poWDS->nGeoNavSegOff = hdrOff;
            poWDS->pszGeoNavText = (char*)poHeader + (hasCheckSum ? 4 : 3);
            poWDS->DecodeGeoNavSegment();
            break;
          case WSI_CTL_RGB_COLORS:  /* Color table segment */
            DEBUG1("Found Color Table segment %02X\n", ctlByte);
            poWDS->nColorTableSegOff = hdrOff;
            poWDS->DecodeColorTableSegment(hasCheckSum);
            break;
          case WSI_CTL_TILE_SIZE: /* Tile size segment (only in tiled images)*/
            poWDS->nTileSizeSegOff = hdrOff;
            DEBUG1("Found Tile size segment %02X\n", ctlByte);
            break;
          case WSI_CTL_DATA_LEVELS:  /* color table data levels */
            DEBUG1("Found Color Data Levels segment %02X\n", ctlByte);
            poWDS->pszDataLevels = poHeader + (hasCheckSum ? 5 : 4);
            poWDS->DecodeDataLevels();
            break;
          case WSI_CTL_NAV_DATA:  /* Navigational data */
            DEBUG1("Found Navigational segment %02X\n", ctlByte);
            break;
          case WSI_CTL_OBJECTS:  /* geometric objects */
            DEBUG1("Found Geometric Object segment %02X\n", ctlByte);
            break;
          case WSI_CTL_VALID_TIME:  /* Valid time */
            /*
             *  Valid time follows in ASCII form: HH:MM:SS DD-MON-YYYY GMT
             *  e.g. "12:00:00 10-May-2006 GMT"
             */
            poWDS->pszValidTimeText = 
                    (const char *)(hasCheckSum ? poHeader + 4 : poHeader + 3);
            break;

          /* Start of data cases */
          case WSI_CTL_RAW_LINE:  case WSI_CTL_CMP1_LINE: 
          case WSI_CTL_CMP2_LINE: case WSI_CTL_CMP3_LINE: 
          case WSI_CTL_CMP4_LINE: case WSI_CTL_CMP5_LINE:
          case WSI_CTL_RAW_TILE:  case WSI_CTL_CMP4_TILE: 
          case WSI_CTL_CMP5_TILE: case WSI_CTL_TASC_TILE:
            poWDS->nStartDataOff = hdrOff;
            break;

          /* Erroneous cases */
          case WSI_CTL_END:  /* end of image */
            CPLError(CE_Failure, CPLE_AppDefined,
                    "Found end-of-image control byte %02X before finding data",
                    ctlByte);
            return NULL;
          default:
            CPLError(CE_Failure, CPLE_AppDefined,
                    "Found unsupported control byte %02X in image header",
                    ctlByte);
            return NULL;
        }  /* end of switch on control byte */

        /* Look for start of next segment */
        hdrOff += (hasCheckSum ? 4 : 3);
        DEBUG1("Seaching for next segment, starting at offset %d\n", hdrOff);
        while (hdrOff < poWDS->nHeaderSize -1 &&
               memcmp(poWDS->pabyHeader + hdrOff, WSI_FLAG, 2) != 0)
        {
            hdrOff++;
        }
    }

    DEBUG1("Found start of image data at offset %d\n", poWDS->nStartDataOff);
    if (poWDS->nStartDataOff < 0)
    {
        DEBUG1("Searched to offset %d without finding data segment\n", hdrOff);
    }
    poWDS->nRasterXSize = nx; 
    poWDS->nRasterYSize = ny; 

    /* -------------------------------------------------------------------- */
    /*      Create band information objects.                                */
    /* -------------------------------------------------------------------- */
    poWDS->SetBand( 1, new WEGRasterBand( poWDS, 1 ));

    /* -------------------------------------------------------------------- */
    /*      Initialize any PAM information.                                 */
    /* -------------------------------------------------------------------- */
    poWDS->SetDescription( poOpenInfo->pszFilename );

    if (poWDS->pszValidTimeText != NULL)
    {
        poWDS->SetMetadataItem("valid_time", poWDS->pszValidTimeText);
    }
    if (poWDS->pszLabelText != NULL)
    {
        poWDS->SetMetadataItem("label", poWDS->pszLabelText);
    }

    poWDS->TryLoadXML();

    return( poWDS );
}

/************************************************************************/
/*                        IdentifyInternal()                            */
/************************************************************************/

int WEGDataset::IdentifyInternal(
        GDALOpenInfo *poOpenInfo,
        short& startCode,
        int& trans_hdr_len)
{
    if (poOpenInfo->fpL == NULL || poOpenInfo->nHeaderBytes < 50)
        return FALSE;

    /*
    *  A WSI image should start with either an image start code followed
    *  by a control byte sequence OR a control byte sequence indicating
    *  the presence of a transmission header.  The transmission header
    *  is found on data read from HCSN.
    */
    /*
    *  Check for transmission header
    */
    trans_hdr_len = 0;
    startCode = WEGGetStartCode(poOpenInfo->pabyHeader[0]);
    if (startCode == -1)
    {
        /*
        *  Image does not start with a start code.
        *  See if the image starts with a transmission header.
        */
        if (memcmp(poOpenInfo->pabyHeader, WSI_FLAG, 2) != 0)
        {
            DEBUG1("Found %02X instead of trans hdr ctl byte at offset 2\n",
                poOpenInfo->pabyHeader[2]);
            return FALSE;  /* No WSI image flag sequence */
        }

        unsigned char ctlByte = poOpenInfo->pabyHeader[2];
        if (REMOVE_MSB(ctlByte) != WSI_CTL_HEADER)
        {
            DEBUG1("Found %02X instead of trans hdr ctl byte at offset 2\n",
                poOpenInfo->pabyHeader[2]);
            return FALSE;  /* No transmission header control byte*/
        }

        /*
        *  Skip over the transmission header and make sure we have
        *  a valid image start code
        */
        trans_hdr_len = 3;
        if (MSB_IS_ON(ctlByte))
        {
            trans_hdr_len++;  /* Add one byte for checksum */
        }
        trans_hdr_len += (int)(poOpenInfo->pabyHeader[trans_hdr_len]);
        trans_hdr_len++; /* header length does not include length itself */

        startCode = WEGGetStartCode(poOpenInfo->pabyHeader[trans_hdr_len]);
        if (startCode == -1)
        {
            DEBUG2("Found %02X instead of image start code at offset %d\n",
                poOpenInfo->pabyHeader[trans_hdr_len], trans_hdr_len);
            return FALSE;  /* Bad code after transmission header */
        }
    }  /* end of check for transmission header */

    /*
    *  Make sure the image start code is followed by a valid flag sequence
    *  and control byte.
    */
    int check_pos = trans_hdr_len + 1;
    if (memcmp((char *)poOpenInfo->pabyHeader + check_pos, WSI_FLAG, 2) != 0)
    {
        DEBUG1("No flag sequence found at offset %d\n", check_pos);
        return FALSE;
    }

    check_pos += 2;
    int ctlByte = WEGGetControlByte(poOpenInfo->pabyHeader[check_pos]);
    if (ctlByte == -1)
    {
        DEBUG2("Invalid control byte %02X at offset %d\n",
            poOpenInfo->pabyHeader[check_pos], check_pos);
        return FALSE;  /* Invalid control byte */
    }

    return TRUE;
}

/************************************************************************/
/*                                Identify()                            */
/************************************************************************/

int WEGDataset::Identify(GDALOpenInfo * poOpenInfo)
{
    short unused1;
    int unused2;
    return IdentifyInternal(poOpenInfo, unused1, unused2);
}

/************************************************************************/
/*                          GDALRegister_WEG()                        */
/************************************************************************/

void GDALRegister_WEG()

{
    GDALDriver	*poDriver;

    static const char FMT_DESC[] = "WEG";
    if( GDALGetDriverByName( FMT_DESC ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( FMT_DESC );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "WSI Express Graphics (.weg)" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_various.html#WEG" );
        poDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "weg" );

        poDriver->pfnOpen = WEGDataset::Open;
        poDriver->pfnIdentify = WEGDataset::Identify;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}
