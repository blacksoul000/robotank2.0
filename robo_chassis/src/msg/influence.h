#ifndef INFLUENCE_H
#define INFLUENCE_H

#include <cstdint>

enum AngleType
{
    Velocity = 0,
    Position = 1,
};

struct Influence
{
    AngleType angleType = AngleType::Velocity;
    uint8_t shot = 0;
    int16_t gunV = 0;
    int16_t cameraV = 0;
    int16_t leftEngine = 0;
    int16_t rightEngine = 0;
    int16_t towerH = 0;
};

#endif // INFLUENCE_H
