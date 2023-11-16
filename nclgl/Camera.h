#pragma once
#include "../nclgl/Vector3.h"
#include "../nclgl/Matrix4.h"

class Camera
{
public:
	Camera(void) {
		yaw = 0.0f;
		pitch = 0.0f;
		speed = 2000.0f;
		position = heightmapSize * Vector3(1.0f, 2.0f, 1.0f);
		automated = false;
		zneg = false;
		zpos = false;
		xneg = false;
		xpos = false;
	};

	Camera(float pitch, float yaw, Vector3 position, Vector3 heightmapSize = Vector3(0,0,0)) {
		this->pitch = pitch;
		this->yaw = yaw;
		this->position = position;
		this->heightmapSize = heightmapSize;
		speed = 2000.0f;
		automated = false;
		zneg = false;
		zpos = false;
		xneg = false;
		xpos = false;
	}

	~Camera(void) {};

	void UpdateCamera(float dt = 1.0f);

	Matrix4 BuildViewMatrix();

	Vector3 GetPosition() const { return position; }
	void SetPosition(Vector3 val) { position = val; }

	float GetYaw() const { return yaw; }
	void SetYaw(float y) { yaw = y; }

	float GetPitch() const { return pitch; }
	void SetPitch(float p) { pitch = p; }

	void SetSpeed(float s) { speed = s; }

protected:
	float yaw;
	float pitch;
	float speed;
	Vector3 position;
	Vector3 heightmapSize;
	bool automated;
	bool zneg;
	bool zpos;
	bool xneg;
	bool xpos;
};

