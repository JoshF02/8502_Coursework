#include "Camera.h"
#include "../nclgl/Window.h"
#include <algorithm>

void Camera::UpdateCamera(float dt)
{
	pitch -= (Window::GetMouse()->GetRelativePosition().y);
	yaw -= (Window::GetMouse()->GetRelativePosition().x);

	pitch = std::min(pitch, 90.0f);
	pitch = std::max(pitch, -90.0f);

	if (yaw < 0) {
		yaw += 360.0f;
	}
	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}

	Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 1, 0));

	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right = rotation * Vector3(1, 0, 0);

	float timeSpeed = speed * dt;

	if (automated) {
		if (zneg) {
			position += Vector3(0, 0, -1) * timeSpeed;
			if (position.z < halfHMSize) {
				position.z += 10.0f;
				xneg = true;
				zneg = false;
			}
		}
		if (zpos) {
			position += Vector3(0, 0, 1) * timeSpeed;
			if (position.z > heightmapSize.z + halfHMSize) {
				position.z -= 10.0f;
				xpos = true;
				zpos = false;
			}
		}
		if (xneg) {
			position += Vector3(-1, 0, 0) * timeSpeed;
			if (position.x < halfHMSize) {
				position.x += 10.0f;
				zpos = true;
				xneg = false;
			}
		}
		if (xpos) {
			position += Vector3(1, 0, 0) * timeSpeed;
			if (position.x > heightmapSize.x + halfHMSize) {
				position.x -= 10.0f;
				zneg = true;
				xpos = false;
			}
		}
		yaw += 0.1;
	}
	else {
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_W)) {
			position += forward * timeSpeed;
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_S)) {
			position -= forward * timeSpeed;
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_A)) {
			position -= right * timeSpeed;
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_D)) {
			position += right * timeSpeed;
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_SHIFT)) {
			position.y += timeSpeed;
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_SPACE)) {
			position.y -= timeSpeed;
		}
	}

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_C)) {
		TriggerAuto();
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_V)) {
		automated = false;
	}
}

Matrix4 Camera::BuildViewMatrix()
{
	return Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) * Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) * Matrix4::Translation(-position);
}
