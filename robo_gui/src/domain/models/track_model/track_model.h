#ifndef TRACK_MODEL_H
#define TRACK_MODEL_H

#include <QObject>
#include <QRectF>

namespace domain
{
    class TrackModel : public QObject
    {
        Q_OBJECT
    public:
        TrackModel(QObject* parent = nullptr);
        ~TrackModel();

        void setTargetRect(const QRectF& rect);
        QRectF targetRect() const;

        void nextCaptureSize();
        QSize captureSize() const;

        void setCaptureRect(const QRectF& rect);
        QRectF captureRect() const;

        void setTracking(bool tracking);
        bool isTracking() const;

    signals:
        void targetRectChanged(const QRectF& rect);
        void captureSizeChanged(const QSize& rect);
        void trackRequest(const QRectF& rect);
        void trackingStatusChanged(bool tracking);

    private:
        class Impl;
        Impl* d;
    };
} //namespace domain

#endif //TRACK_MODEL_H
