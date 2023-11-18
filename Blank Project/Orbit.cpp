#define _USE_MATH_DEFINES

#include "Orbit.h"
#include <math.h>

const float degToRadians = M_PI / 180.0f;

Orbit::Orbit(float angle, Vector3 rotCenter, Vector3 position, float rotationAmount) {
    this->angleDegrees = angle;
    this->rotCenter = rotCenter;
    this->position = position;
    this->rotationAmount = rotationAmount;

    radius = hypot(hypot(position.x - rotCenter.x, position.y - rotCenter.y), position.z - rotCenter.z);
}

Vector3 Orbit::CalculateRelativePosition() {
    angleDegrees += rotationAmount;

    if (angleDegrees >= 360.0f)
    {
        angleDegrees -= 360.0f;
    }

    float angleRadians = angleDegrees * degToRadians;

    float x = rotCenter.x + (radius * cosf(angleRadians));
    float y = rotCenter.y + (radius * sinf(angleRadians));

    position = Vector3(x, y, position.z);

    return position - rotCenter;    // subtract centre position as orbiting object will be a child of centre node, so its local transform shouldnt include rotCenter
}