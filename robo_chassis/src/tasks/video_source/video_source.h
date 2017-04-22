#ifndef VIDEO_SOURCE_H
#define VIDEO_SOURCE_H

#include "i_task.h"

struct ImageSettings;

class VideoSource : public ITask
{
public:
    VideoSource(int& argc, char** argv);
    ~VideoSource();

    void start() override;
    void execute() override;

private:
    void onImageSettingsChanged(const ImageSettings& settings);
    void onVideoSourceChanged(const QString& source);

private:
    class Impl;
    Impl* d;
};

#endif // VIDEO_SOURCE_H
