#ifndef INFLUENCE_H
#define INFLUENCE_H

#include <cstdint>

struct Influence
{
    uint8_t angleType;
    uint8_t shot;
    int16_t gunV;
    int16_t cameraV;
    int16_t leftEngine;
    int16_t rightEngine;
    int16_t towerH;
};

#endif // INFLUENCE_H
