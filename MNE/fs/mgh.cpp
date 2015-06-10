#include "mgh.h"

#include <blendian.h>

#include <QCoreApplication>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <string>

Mgh::Mgh()
{

}

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
        QString uncompFName = QDir::tempPath() + "/001.mgh"; // todo: automatic assignment of file name
        unGz(fName, uncompFName);

        qDebug() << "Done uncompressing.";
        qDebug() << uncompFName;
        fName = uncompFName;
    }

//    // experiment to read file using QDataStream, still not working
//    QFile file(fName);
//    QDataStream in(&file);
//    file.open(QIODevice::ReadOnly);
//    QString info = "";
//    in >> info;
//    file.close();

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

        // ------ Read in the entire volume -------
//        switch (type)
//        {
//        case MRI_FLOAT:
//            int dothis;
//            break;
//        case MRI_UCHAR:
//            int dothat;
//            break;
//        case MRI_SHORT:
//            int dothis;
//            break;
//        case MRI_INT:
//            int dothis;
//            break;
//        }
    }



//    if (gZipped)
//    {
//        // delete temporary mgh file (ml: line 180 / c: line ┤).
//    }
//    else
//    {

//    }
//    nFrames = 1;

    //------------------ Read in the entire volume ----------------


    return mri;
}

// function to unpack compressed data
//      corrupted data error, try new approach with other lib
void Mgh::unGz(QString gzFName, QString fName)
{
    // http://stackoverflow.com/questions/2690328/qt-quncompress-gzip-data
    // http://stackoverflow.com/questions/5741657/error-decompressing-gzip-data-using-qt
    // http://www.zlib.net/zlib_how.html

    QFile inFile(gzFName);
    QFile outFile(fName);
    inFile.open(QIODevice::ReadOnly);
    outFile.open(QIODevice::WriteOnly);

    QByteArray compressedData = inFile.readAll();
    QByteArray uncompressedData = qUncompress(compressedData); // does not work, because zlib and gzip use different header
    outFile.write(uncompressedData);

    inFile.close();
    outFile.close();
}


Mgh::~Mgh()
{

}

