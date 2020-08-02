#pragma once
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include "Util.h"

class Camera {
public:

    float focalLength = 0.050f; // meter
    float focusDistance = 14.f; // meter
    float aperture = 1.4f; // ratio
    int dofSamples = 64;

    int apertureBlades = 5;
    float bokehSqueeze = 0.f;
    float bokehSqueezeFalloff = 1.f;

    float vignetteStrength = 0.2f;
    float vignetteFalloff = 3.0f;
    float aspectRatio = 1.f;
    float vignetteDesaturation = 0.f;

    float dispersionStrength = 0.f;
    float dispersionFalloff = 3.f;
    float barrelDistortion = 0.f;
    float barrelDistortionFalloff = 4.f;

    float sensorSize = 0.035f; // meter
    float sensorCrop = 0.f; // ratio
    float resolutionScale = 1.f; // ratio
    float nearPlane = 0.1f; // meter
    float farPlane = 50.f; // meter

    float grain = 0.0;
    float grainSize = 1.0;

    float exposure = 1.0;
    
    // Camera Attributes
    glm::vec3 position, up, right;
    glm::vec3 front = { 0.0f, 0.0f, -1.0f };
    glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };

    // Euler Angles
    float yaw = 0.f, pitch = 0.f;

    // Camera options
    float movementSpeed = 50.f;
    float mouseSensitivity = 0.2f;

    Camera(glm::vec3 pos = { 0.0f, 4.0f, 10.0f }, float y = -90.f, float p = 0.f) {
        position = pos;
        yaw = y;
        pitch = p;
        updateCameraVectors();
    }

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 getViewMatrix() const {
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
                // fieldOfView -= i.x;
                // fieldOfView = clamp(fieldOfView, 1.f, 180.f);
                i.handled = true;
            } else
            if (i.type == Event::MOUSE_MOVE) {
                yaw += i.x * mouseSensitivity;
                pitch += i.y * mouseSensitivity;
                pitch = clamp(pitch, -89.f, 89.f);
                updateCameraVectors();
                i.handled = true;
            }
        }
    }

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

    glm::mat4 getProjectionMatrix(float aspectRatio) const {
        return glm::perspective(
            float(2.0 * atan(sensorSize / (2.0 * focalLength))),
            aspectRatio, nearPlane, farPlane
        );
    }
};

inline Camera& getDefaultCam() {
    static Camera cam;
    return cam;
}

inline Camera& getTestCam(bool reset = false) {
    static Camera cam;
    if (reset) { cam = Camera(); }
    // cam.fieldOfView = 29.86f;
    cam.focusDistance = 6.f;
    cam.dispersionStrength = 0.f;
    //cam.farPlane = 9.f;
    cam.position = glm::vec3(0, 0, 6.f);
    return cam;
}

inline Camera& getTestCam2(bool reset = false) {
    static Camera cam;
    if (reset) { cam = Camera(); }
    // cam.fieldOfView = 29.86f;
    cam.focusDistance = 6.f;
    cam.dispersionStrength = 0.f;
    //cam.farPlane = 9.f;
    cam.position = glm::vec3(12.1f, 1.5f, -2.f);
    cam.yaw = 165.f;
    cam.pitch = 0.f;
    cam.updateCameraVectors();
    return cam;
}

inline Camera& getSkateboardCam() {
    static Camera cam;
    // cam.fieldOfView = 110.f;
    cam.sensorCrop = 0.48f;
    cam.resolutionScale = 1.4f;
    cam.dofSamples = 1;
    cam.dispersionStrength = 2.f;
    cam.barrelDistortion = -2.f;
    cam.barrelDistortionFalloff = 2.2f;
    return cam;
}