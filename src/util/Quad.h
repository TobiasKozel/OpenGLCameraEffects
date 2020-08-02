#pragma once
#include "glad/glad.h"

/**
 * Draw a Quad, nothing more
 */
class Quad {
    GLuint quadVAO, quadVBO;
public:
    NO_COPY(Quad)

    Quad() {
        // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        GLC(glGenVertexArrays(1, &quadVAO));
        GLC(glGenBuffers(1, &quadVBO));
        GLC(glBindVertexArray(quadVAO));
        GLC(glBindBuffer(GL_ARRAY_BUFFER, quadVBO));
        GLC(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW));
        GLC(glEnableVertexAttribArray(0));
        GLC(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr));
        GLC(glEnableVertexAttribArray(1));
        GLC(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float))));
    }

    ~Quad() {
        GLC(glDeleteVertexArrays(1, &quadVAO));
        GLC(glDeleteBuffers(1, &quadVBO));
    }

    void draw() const {
        GLC(glBindVertexArray(quadVAO));
        GLC(glDrawArrays(GL_TRIANGLES, 0, 6));
        GLC(glBindVertexArray(0));
    }
};