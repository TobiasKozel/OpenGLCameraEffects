#pragma once
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include "Util.h"

class Camera {
public:
    float fieldOfView = 45; // Fov in degrees
    float focusDistance = 14.0; // focus distance in linear space
	float focusScale = 1.0; // Strength of the depth of field
    int dofSamples = 32; //  samples per pixel

    float vignetteStrength = 0.95;
    float vignetteFalloff = 3.0;
	float aspectRatio = 1.0;
    float vignetteDesaturation = 0.0;

    float dispersionStrength = 0.95;
	float dispersionFalloff = 3.0;
    float barrelDistortion = 0.0;
    float barrelDistortionFalloff = 1.0;

    float sensorCrop = 0.0;
	float resolutionScale = 1.0;
    float nearPlane = 0.1;
	float farPlane = 50.0;
	
    // Camera Attributes
    glm::vec3 position, up, right;
    glm::vec3 front = { 0.0f, 0.0f, -1.0f };
    glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };
	
    // Euler Angles
    float yaw, pitch;
	
    // Camera options
    float movementSpeed = 50.0;
    float mouseSensitivity = 0.2;

    Camera(glm::vec3 pos = { 0.0f, 4.0f, 10.0f }, float y = -90, float p = 0) {
        position = pos;
        yaw = y;
        pitch = p;
        updateCameraVectors();
    }

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

	void update(std::vector<Event> &events, float deltaTime) {
        const float velocity = movementSpeed * deltaTime;
	    for (auto &i : events) {
            if (i.handled) { continue; }
	    	
            if (i.type == Event::FORWARD) {
                position += front * velocity;
                i.handled = true;
            } else
            if (i.type == Event::BACKWARD) {
                position -= front * velocity;
                i.handled = true;
            } else
            if (i.type == Event::LEFT) {
                position -= right * velocity;
                i.handled = true;
            } else
            if (i.type == Event::RIGHT) {
                position += right * velocity;
                i.handled = true;
            } else
	    	if (i.type == Event::ZOOM) {
                fieldOfView -= i.x;
                fieldOfView = clamp(fieldOfView, 1, 180);
                i.handled = true;
	    	} else
	    	if (i.type == Event::MOUSE_MOVE) {
                yaw += i.x * mouseSensitivity;
                pitch += i.y * mouseSensitivity;
                pitch = clamp(pitch, -89, 89);
                updateCameraVectors();
                i.handled = true;
	    	}
	    }
    }

private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors() {
        // Calculate the new Front vector
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);
        // Also re-calculate the Right and Up vector
        // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        right = glm::normalize(glm::cross(front, worldUp));
        up    = glm::normalize(glm::cross(right, front));
    }
};

inline Camera& getDefaultCam() {
    static Camera cam;
    return cam;
}

inline Camera& getSkateboardCam() {
    static Camera cam;
    cam.fieldOfView = 110;
    cam.sensorCrop = 0.48;
    cam.resolutionScale = 1.4;
    cam.focusScale = 0;
    cam.dofSamples = 1;
    cam.dispersionStrength = 2;
    cam.barrelDistortion = -2;
    cam.barrelDistortionFalloff = 2.2;
    return cam;
}