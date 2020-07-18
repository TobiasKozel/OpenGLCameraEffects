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
public:
	enum AttachmentType {
		COLOR = 0, // RGBA Color
		COLORSOFT, // RGBA Color
		DEPTH, // Will add a depth rbo, needed for ZBUFFER and anything with ztesting
		GREY, // Simple 8Bit greyscale map
		GREYSOFT, // Simple 8Bit greyscale map linear interpolation
		ZBUFFER, // 32Bit Float depth buffer. The zbuffer will be blitted in the automatically
		ZBUFFERLINEAR, // 32 Bit Float depth buffer, will not be filled 
		NORMAL, // 16Bit Float with 3 Channels
		POSITION, // 16Bit Float with 3 Channels
		ATTACHMENTCOUNT
	};

	typedef std::map<AttachmentType, TextureConfig> AttachmentMap;

private:
	int width = 0, height = 0;
	int scaledWidth = 0, scaledHeight = 0;
	float scale = 1.0;
	GLuint fbId = 0, depthId = 0;

	GLuint depthTexture = 0;

	/**
	 * Textures a shader can write into
	 */
	Textures textures;

	/**
	 * Attachment types to create textures for
	 */
	std::vector<AttachmentType> attachments;
public:
	NO_COPY(FrameBufferObject)
	
	FrameBufferObject(std::vector<AttachmentType> attach = {}, int w = 0, int h = 0) {
		if (!attach.empty()) {
			attachments = attach;
			resize(w, h);
		}
	}

	~FrameBufferObject() {
		cleanUp();
	}

	/**
	 * Returns a list of attachments and how they configure a texture
	 */
	static AttachmentMap &getAttachmentMap() {
		static std::map<AttachmentType, TextureConfig> attachmentMap = {
			{ POSITION, { "gPosition", GL_RGB16F, GL_RGB, GL_FLOAT } },
			{ NORMAL, { "gNormal", GL_RGB16F, GL_RGB, GL_FLOAT } },
			{ COLOR, { "gColor", GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE } },
			{ COLORSOFT, { "gColorSoft", GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR } },
			{ GREY, { "gGrey", GL_RED, GL_RED, GL_UNSIGNED_BYTE } },
			{ GREYSOFT, { "gGreySoft", GL_RED, GL_RED, GL_UNSIGNED_BYTE, GL_LINEAR, GL_LINEAR } },
			{ ZBUFFER, { "zBuffer", GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT } },
			{ ZBUFFERLINEAR, { "zBufferLinear", GL_R16F, GL_RED, GL_FLOAT } }
		};
		return attachmentMap;
	}

	/**
	 * Allows registering a new type of texture to attach to the buffer
	 */
	static AttachmentType registerAttachmentType(TextureConfig c, AttachmentType type = ATTACHMENTCOUNT) {
		AttachmentMap& map = getAttachmentMap();
		if (type == ATTACHMENTCOUNT) {
			// Make sure to get a unique enum
			type = static_cast<AttachmentType>(map.size() + ATTACHMENTCOUNT);
		}
		map.insert({ type, c });
		return type;
	}

	void setAttachments(std::vector<AttachmentType> attach = {}) {
		cleanUp();
		attachments = attach;
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
		
		f();
		
		if (depthTexture != 0) { // If we have a depth texture, copy the current zbuffer over to a texture
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
		
		bool depthbuffer = false; // whether we'll need to add a depth rbo
		bool multiSample = false; // TODO Multisampling doesn't work

		int index = 0; // texture index
		for (auto& i : attachments) {
			if (i == DEPTH) { depthbuffer = true; } // depth buffer is an RBO not a texture
			
			auto c = getAttachmentMap().find(i);
			
			if (c == getAttachmentMap().end()) { continue; } // No attachment found to configure a texture
			
			const auto config = c->second;

			if (config.target == GL_TEXTURE_2D_MULTISAMPLE) { multiSample = true; }

			std::shared_ptr<Texture> tex(new Texture(scaledWidth, scaledHeight, config));
			
			if (i == ZBUFFER) {
				// The depth texture doesn't need to be registered to the fbo, since the shader doesn't write into it
				depthTexture = tex->getId();
			} else {
				// The other texture will be accessible from the fragment shader as outputs
				GLuint attachmentId = GL_COLOR_ATTACHMENT0 + index;
				GLC(glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentId, config.target, tex->getId(), 0));
				attach.push_back(attachmentId);
				index++;
			}
			textures.push_back(tex);
		}

		// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
		GLC(glDrawBuffers(attach.size(), attach.data()));

		if (depthbuffer) { // add the rbo if  needed
			GLC(glGenRenderbuffers(1, &depthId));
			GLC(glBindRenderbuffer(GL_RENDERBUFFER, depthId));
			if (multiSample) {
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
	}
};
