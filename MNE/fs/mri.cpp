#include "mri.h"
#include <QtDebug>

Mri::Mri()
{

}

// width, height, depth, type
void Mri::allocHeader(int x, int y, int z, int t)
{
    imNr0 = 1;
    imNr1 = depth;
    fov = x;
    thick = 1.0;
    scale = 1;
    roi.dX = width = x;
    roi.dY = height = y;
    roi.dZ = depth = z;
    yInvert = 1 ;
    xSize = ySize = zSize = 1;
    type = t;
    nFrames = 1;
    xI = yI = zI = 0;
    slices = 0;
    ps = 1;
    xStart = -width/2.0;
    xEnd = width/2.0;
    yStart = -height/2.0;
    yEnd = height/2.0;
    zStart = -depth/2.0 ;
    zEnd = depth/2 ;
    //
    x_r = -1;
    x_a = 0.;
    x_s = 0.;
    //
    y_r = 0.;
    y_a = 0.;
    y_s = -1;
    //
    z_r = 0.;
    z_a = 1.;
    z_s = 0.;
    //
    c_r = c_a = c_s = 0.0;
    ras_good_flag = 0;
    brightness = 1;
//    register_mat = 0; // datatype MATRIX not yet defined
    subject_name = '\0';
    path_to_t1 = '\0';
    fname_format = '\0';
    gdf_image_stem = '\0';
    tag_data = 0;
    tag_data_size = 0;

//    i_to_r__ = extract_i_to_r(mri); // todo
//    r_to_i__ = extract_r_to_i(mri); // todo
}

void Mri::allocSequence(int x, int y, int z, int t, int f)
{

    // todo: what is the mri allocChunk method for? check, use and implement if useful...

    BUFTYPE *buf;

    qCritical() << "this is critical";
    return;

    if (x<=0 || y<=0 || z<=0)
        qWarning() << "allocSequence("
                 << x << ", "
                 << y << ", "
                 << z << ", "
                 << t << ", "
                 << f << "): bad parameters";
    allocHeader(x, y, z, t);
    //todo: initHeader();
    nFrames = f;
    //todo: initIndices();
    //todo: outside_val = 0;
    slices = (BUFTYPE ***)calloc(z*nFrames, sizeof(BUFTYPE **)) ;
    if (!slices)
    {
        qWarning() << "allocSequence(): out of memory. could not allocate "
                 << z*sizeof(BUFTYPE *)
                 << " bytes for "
                 << depth
                 << " slices";
    }

    // allocate each slice
    int slice;
    for (slice=0; slice<z*nFrames; slice++)
    {
        // allocate pointer to array of rows
        slices[slice] = (BUFTYPE **)calloc(height, sizeof(BUFTYPE *));
        if (!slices[slice])
        {
//            qWarning() << "allocSequence(): could not allocate"
//                     << (height*sizeof(BUFTYPE *)
//                     << "bytes for "
//                     << slice
//                     << " slices";
        }

        // allocate each row
        int row;
        for (row=0; row<height; row++)
        {
//            switch (type)
//            {
//            case MRI_BITMAP:
//              slices[slice][row] = (BUFTYPE *)calloc(width/8,sizeof(BUFTYPE));
//              break;
//            case MRI_UCHAR:
//              slices[slice][row] = (BUFTYPE *)calloc(width,sizeof(BUFTYPE));
//              break;
//            case MRI_FLOAT:
//              slices[slice][row] = (BUFTYPE *)calloc(width, sizeof(float));
//              break;
//            case MRI_INT:
//              slices[slice][row] = (BUFTYPE *)calloc(width, sizeof(int));
//              break;
//            case MRI_SHORT:
//              slices[slice][row] = (BUFTYPE *)calloc(width, sizeof(short));
//              break;
//            case MRI_LONG:
//              slices[slice][row] = (BUFTYPE *)calloc(width, sizeof(long));
//            }

            if (!slices[slice][row])
            {
                qWarning() << "could not allocate "
                         << row
                         << "th row in "
                         << slice
                         << "th slice";
            }

        }
    }
}



Mri::~Mri()
{

}

