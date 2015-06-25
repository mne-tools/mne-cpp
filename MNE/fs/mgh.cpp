//=============================================================================================================
/**
* @file     mgh.h
* @author   Carsten Boensel <carsten.boensel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*           Bruce Fischl <fischl@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June, 2015
*
* @section  LICENSE
*
* Copyright (C) June, 2015 Carsten Boensel and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief     Mgh class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_global.h"
#include "mgh.h"
#include "blendian.h"
#include "3rdParty/miniz.c"

#include <QCoreApplication>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>

#include <string>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Mgh::Mgh()
{

}

//*************************************************************************************************************

Mri Mgh::loadMGH(QString fName, std::vector<int> slices, int frame, bool headerOnly)
{
    Mri mri;
//    mri.nFrames = 2; // for testing purposes

    // get file extension
    QFileInfo fi(fName);
    QString ext = fi.completeSuffix(); // ext = "mgz"

    // check if file is compressed
    bool gZipped = (QString::compare(ext,"mgz", Qt::CaseInsensitive)==0
            || QString::compare(ext,"gz", Qt::CaseInsensitive)==0);

    // uncompress if needed (gzip is used)
    if (gZipped)
    {
        // generate unique temporary absolute name destination file
        QFileInfo fileInfo(fName);
        QTemporaryFile tempFile(QDir::tempPath() + "/" + fileInfo.completeBaseName()
                + "." + fileInfo.suffix());
        tempFile.open();
        QString uncompFName = tempFile.fileName() + ".mgh";
        tempFile.close();

        // call function
        unGz(fName, uncompFName);

        // overwrite older fName
        fName = uncompFName;
    }


    // c-style file reading method
    QFile myFile(fName);
    myFile.open(QIODevice::ReadOnly);

    std::string stdFName = fName.toStdString(); // convert to std string
    FILE* fp = fopen(stdFName.c_str(), "rb"); // rb because non-text-file is opened, b because faithfully read file as it is and prevend ascii confusion

    int nRead, nDimX, nDimY, nDimZ, nFrames, type, dof;

    nRead = BLEndian::freadInt(fp);

    // width, height, depth
    nDimX = BLEndian::freadInt(fp);
    nDimY = BLEndian::freadInt(fp);
    nDimZ = BLEndian::freadInt(fp);

    nFrames = BLEndian::freadInt(fp);
    type = BLEndian::freadInt(fp);
    dof = BLEndian::freadInt(fp);

    //todo: check "slices" and "frames" vars

    int UNUSED_SPACE_SIZE = 256;
    int USED_SPACE_SIZE = (3*4+4*3*4); // space for ras transform

    int unused_space_size = UNUSED_SPACE_SIZE-2;

    int ras_good_flag = BLEndian::freadShort(fp);
    // delta
    float xSize, ySize, zSize;
    // Mdc
    float x_r, x_a, x_s,
            y_r, y_a, y_s,
            z_r, z_a, z_s,
            c_r, c_a, c_s;

    if (ras_good_flag>0) // has RAS and voxel size info
    {
        unused_space_size -= USED_SPACE_SIZE ;

        // delta
        xSize = BLEndian::freadFloat(fp);
        ySize = BLEndian::freadFloat(fp);
        zSize = BLEndian::freadFloat(fp);

        // Mdc
        x_r = BLEndian::freadFloat(fp); x_a = BLEndian::freadFloat(fp); x_s = BLEndian::freadFloat(fp);
        y_r = BLEndian::freadFloat(fp); y_a = BLEndian::freadFloat(fp); y_s = BLEndian::freadFloat(fp);
        z_r = BLEndian::freadFloat(fp); z_a = BLEndian::freadFloat(fp); z_s = BLEndian::freadFloat(fp);
        c_r = BLEndian::freadFloat(fp); c_a = BLEndian::freadFloat(fp); c_s = BLEndian::freadFloat(fp);
    }


    // determine number of bytes per voxel
    int nBytesPerVox;
    switch (type)
    {
        case MRI_FLOAT:
            nBytesPerVox = sizeof(float);
            break;
        case MRI_UCHAR:
            nBytesPerVox = sizeof(char);
            break;
        case MRI_SHORT:
            nBytesPerVox = sizeof(short);
            break;
        case MRI_INT:
            nBytesPerVox = sizeof(int);
            break;
        case MRI_BITMAP:
            nBytesPerVox = sizeof(float);
            nFrames = 9;
            break;
    }

    int nBytesPerSlice = nDimX*nDimY*nBytesPerVox; // number of bytes per slice
    int nVol = nDimX*nDimY*nDimZ*nFrames; // number of volume elements over time

    if (headerOnly)
    {
        // initialize vars and set volume size in mri
//        mri.STRLEN = 20; // change this to length of fName
        mri.allocHeader(nDimX, nDimY, nDimZ, type);
        mri.dof = dof;
        mri.nFrames = nFrames;

        if (gZipped) // pipe cannot seek
        {
            int count;
            for (count=0; count < nVol*nBytesPerVox; count++)
                fgetc(fp);
        }
        else
            fseek(fp, nVol*nBytesPerVox, 1);
    }
    else // not header only option
    {
        int start_frame, end_frame;

        if (frame >= 0)
        {
            start_frame = end_frame = frame;
            if (gZipped) // pipe cannot seek
            {
                int count;
                for (count=0; count < frame*nDimX*nDimY*nDimZ*nBytesPerVox; count++)
                    fgetc(fp);
            }
            else
                fseek(fp, frame*nDimX*nDimY*nDimZ*nBytesPerVox, 1);
            nFrames = 1;
        }
        else
        { // frame < 0 means to read in as many frames,
          // has been defined to prevend using a vector.
            nFrames = frame*-1;
            start_frame = 0; end_frame = nFrames-1;
            if (VERBOSE)
                qDebug() << "read" << nFrames << "frames\n";
        }
        qDebug() << "read" << nFrames << "frames\n";
        BUFTYPE *buf = (BUFTYPE*)calloc(nBytesPerSlice, sizeof(BUFTYPE));
        mri.allocSequence(nDimX, nDimY, nDimZ, type, nFrames);
        mri.dof = dof;

//        for (frame=start_frame; frame<=end_frame; frame++)
//        {
//            if (fread(buf, sizeof(char), bytes, fp) != bytes)
//            {
//                fclose(fp);
//                free(buf);

//            }
//        }

//        // ------ Read in the entire volume -------
//        switch (type)
//        {
//        case MRI_INT:
//        for (i = y = 0 ; y < height ; y++)
//          {
//            for (x = 0 ; x < width ; x++, i++)
//              {
//                ival = orderIntBytes(((int *)buf)[i]) ;
//                MRIIseq_vox(mri,x,y,z,frame-start_frame) = ival ;
//              }
//          }
//        break ;
//        case MRI_SHORT:
//        for (i = y = 0 ; y < height ; y++)
//          {
//            for (x = 0 ; x < width ; x++, i++)
//              {
//                sval = orderShortBytes(((short *)buf)[i]) ;
//                MRISseq_vox(mri,x,y,z,frame-start_frame) = sval ;
//              }
//          }
//        break ;
//        case MRI_TENSOR:
//        case MRI_FLOAT:
//        for (i = y = 0 ; y < height ; y++)
//          {
//            for (x = 0 ; x < width ; x++, i++)
//              {
//                fval = orderFloatBytes(((float *)buf)[i]) ;
//                MRIFseq_vox(mri,x,y,z,frame-start_frame) = fval ;
//              }
//          }
//        break ;
//        case MRI_UCHAR:
//        local_buffer_to_image(buf, mri, z, frame-start_frame) ;
//        break ;
//        default:
//        errno = 0;
//        ErrorReturn(NULL,
//                    (ERROR_UNSUPPORTED, "mghRead: unsupported type %d",
//                     mri->type)) ;
//        break ;
    }







    fclose(fp);

    // xstart, xend, ystart, yend, zstart, zend are not stored
    mri.xStart = -mri.width/2*mri.xSize;
    mri.xEnd = mri.xEnd/2*mri.xSize;
    mri.yStart = -mri.height/2*mri.ySize;
    mri.yEnd = mri.height/2*mri.ySize;
    mri.zStart = -mri.depth/2*mri.zSize;
    mri.zEnd = mri.depth/2*mri.zSize;
    mri.fName = fName;

    return mri;
}

//*************************************************************************************************************

// function to unpack compressed data
//      corrupted data error, try new approach with other lib
int Mgh::unGz(QString gzFName, QString unGzFName)
{

// qUncompress does not work for gZip files, so we use miniz instead, a small and compact zlib clone

//    QFile inFile(gzFName);
//    QFile outFile(fName);
//    inFile.open(QIODevice::ReadOnly);
//    outFile.open(QIODevice::WriteOnly);

//    QByteArray compressedData = inFile.readAll();
//    QByteArray uncompressedData = qUncompress(compressedData); // does not work, because zlib and gzip use different header
//    outFile.write(uncompressedData);

//    inFile.close();
//    outFile.close();

// usage of miniz functionality, derived from miniz example 3

printf("miniz.c version: %s\n", MZ_VERSION);

#define my_max(a,b) (((a) > (b)) ? (a) : (b))
#define my_min(a,b) (((a) < (b)) ? (a) : (b))

#define BUF_SIZE (1024 * 1024)

static quint8 s_inbuf[BUF_SIZE];
static quint8 s_outbuf[BUF_SIZE];

// initialize vars
int level = Z_BEST_COMPRESSION;
z_stream stream;
int p = 1;


// convert qString filenames to const char *
QByteArray gzByteArray = gzFName.toLatin1();
const char *pSrc_filename = gzByteArray.data();
QByteArray unGzByteArray = unGzFName.toLatin1();
const char *pDst_filename = unGzByteArray.constData();
printf("Input File: \"%s\"\nOutput File: \"%s\"\n", pSrc_filename, pDst_filename);

// Open input file.
FILE *pInfile = fopen(pSrc_filename, "rb");
if (!pInfile)
{
  printf("Failed opening input file!\n");
  return EXIT_FAILURE;
}
long file_loc;
if ((file_loc < 0) || (file_loc > INT_MAX))
{
   // This is not a limitation of miniz or tinfl.
   printf("File is too large to be processed by this example.\n");
   return EXIT_FAILURE;
}
uint infile_size = (uint)file_loc;
printf("Input file size: %u\n", infile_size);

// Open output file.
FILE *pOutfile = fopen(pDst_filename, "wb");
if (!pOutfile)
{
  printf("Failed opening output file!\n");
  return EXIT_FAILURE;
}

// Init the z_stream
memset(&stream, 0, sizeof(stream));
stream.next_in = s_inbuf;
stream.avail_in = 0;
stream.next_out = s_outbuf;
stream.avail_out = BUF_SIZE;

// Decompression.
uint infile_remaining = infile_size;

if (inflateInit(&stream))
{
  printf("inflateInit() failed!\n");
  return EXIT_FAILURE;
}

for ( ; ; )
{
  int status;
  if (!stream.avail_in)
  {
    // Input buffer is empty, so read more bytes from input file.
    uint n = my_min(BUF_SIZE, infile_remaining);

    if (fread(s_inbuf, 1, n, pInfile) != n)
    {
      printf("Failed reading from input file!\n");
      return EXIT_FAILURE;
    }

    stream.next_in = s_inbuf;
    stream.avail_in = n;

    infile_remaining -= n;
  }

  status = inflate(&stream, Z_SYNC_FLUSH);

  if ((status == Z_STREAM_END) || (!stream.avail_out))
  {
    // Output buffer is full, or decompression is done, so write buffer to output file.
    uint n = BUF_SIZE - stream.avail_out;
    if (fwrite(s_outbuf, 1, n, pOutfile) != n)
    {
      printf("Failed writing to output file!\n");
      return EXIT_FAILURE;
    }
    stream.next_out = s_outbuf;
    stream.avail_out = BUF_SIZE;
  }

  if (status == Z_STREAM_END)
    break;
  else if (status != Z_OK)
  {
    printf("inflate() failed with status %i!\n", status);
    return EXIT_FAILURE;
  }
}

if (inflateEnd(&stream) != Z_OK)
{
  printf("inflateEnd() failed!\n");
  return EXIT_FAILURE;
}

fclose(pInfile);
if (EOF == fclose(pOutfile))
{
  printf("Failed writing to output file!\n");
  return EXIT_FAILURE;
}

printf("Total input bytes: %u\n", (mz_uint32)stream.total_in);
printf("Total output bytes: %u\n", (mz_uint32)stream.total_out);
printf("Success.\n");
return EXIT_SUCCESS;

}

//*************************************************************************************************************

Mgh::~Mgh()
{

}

