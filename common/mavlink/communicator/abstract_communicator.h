#ifndef ABSTRACT_COMMUNICATOR_H
#define ABSTRACT_COMMUNICATOR_H

#include <QObject>

namespace data_source
{
    class AbstractLink;
    class Endpoint;
}  // namespace data_source

namespace domain
{
    class AbstractCommunicator: public QObject
    {
        Q_OBJECT

    public:
        AbstractCommunicator(QObject* parent);
        ~AbstractCommunicator();

        QList< data_source::AbstractLink* > links() const;

        virtual bool isAddLinkEnabled() = 0;

    public slots:
        virtual void addLink(data_source::AbstractLink* link);
        virtual void removeLink(data_source::AbstractLink* link);

    signals:
        void addLinkEnabledChanged(bool addLinkEnabled);
        void linkAdded(data_source::AbstractLink* link);
        void linkRemoved(data_source::AbstractLink* link);

    protected slots:
        virtual void onDataReceived(const QByteArray& data, const data_source::Endpoint& sender) = 0;

    private:
        QList< data_source::AbstractLink* > m_links;
    };
}

#endif // ABSTRACT_COMMUNICATOR_H
