#pragma once
#include "glad/glad.h"
#include "../util/Util.h"
#include <memory>

/**
 * Config object which can be used to set up textures
 */
struct TextureConfig {
    std::string name = ""; // The name used to set the texture uniform for the shader, empty won't bind
    GLuint internalFormat = GL_RGB;
    GLint format = GL_RGB;
    GLuint type = GL_UNSIGNED_BYTE;
    GLuint minFilter = GL_NEAREST;
    GLuint maxFilter = GL_NEAREST;
    GLuint wrapS = GL_CLAMP_TO_EDGE;
    GLuint wrapT = GL_CLAMP_TO_EDGE;
    GLuint target = GL_TEXTURE_2D;
    const void* pixels = nullptr;
};

class Texture {
    GLuint texId = 0;
    TextureConfig config;
    std::string name;
    int width = 0, height = 0;
public:
    NO_COPY(Texture)

    Texture(int w, int h, const TextureConfig c, std::string texname = "") {
        config = c;
        GLC(glGenTextures(1, &texId));
        resize(w, h);

        if (texname.empty()) {
            name = c.name;
        } else {
            name = texname;
        }
    }

    std::string &getName() {
        return name;
    }

    void use() const {
        GLC(glBindTexture(config.target, texId));
    }

    void resize(int w, int h) {
        if (texId == 0 || w == width && h == height) { return; }
        width = w;
        height = h;
        GLC(glBindTexture(config.target, texId));
        if (config.target == GL_TEXTURE_2D_MULTISAMPLE) {
            GLC(glTexImage2DMultisample(config.target, 4, config.internalFormat, w, h, GL_TRUE));
        } else {
            GLC(glTexImage2D(config.target, 0, config.internalFormat, w, h, 0, config.format, config.type, config.pixels));
        }
        
        GLC(glTexParameteri(config.target, GL_TEXTURE_MIN_FILTER, config.minFilter));
        GLC(glTexParameteri(config.target, GL_TEXTURE_MAG_FILTER, config.maxFilter));
        GLC(glTexParameteri(config.target, GL_TEXTURE_WRAP_S, config.wrapS));
        GLC(glTexParameteri(config.target, GL_TEXTURE_WRAP_T, config.wrapT));
        glBindTexture(config.target, 0);
    }

    ~Texture() {
        if (texId != 0) {
            GLC(glDeleteTextures(1, &texId));
        }
    }

    GLuint getId() const { return texId; }
};

typedef std::vector<std::shared_ptr<Texture>> Textures;