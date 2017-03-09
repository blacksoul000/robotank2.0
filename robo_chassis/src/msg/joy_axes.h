#ifndef JOY_AXES_H
#define JOY_AXES_H

#include <QVector>

struct JoyAxes
{
    static const int axesCount = 8;
    QVector< double > axes = QVector< double >(axesCount);
};

#endif // JOY_AXES_H
