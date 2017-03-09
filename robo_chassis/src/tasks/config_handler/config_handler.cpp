#include "config_handler.h"

//msgs
#include "image_settings.h"

#include "pub_sub.h"

#include <QCoreApplication>
#include <QSettings>
#include <QPoint>

namespace
{
    const QString settingsFileName = "robo_chassis.cfg";

    const QString qualityId = "quality";
    const QString brightnessId = "brightness";
    const QString contrastId = "contrast";
    const QString leftEngineId = "leftEngine";
    const QString rightEngineId = "rightEngine";
    const QString trackerId = "tracker";

    const int defaultQualityValue = 40;
    const int defaultBrightnessValue = 65;
    const int defaultContrastValue = 75;
    const int defaultLeftEngineValue = 100;
    const int defaultRightEngineValue = 100;
    const int defaultTrackerValue = 2; // OpenTLD
}

class ConfigHandler::Impl
{
public:
    Publisher< ImageSettings >* imageSettingsP = nullptr;
    Publisher< QPoint >* enginePowerP = nullptr;
    Publisher< quint8 >* trackSelectorP = nullptr;

    QSettings* settings = nullptr;

    void loadConfig();
};

ConfigHandler::ConfigHandler() :
    ITask(),
    d(new Impl)
{
    qRegisterMetaType< quint8 >("quint8");

    d->imageSettingsP = PubSub::instance()->advertise< ImageSettings >("camera/settings");
    d->enginePowerP = PubSub::instance()->advertise< QPoint >("core/enginePower");
    d->trackSelectorP = PubSub::instance()->advertise< quint8 >("tracker/selector");

    d->settings = new QSettings(QCoreApplication::applicationDirPath() + "/" + ::settingsFileName,
                                QSettings::NativeFormat, this);
    d->loadConfig(); // before subscribe

    PubSub::instance()->subscribe("core/enginePower", &ConfigHandler::onEnginePowerChanged, this);
    PubSub::instance()->subscribe("camera/settings", &ConfigHandler::onImageSettingsChanged, this);
    PubSub::instance()->subscribe("tracker/selector", &ConfigHandler::onSwitchTrackerRequest, this);
}

ConfigHandler::~ConfigHandler()
{
    d->settings->sync();
    delete d;
}

void ConfigHandler::execute()
{}

void ConfigHandler::onImageSettingsChanged(const ImageSettings& settings)
{
    d->settings->setValue(::qualityId, settings.quality);
    d->settings->setValue(::brightnessId, settings.brightness);
    d->settings->setValue(::contrastId, settings.contrast);
}

void ConfigHandler::onEnginePowerChanged(const QPoint& enginePower)
{
    d->settings->setValue(::leftEngineId, enginePower.x());
    d->settings->setValue(::rightEngineId, enginePower.y());
}

void ConfigHandler::onSwitchTrackerRequest(const quint8& code)
{
    d->settings->setValue(::trackerId, code);
}

//------------------------------------------------------------------------------------
void ConfigHandler::Impl::loadConfig()
{
    ImageSettings s{settings->value(::qualityId, ::defaultQualityValue).value< quint8 >(),
                    settings->value(::brightnessId, ::defaultBrightnessValue).value< quint8 >(),
                    settings->value(::contrastId, ::defaultContrastValue).value< quint8 >()};
    imageSettingsP->publish(s);

    QPoint engine(settings->value(::leftEngineId, ::defaultLeftEngineValue).value< quint8 >(),
                  settings->value(::rightEngineId, ::defaultRightEngineValue).value< quint8 >());
    enginePowerP->publish(engine);

    quint8 tracker(settings->value(::trackerId, ::defaultTrackerValue).value< quint8 >());
    trackSelectorP->publish(tracker);
}
