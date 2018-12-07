#include "abstract_command_handler.h"

// Qt
#include <QMap>
#include <QTimerEvent>
#include <QDebug>

namespace
{
    const int interval = 700;
    const int maxAttemps = 5;
}

using namespace domain;

class AbstractCommandHandler::Impl
{
public:
    QMultiMap< int, CommandPtr > vehicleCommands;
    QMap< CommandPtr, int > attemps;
    QMap< CommandPtr, int > commandTimers;
};

AbstractCommandHandler::AbstractCommandHandler(QObject* parent) :
    QObject(parent),
    d(new Impl())
{
}

AbstractCommandHandler::~AbstractCommandHandler()
{
}

void AbstractCommandHandler::executeCommand(int vehicleId, const CommandPtr& command)
{
    for (const CommandPtr& timedCommand : d->commandTimers.keys())
    {
        if (timedCommand->type() != command->type()
                || !d->vehicleCommands.values(vehicleId).contains(timedCommand)) continue;

        this->stopCommand(vehicleId, timedCommand);

        timedCommand->setStatus(Command::Canceled);
        emit commandChanged(vehicleId, timedCommand);

        break;
    }

    d->vehicleCommands.insert(vehicleId, command);
    d->attemps[command] = 0;
    d->commandTimers[command] = this->startTimer(::interval);
    command->setStatus(Command::Sending);

    this->sendCommand(vehicleId, command);

    emit commandChanged(vehicleId, command);
}

void AbstractCommandHandler::cancelCommand(int vehicleId, int type)
{
    this->ackCommand(vehicleId, type, Command::Canceled);
}

void AbstractCommandHandler::ackCommand(int vehicleId, int type,
        Command::CommandStatus status)
{
    for (const CommandPtr& command : d->vehicleCommands.values(vehicleId))
    {
        if (command->type() != type) continue;
        command->setStatus(status);
        emit commandChanged(vehicleId, command);
    }
}

void AbstractCommandHandler::stopCommand(int vehicleId, const CommandPtr& command)
{
    d->vehicleCommands.remove(vehicleId, command);
    d->attemps.remove(command);
    if (d->commandTimers.contains(command))
    {
        this->killTimer(d->commandTimers.take(command));
    }
}

void AbstractCommandHandler::timerEvent(QTimerEvent* event)
{
    CommandPtr command = d->commandTimers.key(event->timerId());
    if (command.isNull()) return QObject::timerEvent(event);

    int vehicleId = d->vehicleCommands.key(command, 0);
    this->sendCommand(vehicleId, command, ++d->attemps[command]);

    if (d->attemps[command] < ::maxAttemps) return;

    this->stopCommand(vehicleId, command);

    command->setStatus(Command::Rejected);
    emit commandChanged(vehicleId, command);
}

void AbstractCommandHandler::onCommandChanged(int vehicleId, const CommandPtr& command)
{
    if (command->isFinished()) this->stopCommand(vehicleId, command);
}
