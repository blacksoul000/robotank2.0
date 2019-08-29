#ifndef COMMAND_SERVICE_H
#define COMMAND_SERVICE_H

// Qt
#include <QObject>

// Internal
#include "mavlink_traits.h"
#include "command.h"

namespace domain
{
    class AbstractCommandHandler;

    class CommandService: public QObject
    {
        Q_OBJECT

    public:
        explicit CommandService(QObject* parent = nullptr);

    public slots:
        void addHandler(AbstractCommandHandler* handler);
        void removeHandler(AbstractCommandHandler* handler);

    signals:
        void executeCommand(int sysId, const CommandPtr& command,
        					bool singleshot = false);
        void cancelCommand(int sysId, int type);

        void commandChanged(int sysId, CommandPtr command);
    };
}

#endif // COMMAND_SERVICE_H
