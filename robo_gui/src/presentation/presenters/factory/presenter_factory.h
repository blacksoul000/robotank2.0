#ifndef PRESENTER_FACTORY_H
#define PRESENTER_FACTORY_H

#include <QObject>

namespace domain
{
    class RoboModel;
}

namespace presentation
{
    class PresenterFactory : public QObject
    {
        Q_OBJECT
    public:
        PresenterFactory(domain::RoboModel* model, QObject* parent = nullptr);
        ~PresenterFactory();

    public slots:
        QObject* framePresenter();
        QObject* trackPresenter();
        QObject* settingsPresenter();
        QObject* statusPresenter();

    private:
        class Impl;
        Impl* d;
    };
}

#endif //PRESENTER_FACTORY_H
