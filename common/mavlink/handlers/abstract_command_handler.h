#ifndef ABSTRACT_COMMAND_HANDLER_H
#define ABSTRACT_COMMAND_HANDLER_H

#include <QObject>

// Internal
#include "mavlink_traits.h"
#include "command.h"

namespace domain
{
    class AbstractCommandHandler: public QObject
    {
        Q_OBJECT

    public:
        explicit AbstractCommandHandler(QObject* parent = nullptr);
        ~AbstractCommandHandler() override;

    public slots:
        void executeCommand(int vehicleId, const CommandPtr& command,
        					bool singleshot = false);
        void cancelCommand(int vehicleId, int type);
        void onCommandChanged(int vehicleId, const CommandPtr& command);

    signals:
        void commandChanged(int vehicleId, CommandPtr command);

    protected:
        void ackCommand(int vehicleId, int commandId, Command::CommandStatus status);
        void stopCommand(int vehicleId, const CommandPtr& command);
        void timerEvent(QTimerEvent* event) override;

        virtual void sendCommand(int vehicleId, const CommandPtr& command, int attempt = 0) = 0;

    private:
        class Impl;
        QScopedPointer<Impl> const d;
    };
}

#endif // ABSTRACT_COMMAND_HANDLER_H
