#ifndef VIDEO_PRESENTER_H
#define VIDEO_PRESENTER_H

#include <QObject>

namespace domain
{
    class RoboModel;
} // namespace domain

namespace presentation
{
    class VideoPresenter : public QObject
    {
        Q_OBJECT
		Q_PROPERTY(QObject* surface READ surface CONSTANT)

    public:
		VideoPresenter(domain::RoboModel* model, QObject* parent = nullptr);
        ~VideoPresenter();

        QObject* surface() const;

	public slots:
        void play();
        void stop();
        void setUri(const QString & uri);

    private:
//        bool onBusMessage(GstBus* bus GstMessage* message);

    private:
        class Impl;
        Impl* d;
    };
} // namespace presentation

#endif //VIDEO_PRESENTER_H
