#ifndef MGH_H
#define MGH_H

#include "mri.h"

#include <vector>


class Mgh
{
public:
    Mgh();
    ~Mgh();

    static Mri loadMGH(QString fName, std::vector<int> slices, int frame, bool headerOnly);
    static void unGz(QString gzFName, QString fName);

};

#endif // MGH_H
