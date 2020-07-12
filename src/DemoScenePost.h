#pragma once

#include <gtc/matrix_transform.hpp>

#include "util/Scene.h"
#include "wrapper/Shader.h"
#include "util/Event.h"
#include "wrapper/FrameBufferObject.h"
#include "util/Quad.h"
#include "wrapper/Model.h"

#include "shaders/GBufferShader.h"
#include "shaders/DOFShaderSimple.h"
#include "shaders/DOFShaderAdvanced.h"
#include "shaders/DOFShaderTest.h"
#include "shaders/SSAOShader.h"
#include "shaders/DeferredShader.h"
#include "shaders/PostShader.h"
#include "shaders/DebugShader.h"


class DemoScenePost : public Scene {
	
	Shader &gShader = getGBufferShader();
	//Model model = { platformPath("assets/littlest_tokyo/scene.obj") };
	Model model = { platformPath("assets/test/test.obj") };
	Quad billboard;
	Shader& dofSimpleShader = getDofShaderSimple();
	Shader& dofAdvancedShader = getDOFShaderAdvanced();
	Shader& dofTestShader = getDofShaderTest();
	Shader& ssaoShader = getSsaoShader();
	Shader& deferredShader = getDeferredShader();
	Shader& postShader = getPostShader();

	Shader* currentDofShader = &dofTestShader;

	FrameBufferObject gFbo = { {
		FrameBufferObject::POSITION,
		FrameBufferObject::NORMAL, FrameBufferObject::COLOR,
		FrameBufferObject::DEPTH,
	} };

	// Holds the unblurred ssao value
	FrameBufferObject ssaoFbo = { { FrameBufferObject::GREYSOFT } };

	// Shading and ao is applied here, the zbuffer is also scaled into linear space
	FrameBufferObject deferredFbo = { {
		FrameBufferObject::COLOR,
		FrameBufferObject::ZBUFFERLINEAR
	} };

	FrameBufferObject dofFbo = { { FrameBufferObject::COLORSOFT } };

	
	float ssaoScale = 0.5f, ssaoStrength = 1.f, ssaoRadius = 0.3f, ssaoBias = 0.025f;
	int ssaoSamples = 32, ssaoBlur = 1;

	std::shared_ptr<Texture> debugFbo = nullptr;
	bool debugRed = false;
	float debugScale = 1.f;
public:
	DemoScenePost(int w, int h) {
		camera = getTestCam();
		DemoScenePost::onResize(w, h);
	}

	void draw() override {
		glm::mat4 projection = glm::perspective(
			glm::radians(camera.fieldOfView),
			width / height, // Won't use the camera aspect here, since it's only used for post
			camera.nearPlane, camera.farPlane
		);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 modelMatrix = glm::mat4(1.0f);

		// GBuffer pass
		gFbo.draw([&]() {
			gShader.use();
			gShader.setMat4("model", modelMatrix);
			gShader.setMat4("projection", projection);
			gShader.setMat4("view", view);
			model.draw(gShader);
		});

		// SSAO pass at half res
		ssaoFbo.draw([&]() {
			ssaoShader.use(gFbo.getTextures());
			ssaoShader.setFloat("strength", ssaoStrength);
			ssaoShader.setFloat("radius", ssaoRadius);
			ssaoShader.setFloat("bias", ssaoBias);
			ssaoShader.setInt("count", ssaoSamples);
			ssaoShader.setMat4("projection", projection);
			billboard.draw();
		});

		/**
		 * Deferred shading and applying+filtering SSAO
		 * and converting the depth buffer in linear space
		 */
		deferredFbo.draw([&]() {
			deferredShader.use(ssaoFbo.getTextures(gFbo.getTextures()));
			deferredShader.setInt("blur", ssaoBlur);
			deferredShader.setFloat("zNear", camera.nearPlane);
			deferredShader.setFloat("zFar", camera.farPlane);
			billboard.draw();
		});

		dofFbo.draw([&]() {
			currentDofShader->use(deferredFbo.getTextures());
			currentDofShader->setInt("apertureBlades", camera.apertureBlades);
			currentDofShader->setInt("iterations", camera.dofSamples);
			currentDofShader->setFloat("focus", camera.focusDistance);
			currentDofShader->setFloat("focusScale", camera.focusScale);
			currentDofShader->setVec2(
				"pixelSize", 1.f / float(width), 1.f / float(height)
			);
			billboard.draw();
		});

		if (debugFbo == nullptr) {
			postShader.use(dofFbo.getTextures());
			postShader.setFloat("vignetteStrength", camera.vignetteStrength);
			postShader.setFloat("vignetteFalloff", camera.vignetteFalloff);
			postShader.setFloat("aspectRatio", camera.aspectRatio);
			postShader.setFloat("vignetteDesaturation", camera.vignetteDesaturation);
			postShader.setFloat("dispersion", camera.dispersionStrength);
			postShader.setFloat("dispersionFalloff", camera.dispersionFalloff);
			postShader.setFloat("barrelDistortionFalloff", camera.barrelDistortionFalloff);
			postShader.setFloat("barrelDistortion", camera.barrelDistortion);
			postShader.setFloat("crop", camera.sensorCrop);
		} else {
			getDebugShader().use({ debugFbo });
			getDebugShader().setBool("red", debugRed);
			getDebugShader().setFloat("scale", debugScale);
		}
		billboard.draw();
	}

	void onEvent(Event& e) override {
		if (e.type == Event::RESET_TEST_CAM) {
			camera = getTestCam(true);
		}
	}

