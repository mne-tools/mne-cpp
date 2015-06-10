#include "blendian.h"


BLEndian::BLEndian()
{

}

// big und little endian swap
int BLEndian::swapInt(int source)
{
    unsigned char *csource = (unsigned char *) &source;
    int result;
    unsigned char *cresult = (unsigned char *) &result;
    cresult[0] = csource[3];
    cresult[1] = csource[2];
    cresult[2] = csource[1];
    cresult[3] = csource[0];
    return result;
}

// read Integer value from file
int BLEndian::freadInt(FILE *fp)
{
    int i, size_i;

    size_i = fread(&i, sizeof(int), 1, fp);
    i = swapInt(i);
    return i;
}

short BLEndian::swapShort(short source)
{
    unsigned char *csource = (unsigned char *)&source;
    short result;
    unsigned char *cresult = (unsigned char *)&result;
    cresult[0] = csource[1];
    cresult[1] = csource[0];
    return result;
}

short BLEndian::freadShort(FILE *fp)
{
    int size_s;
    short s;

    size_s = fread(&s, sizeof(short), 1, fp);
    s = swapShort(s);
    return s;
}

float BLEndian::swapFloat(float source)
{
    char *csource = (char*) &source;
    float result;
    char *cresult = (char*) &result;

    // swap the bytes into a temporary buffer
    cresult[0] = csource[3];
    cresult[1] = csource[2];
    cresult[2] = csource[1];
    cresult[3] = csource[0];

    return result;
}

float BLEndian::freadFloat(FILE *fp)
{
    float f;
    int size_f;

    size_f = fread(&f, sizeof(float), 1, fp);
    f = swapFloat(f);
    return f;
}

BLEndian::~BLEndian()
{

}

