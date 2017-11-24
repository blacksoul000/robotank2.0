#ifndef GUI_EXCHANGER_H
#define GUI_EXCHANGER_H

#include "i_task.h"

struct PointF3D;
struct ImageSettings;

class GuiExchanger : public ITask
{
public:
    explicit GuiExchanger();
    ~GuiExchanger();

    void start() override;
    void execute() override;

private:
    void onReadyRead();

    void onGunPosition(const QPointF& position);
    void onCameraPosition(const QPointF& position);
    void onYpr(const PointF3D& ypr);
    void onArduinoStatus(const bool& status);
    void onJoyButtons(const quint16& buttons);
    void onJoyStatus(const bool& status);
    void onTrackerTarget(const QRectF& target);
    void onTrackerStatus(const bool& status);
    void onEnginePowerChanged(const QPoint& enginePower);
    void onImageSettingsChanged(const ImageSettings& settings);
    void onSwitchTrackerRequest(const quint8& code);
    void onVideoSourceChanged(const QString& source);
    void onJoyCapacity(const quint8& capacity);
    void onJoyCharging(const bool& charging);

private:
    class Impl;
    Impl* d;
};

#endif // GUI_EXCHANGER_H
