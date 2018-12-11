#ifndef COMMAND_H
#define COMMAND_H

// Qt
#include <QVariant>

namespace domain
{
    class Command
    {
        Q_GADGET

        Q_PROPERTY(int commandId READ commandId WRITE setCommandId)
        Q_PROPERTY(CommandStatus status READ status WRITE setStatus)
        Q_PROPERTY(QVariantList arguments READ arguments WRITE setArguments)

    public:
        enum CommandStatus
        {
            Idle,
            Rejected,
            Canceled,
            Sending,
            InProgress,
            Completed,
        };

        int type() const;
        void setType(int type);

        int commandId() const;
        void setCommandId(int commandId);

        CommandStatus status() const;
        void setStatus(CommandStatus status);

        QVariantList arguments() const;
        void setArguments(const QVariantList& arguments);
        void addArgument(const QVariant& argument);

        bool isFinished() const;

    private:
        int m_type = 0;
        int m_commandId = 0;
        CommandStatus m_status = Idle;
        QVariantList m_arguments;

        Q_ENUM(CommandStatus)
    };
}

#endif // COMMAND_H
