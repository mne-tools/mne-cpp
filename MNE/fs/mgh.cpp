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

//#define MINIZ_HEADER_FILE_ONLY
//#include "3rdParty/miniz.c"

//extern "C" {
#include "3rdParty/tinfl.c"
//}

#include <stdio.h>
#include <iostream>
#include <limits.h>

#include <QCoreApplication>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>

#include <string>

//*************************************************************************************************************

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;

#define my_max(a,b) (((a) > (b)) ? (a) : (b))
#define my_min(a,b) (((a) < (b)) ? (a) : (b))

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Mgh::Mgh()
{

}

//*************************************************************************************************************

//QList<MatrixXd>
Mri Mgh::loadMGH(QString fName, Eigen::VectorXi slices, int frame, bool headerOnly)
{
    Mri mri;
    //QList<MatrixXd> sliceDataList;
    QList<FSLIB::SliceData> sliceDataList;

    int sliceCount = 0;

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

    int nRead, nDimX, nDimY, nDimZ, nFrames, type, dof; // define temporal vars

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

    // define temporal vars to save read data
    int iVal;
    short sVal;
    float fVal;

    if (headerOnly)
    {
        // initialize vars and set volume size in mri
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
          // has been designed like this in freesurfer
          // to prevend using a vector.
            nFrames = frame*-1;
            start_frame = 0; end_frame = nFrames-1;
            if (VERBOSE)
                qDebug() << "read" << nFrames << "frames\n";
        }
        qDebug() << "read" << nFrames << "frames\n";
        BUFTYPE *buf = (BUFTYPE*)calloc(nBytesPerSlice, sizeof(BUFTYPE));
        mri.allocSequence(nDimX, nDimY, nDimZ, type, nFrames);
        mri.dof = dof;

        // define further temporal vars to save read data
        int x, y, z, i;
        MatrixXd slice(nDimX, nDimY);
        SliceData sliceData(slice);

        for (frame=start_frame; frame<=end_frame; frame++)
        {
            for (z=0; z<nDimZ; z++)
            {
                if (fread(buf, sizeof(char), nBytesPerSlice, fp) != nBytesPerSlice)
                {
                    fclose(fp);
                    free(buf);
                    qDebug() << "loadMGH(" << fName << "): could not read "
                             << nBytesPerSlice << "at slice" << z;
                }
                sliceData.setSliceIdx(z);

                // ------ Read in the entire volume -------
                switch (type)
                {
                case MRI_INT:
                    qDebug() << "### Debug watch out Ints are coming! "
                             << sliceCount;
                    for (i = y = 0 ; y < nDimY ; y++)
                      {
                        for (x = 0 ; x < nDimX ; x++, i++)
                          {
                            // voxel access
                            iVal = BLEndian::swapInt(((int *)buf)[i]);
    //                        qDebug() << iVal << " ";
                            slice(x,y) = iVal;
                          }
                      }
                    ++sliceCount;
                    sliceData.setSliceMatrix(slice); // maybe fill already "sliceData" instead of "slice" above
                    sliceDataList.append(sliceData);
                    break ;
                case MRI_SHORT:
                    qDebug() << "### Debug watch out Shorts are coming! "
                             << sliceCount;
                    for (i = y = 0 ; y < nDimY ; y++)
                      {
                        for (x = 0 ; x < nDimX ; x++, i++)
                          {
                            // voxel access
                            sVal = BLEndian::swapShort(((short *)buf)[i]);
                            qDebug() << sVal << " at pos " << x << "|" << y <<  ". ";
                            slice(x,y) = sVal;
                          }
                      }
                    ++sliceCount;
                    sliceData.setSliceMatrix(slice);
                    sliceDataList.append(sliceData);
                    break ;
                case MRI_TENSOR:
                case MRI_FLOAT:
                    qDebug() << "### Debug watch out Floats are coming! "
                             << sliceCount;
                    for (i = y = 0 ; y < nDimY ; y++)
                    {
                       for (x = 0 ; x < nDimX ; x++, i++)
                       {
                           // voxel access
                           fVal = BLEndian::swapFloat(((float *)buf)[i]);
//                           qDebug() << fVal << " ";
                           slice(x,y) = fVal;
//                           ((float*)(mri->slices[z+((n)*mri->depth)][y]))[x] = fval;
//                           MRIFseq_vox(mri,x,y,z,frame-start_frame) = fval;
                       }
                    }
                    ++sliceCount;
                    sliceData.setSliceMatrix(slice);
                    sliceDataList.append(sliceData);
                    break;
                case MRI_UCHAR:
                    //local_buffer_to_image(buf, mri, z, frame-start_frame); // todo
                    break;
                default:
                    qDebug() << "loadMGH: unsupported type" << mri.type; // return error number!?
                    break;
                }
            }
        }
        if (buf)
            free(buf);
    }

    if(ras_good_flag > 0)
    {
        // write delta
        mri.xSize = xSize;
        mri.ySize = ySize;
        mri.zSize = zSize;

        // Mdc
        mri.x_r = x_r;
        mri.x_a = x_a;
        mri.x_s = x_s;

        mri.y_r = y_r;
        mri.y_a = y_a;
        mri.y_s = y_s;

        mri.z_r = z_r;
        mri.z_a = z_a;
        mri.z_s = z_s;

        mri.c_r = c_r;
        mri.c_a = c_a;
        mri.c_s = c_s;

        mri.ras_good_flag = 1;
    }
    else
    {
        qDebug() << "-----------------------------------------------------------------\n"
                 << "Could not find the direction cosine information.\n"
                 << "Will use the CORONAL orientation.\n"
                 << "If not suitable, please provide the information in"
                 << fName
                 << ".\n"
                 << "-----------------------------------------------------------------\n";
        //todo: set direction cosine (mri, MRI_CORONAL);
    }
    // read TR, Flip, TE, TI, FOV
    if (BLEndian::freadFloatEx(&mri.tR, fp)){
      if (BLEndian::freadFloatEx(&fVal, fp))
        {
          mri.flip_angle = fVal;
          // flip_angle is double. I cannot use the same trick.
          if (BLEndian::freadFloatEx(&mri.tE, fp))
            if (BLEndian::freadFloatEx(&mri.tI, fp))
              BLEndian::freadFloatEx(&mri.fov, fp);
        }
    }

    //  todo: tag reading and transformations

    fclose(fp);

    // xstart, xend, ystart, yend, zstart, zend are not stored
    mri.xStart = -mri.width/2*mri.xSize;
    mri.xEnd = mri.xEnd/2*mri.xSize;
    mri.yStart = -mri.height/2*mri.ySize;
    mri.yEnd = mri.height/2*mri.ySize;
    mri.zStart = -mri.depth/2*mri.zSize;
    mri.zEnd = mri.depth/2*mri.zSize;
    mri.fName = fName;

    mri.slices = sliceDataList;
    //mri.slices.getNewCube(sliceDataList);
    //mri.cubeData.appendSlice

    return mri;
