#ifndef SIGHT_MODEL_H
#define SIGHT_MODEL_H

#include <QImage>
#include <QObject>

namespace domain
{
    class SightModel : public QObject
    {
        Q_OBJECT
    public:
        SightModel(QObject* parent = nullptr);
        ~SightModel();

        void setFrame(const QImage& frame);
        QImage frame() const;
        QSize size() const;

    signals:
        void frameChanged(const QImage& frame);

    private:
        void readSettings();

    private:
        class Impl;
        Impl* d;
    };
}

#endif //SIGHT_MODEL_H
