#ifndef BLENDIAN_H
#define BLENDIAN_H

#include <stdio.h>

class BLEndian
{
public:
    BLEndian();
    ~BLEndian();

    static int swapInt(int source);
    static int freadInt(FILE *fp);
    static short swapShort(short source);
    static short freadShort(FILE *fp);
    static float swapFloat(float source);
    static float freadFloat(FILE *fp);
};

#endif // BLENDIAN_H