	void onResize(int w, int h) override {
		debugFbo = nullptr;
		Scene::onResize(w, h);
		gFbo.resize(w, h, camera.resolutionScale);
		ssaoFbo.resize(w, h, ssaoScale * camera.resolutionScale);
		deferredFbo.resize(w, h, camera.resolutionScale);
		dofFbo.resize(w, h, camera.resolutionScale);
		camera.aspectRatio = w / float(h);
		
	}

	void debugUi() override {
		const auto helpMaker = [](const char* desc) {
			ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered()) {
				ImGui::BeginTooltip();
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted(desc);
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
		};
		
		if (ImGui::CollapsingHeader("Camera Settings"), ImGuiTreeNodeFlags_DefaultOpen) {
			if (ImGui::TreeNode("General")) {
				ImGui::SliderFloat("Aspect Ratio", &camera.aspectRatio, 0.1f, 4.f);
				helpMaker("Aspect ratio to simulate effects of a round lens");
				ImGui::SliderFloat("Field of View", &camera.fieldOfView, 1.f, 180.f);
				helpMaker("Field of view in degrees");
				ImGui::SliderFloat("Sensor Crop", &camera.sensorCrop, -1.f, 0.9f);
				helpMaker("Will zoom im on a smaller part of the image, same effect as FoV, but allows to crop away borders caused by barrel distortion");
				float scale = camera.resolutionScale;
				ImGui::SliderFloat("Resolution Scale", &scale, 0.1f, 4.f);
				helpMaker("Scales up the render resolution to adjust for cropping in");
				if (scale != camera.resolutionScale) {
					camera.resolutionScale = scale;
					onResize(width, height);
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Depth of Field")) {
				ImGui::SliderFloat(
					"Focus distance", &camera.focusDistance, 
					camera.nearPlane, 20
				);
				ImGui::SliderFloat("Focus Scale", &camera.focusScale, 0.0f, 30.0f);
				helpMaker("Strength of the depth of field");
				ImGui::SliderInt("Bokeh Edges", &camera.apertureBlades, 3, 14);
				helpMaker("How many aperture blades the Bokeh has");
				ImGui::SliderInt("Samples", &camera.dofSamples, 0, 512);
				helpMaker("Depth of field samples per pixel");
				
				ImGui::Separator();
				int e = 0;
				if (currentDofShader == &dofAdvancedShader) { e = 1; }
				if (currentDofShader == &dofTestShader) { e = 2; }
				ImGui::Text("Method");
				ImGui::RadioButton("Simple", &e, 0); ImGui::SameLine();
				helpMaker("Naive gather DOF without any optimizations to mitigate artifacts"); ImGui::SameLine();
				ImGui::RadioButton("Advanced", &e, 1);
				helpMaker("Test dof shader"); ImGui::SameLine();
				ImGui::RadioButton("Test", &e, 2);
				if (e == 0) { currentDofShader = &dofSimpleShader; }
				if (e == 1) { currentDofShader = &dofAdvancedShader; }
				if (e == 2) { currentDofShader = &dofTestShader; }
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Vignette")) {
				ImGui::SliderFloat("Strength", &camera.vignetteStrength, 0.f, 4.f);
				ImGui::SliderFloat("Falloff", &camera.vignetteFalloff, 0.f, 4.f);
				ImGui::SliderFloat(
					"Desaturation", &camera.vignetteDesaturation, 0.0f, 1.2f
				);
				helpMaker("Desaturates the image around the Edges");
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Distortion")) {
				ImGui::SliderFloat(
					"Dispersion Strength", &camera.dispersionStrength, -10.f, 10.f
				);
				ImGui::SliderFloat(
					"Dispersion Falloff", &camera.dispersionFalloff, 0.f, 4.f
				);
				ImGui::SliderFloat(
					"Barrel Distortion", &camera.barrelDistortion, -6.f, 1.f
				);
				ImGui::SliderFloat(
					"Barrel Distortion Falloff", &camera.barrelDistortionFalloff,
					0.f, 4.f
				);
				ImGui::TreePop();
			}
		}
		
		/**
		 * Extra Stuff
		 */
		if (ImGui::CollapsingHeader("FBO Debug")) {
			ImGui::Checkbox("Red only", &debugRed); ImGui::SameLine();
			ImGui::SliderFloat("Scale", &debugScale, 0.001f, 2.f);
			std::vector<FrameBufferObject*> fbos = { &gFbo, &ssaoFbo, &deferredFbo, &dofFbo };
			int i = 0;
			for (auto& f : fbos) {
				Textures textures = f->getTextures();
				for (auto& t : textures) {
					if (debugFbo == nullptr) { debugFbo = t; }
					bool enabled = debugFbo.get() == t.get();
					bool was = enabled;
					ImGui::Checkbox((t->getName() + std::to_string(i)).c_str(), &enabled);
					ImGui::SameLine();
					if (enabled && !was) {
						debugFbo = t;
					}
				}
				i++;
				ImGui::Separator();
			}
		} else {
			debugFbo = nullptr;
		}

		if (ImGui::CollapsingHeader("SSAO")) {
			float sscale = ssaoScale;
			ImGui::SliderFloat("Resolution", &sscale, 0.1f, 4.f);
			if (sscale != ssaoScale) {
				ssaoScale = sscale;
				ssaoFbo.resize(width, height, ssaoScale);
			}
			ImGui::SliderFloat("Strength", &ssaoStrength, 0.1f, 10.f);
			ImGui::SliderFloat("Radius", &ssaoRadius, 0.01f, 2.f);
			ImGui::SliderFloat("Bias", &ssaoBias, 0.001f, 1.f);
			ImGui::SliderInt("Blur", &ssaoBlur, 0, 32);
			ImGui::SliderInt("Samples", &ssaoSamples, 1, 256);
		}
	}
};
