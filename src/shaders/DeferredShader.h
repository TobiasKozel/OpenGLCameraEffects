#pragma once

#include "../wrapper/Shader.h"

/**
 * Simple deferred shader which only filters and applies the SSAO
 * https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/9.ssao/9.ssao_blur.fs
 *
 * Also scales the zbuffer linearly to actual scale
 */
inline Shader& getDeferredShader() {
	static Shader shader = { Shader::getBillboardVertexShader(), GLSL(
		layout(location = 0) out vec4 outColor;
		layout(location = 1) out float outZBufferLinear;

		in vec2 TexCoords;
		uniform sampler2D gPosition;
		uniform sampler2D gNormal;
		uniform sampler2D gColor;
		uniform sampler2D gGreySoft; // The unblurred SSAO

		uniform float zNear;
		uniform float zFar;
		uniform int blur = 4;

		float linearDepth(float depthSample) {
			return (2.0 * zNear) / (zFar + zNear - depthSample * (zFar - zNear)) * zFar;
		}

		float blurredSSAO(float depth) {
			if (blur == 0) { // Skip the whole thing and return the unblurred ssao
				return texture(gGreySoft, TexCoords).r;
			}
			vec2 texelSize = 1.0 / vec2(textureSize(gGreySoft, 0));
			float result = 0.0;
			for (int x = -blur; x < blur; ++x) {
				for (int y = -blur; y < blur; ++y) {
					vec2 offset = vec2(float(x), float(y)) * texelSize;
					result += texture(gGreySoft, TexCoords + offset).r;
				}
			}
			int d = blur * 2;
			return result / (d * d);
		}

		void main() {
			outZBufferLinear = -texture(gPosition, TexCoords).z;
			vec4 color = texture(gColor, TexCoords);
			if (outZBufferLinear < zFar) {
				// No ssao for the sky
				color.rgb *= blurredSSAO(outZBufferLinear);
			}
			outColor = color;
		}
	), __FILE__ };
	return shader;
}