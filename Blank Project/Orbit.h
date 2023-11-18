#pragma once
#include "../nclgl/Vector3.h"

class Orbit {
public:
	Orbit(float angle = 0.0f, Vector3 rotCenter = Vector3(0,0,0), Vector3 position = Vector3(0,1,0), float rotationAmount = 1.0f);

	Vector3 CalculateRelativePosition(void);

protected:
	float angleDegrees;
	Vector3 rotCenter;
	Vector3 position;
	double radius;
	float rotationAmount;
};