//    return sliceDataList;
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

    // try to derive functionality from miniz example 4 - "0" bytes in file, return error "0"

    // convert qString filename to const char *
    QByteArray gzByteArray = gzFName.toLatin1();
    const char *pSrc_filename = gzByteArray.data();

    // Open input file.
    FILE *pInfile = fopen(pSrc_filename, "rb");
    if (!pInfile)
    {
      printf("Failed opening input file!\n");
      return EXIT_FAILURE;
    }

    // Determine input file's size.
    fseek(pInfile, 0, SEEK_END);
    long file_loc = ftell(pInfile);
    fseek(pInfile, 0, SEEK_SET);

    // check file size limitation
    if ((file_loc < 0) || (file_loc > INT_MAX))
    {
       // This is not a limitation of miniz or tinfl, but this example.
       printf("File is too large to be processed.\n");
       return EXIT_FAILURE;
    }

    uint infile_size = (uint)file_loc;

    uint8 *pCmp_data = (uint8 *)malloc(infile_size);
    if (!pCmp_data)
    {
      printf("Out of memory!\n");
      return EXIT_FAILURE;
    }
    if (fread(pCmp_data, 1, infile_size, pInfile) != infile_size)
    {
      printf("Failed reading input file!\n");
      return EXIT_FAILURE;
    }

    // convert qString filename to const char *
    QByteArray unGzByteArray = unGzFName.toLatin1();
    const char *pDst_filename = unGzByteArray.constData();

    // Open output file.
    FILE *pOutfile = fopen(pDst_filename, "wb");
    if (!pOutfile)
    {
      printf("Failed opening output file!\n");
      return EXIT_FAILURE;
    }

    // print filenames and size
    printf("Input File: \"%s\"\nOutput File: \"%s\"\n", pSrc_filename, pDst_filename);
    printf("Input file size: %u\n", infile_size);

    size_t in_buf_size = infile_size;
    int status = tinfl_decompress_mem_to_callback(pCmp_data,
                                                  &in_buf_size,
                                                  tinfl_put_buf_func,
                                                  pOutfile,
                                                  TINFL_FLAG_PARSE_ZLIB_HEADER);
    if (!status)
    {
      printf("tinfl_decompress_mem_to_callback() failed with status %i!\n", status);
      return EXIT_FAILURE;
    }

    uint outfile_size = ftell(pOutfile);

    fclose(pInfile);
    if (EOF == fclose(pOutfile))
    {
      printf("Failed writing to output file!\n");
      return EXIT_FAILURE;
    }

    printf("Total input bytes: %u\n", (uint)in_buf_size);
    printf("Total output bytes: %u\n", outfile_size);
    printf("Success.\n");
    return EXIT_SUCCESS;
}

//*************************************************************************************************************

Mgh::~Mgh()
{

}

