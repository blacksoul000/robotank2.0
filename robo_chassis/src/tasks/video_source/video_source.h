#ifndef VIDEO_SOURCE_H
#define VIDEO_SOURCE_H

#include "i_task.h"

struct ImageSettings;

class VideoSource : public ITask
{
public:
    VideoSource();
    ~VideoSource();

    void start() override;

    void onNewFrame(const void* data, int size);

private:
    void onImageSettingsChanged(const ImageSettings& settings);

private:
    class Impl;
    Impl* d;
};

#endif // VIDEO_SOURCE_H
