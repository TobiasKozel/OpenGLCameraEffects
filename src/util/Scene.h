#pragma once
#include "Camera.h"
#include "Event.h"

#include <glm.hpp>
#include <gtc/type_ptr.hpp>

class Scene {
protected:
    Camera& camera = getDefaultCam();

    float width = 0.f, height = 0.f;

    float time = 0.0;

public:
    Scene() { }
    
    virtual void draw() = 0;

    virtual void onResize(int w, int h) {
        width = w;
        height = h;
    }

    virtual void onEvent(Event &e) { }

    virtual void debugUi() {}

    void update(std::vector<Event> &events, float deltaTime) {
        time += deltaTime;
        camera.update(events, deltaTime);
        
        for (auto &i : events) {
            if (i.handled) { continue; }
            
            if (i.type == Event::RESIZE) {
                onResize(i.x, i.y);
            } else {
                onEvent(i);
            }
        }
        
    }
};