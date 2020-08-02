#pragma once
#include "glad/glad.h"
#include <cassert>
#include <vector>
#include <memory>
#include "../util/Util.h"
#include "Texture.h"
#include <map>
#include <functional>

class FrameBufferObject {
    struct Attachment {
        TextureConfig tex;
        int w = 0;
        int h = 0;
    };
public:
    /**
     * Class which is passed to the configuration callback
     * to setup the textures
     */
    struct FrameBufferConfig {
        int w = 0, h = 0;
        std::vector<Attachment> attachments;
        bool depth = false;
        bool multisample = false;
        bool zBuffer = false;
        void addCustom(const TextureConfig& c, int _w = 0, int _h = 0 ) {
            if (!_w) { _w = w; }
            if (!_h) { _h = h; }
            attachments.push_back({ c , _w, _h });
        }
        void addRGB16F(std::string name, int _w = 0, int _h = 0, GLuint filter = GL_NEAREST) {
            addCustom({ name, GL_RGB16F, GL_RGB, GL_FLOAT, filter, filter }, _w, _h);
        }
        void addRGBA8(std::string name, int _w = 0, int _h = 0, GLuint filter = GL_NEAREST) {
            addCustom({ name, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, filter, filter }, _w, _h);
        }
        void addRGB8(std::string name, int _w = 0, int _h = 0, GLuint filter = GL_NEAREST) {
            addCustom({ name, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, filter, filter }, _w, _h);
        }
        void addR8(std::string name, int _w = 0, int _h = 0, GLuint filter = GL_NEAREST) {
            addCustom({ name, GL_RED, GL_RED, GL_UNSIGNED_BYTE, filter, filter }, _w, _h);
        }
        void addR16F(std::string name, int _w = 0, int _h = 0, GLuint filter = GL_NEAREST) {
            addCustom({ name, GL_R16F, GL_RED, GL_FLOAT, filter, filter }, _w, _h);
        }
        void addZBuffer() {
            depth = true;
            if (zBuffer) { return; }
            zBuffer = true;
            addCustom({ "zBuffer", GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT });
        }
    };

    typedef std::function<void (FrameBufferConfig&)> ConfigurationFunction;
private:
    const ConfigurationFunction configuration;
    int width = 0, height = 0;
    int scaledWidth = 0, scaledHeight = 0;
    float scale = 1.0;
    GLuint fbId = 0, depthId = 0;

    GLuint depthTexture = 0;

    /**
     * Textures a shader can write into
     */
    Textures textures;

public:
    NO_COPY(FrameBufferObject)

    FrameBufferObject(const ConfigurationFunction& c) : configuration(c) { }
    
    ~FrameBufferObject() {
        cleanUp();
    }

    /**
     * Will bind/unbind the FBO and render call the provided function at the right time
     */
    void draw(const std::function<void()> &f) const {
        GLC(glBindFramebuffer(GL_FRAMEBUFFER, fbId));
        
        if (depthId != 0) {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
        } else {
            glClear(GL_COLOR_BUFFER_BIT);
        }

        if (scaledWidth != width) { // scale the viewport down if needed
            GLC(glViewport(0, 0, scaledWidth, scaledHeight));
        }
        
        f(); // Render
        
        if (depthTexture != 0) {
            // If we have a depth texture, copy the current zbuffer over to a texture
            GLC(glBindTexture(GL_TEXTURE_2D, depthTexture));
            GLC(glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, scaledWidth, scaledHeight, 0));
            GLC(glBindTexture(GL_TEXTURE_2D, 0));
        }
        
        GLC(glBindFramebuffer(GL_FRAMEBUFFER, 0));

        if (scaledWidth != width) { // scale it back up again
            GLC(glViewport(0, 0, width, height));
        }
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    /**
     * This contains the main FBO setup
     */
    void resize(int w, int h, float s = 1.0) {
        if (w == width && h == height && w <= 0 && h <= 0) { return; }
        cleanUp();

        scale = s;
        width = w;
        height = h;
        scaledWidth = width * s;
        scaledHeight = height * s;
        
        GLC(glGenFramebuffers(1, &fbId));
        GLC(glBindFramebuffer(GL_FRAMEBUFFER, fbId));

        std::vector<GLuint> attach;

        FrameBufferConfig c = { scaledWidth, scaledHeight };
        configuration(c);
        
        int index = 0; // texture index
        for (auto& i : c.attachments) {
            std::shared_ptr<Texture> tex(new Texture(i.w, i.h, i.tex));
            
            if (i.tex.format == GL_DEPTH_COMPONENT) {
                // The depth texture doesn't need to be registered to the fbo, since the shader doesn't write into it
                depthTexture = tex->getId();
            } else {
                // The other texture will be accessible from the fragment shader as outputs
                GLuint attachmentId = GL_COLOR_ATTACHMENT0 + index;
                GLC(glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentId, i.tex.target, tex->getId(), 0));
                attach.push_back(attachmentId);
                index++;
            }
            textures.push_back(tex);
        }

        // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
        GLC(glDrawBuffers(attach.size(), attach.data()));

        if (c.depth) { // add the rbo if  needed
            GLC(glGenRenderbuffers(1, &depthId));
            GLC(glBindRenderbuffer(GL_RENDERBUFFER, depthId));
            if (c.multisample) {
                // TODO This might be wrong
                GLC(glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, scaledWidth, scaledHeight));
            } else {
                GLC(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, scaledWidth, scaledHeight));
            }
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            GLC(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthId));
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Textures getTextures(Textures tex) {
        tex.insert(tex.end(), textures.begin(), textures.end());
        return tex;
    }

    Textures getTexture(const int index) {
        return { textures[index] };
    }

    const Textures &getTextures() const {
        return textures;
    }

private:
    void cleanUp() {
        textures.clear();
        if (fbId != 0) {
            GLC(glDeleteFramebuffers(1, &fbId));
            fbId = 0;
        }

        if (depthId != 0) {
            GLC(glDeleteRenderbuffers(1, &depthId));
            depthId = 0;
        }
        depthTexture = 0;
    }
};
