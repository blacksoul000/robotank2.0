#ifndef CONFIG_HANDLER_H
#define CONFIG_HANDLER_H

#include "i_task.h"

struct ImageSettings;

class ConfigHandler : public ITask
{
public:
    ConfigHandler();
    ~ConfigHandler();

    void execute() override;

private:
    void onEnginePowerChanged(const QPoint& enginePower);
    void onImageSettingsChanged(const ImageSettings& settings);
    void onSwitchTrackerRequest(const quint8& code);
    void onVideoSourceChanged(const QString& source);

private:
    class Impl;
    Impl* d;
};

#endif // CONFIG_HANDLER_H
