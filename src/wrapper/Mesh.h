#pragma once
#include "glad/glad.h"

#include <glm.hpp>

#include "Shader.h"

#include <string>
#include <vector>

/**
 * Slightly alter code from learnopengl
 */
class Mesh {
public:
    struct Material {
        std::string name;
        glm::vec4 color;
        std::shared_ptr<Texture> colorTexture;
    };
    
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
    };

    Material material;

    
    unsigned int VAO;

    NO_COPY(Mesh)
    Mesh(std::vector<Vertex> &_vertices, std::vector<unsigned int> &_indices, Material &_material) {
        material = _material;
        indiceCount = _indices.size();
        // create buffers/arrays
        GLC(glGenVertexArrays(1, &VAO));
        GLC(glGenBuffers(1, &VBO));
        GLC(glGenBuffers(1, &EBO));

        GLC(glBindVertexArray(VAO));
        // load data into vertex buffers
        GLC(glBindBuffer(GL_ARRAY_BUFFER, VBO));
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        GLC(glBufferData(
            GL_ARRAY_BUFFER, _vertices.size() * sizeof(Vertex),
            &_vertices[0], GL_STATIC_DRAW
        ));

        GLC(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO));
        GLC(glBufferData(
            GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(unsigned int),
            &_indices[0], GL_STATIC_DRAW
        ));

        // set the vertex attribute pointers
        // vertex Positions
        GLC(glEnableVertexAttribArray(0));
        GLC(glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr
        ));
        // vertex normals
        GLC(glEnableVertexAttribArray(1));
        GLC(glVertexAttribPointer(
            1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, Normal))
        ));
        // vertex texture coords
        GLC(glEnableVertexAttribArray(2));
        GLC(glVertexAttribPointer(
            2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, TexCoords))
        ));
        GLC(glBindVertexArray(0));
    }

    ~Mesh() {
        GLC(glDeleteBuffers(1, &EBO));
        GLC(glDeleteBuffers(1, &VBO));
        GLC(glDeleteVertexArrays(1, &VAO));
    }

    void Draw(Shader &shader)  {
        if (material.colorTexture != nullptr) {
            const int textureSlot = 0;
            GLC(glActiveTexture(GL_TEXTURE0 + textureSlot));
            shader.setInt("texture_diffuse", textureSlot);
            shader.setBool("use_color", false);
            material.colorTexture->use();
        } else {
            shader.setBool("use_color", true);
            shader.setVec4("diffuse_color", material.color);
        }
        
        // draw mesh
        GLC(glBindVertexArray(VAO));
        GLC(glDrawElements(GL_TRIANGLES, indiceCount, GL_UNSIGNED_INT, 0));
        GLC(glBindVertexArray(0));

        // always good practice to set everything back to defaults once configured.
        GLC(glActiveTexture(GL_TEXTURE0));
    }

private:
    GLuint VBO, EBO;
    GLuint indiceCount = 0;
};
