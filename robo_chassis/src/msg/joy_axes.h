#ifndef JOY_AXES_H
#define JOY_AXES_H

#include <QVector>

struct JoyAxes
{
    static const int axesCount = 18;
    QVector< short > axes = QVector< short >(axesCount);
};

#endif // JOY_AXES_H
