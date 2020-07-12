#pragma once
#include "../wrapper/Shader.h"


/**
 * Simple DOF
 * Blurs the image with a blur size directly based of the circle of confusion
 */
inline Shader &getDofShaderSimple() {
	static Shader shader = { Shader::getBillboardVertexShader() , GLSL(
		out vec3 FragColor;
		in vec2 TexCoords;

		uniform sampler2D gColor; //Image to be processed
		uniform sampler2D zBufferLinear;
		uniform vec2 pixelSize; //The size of a pixel: vec2(1.0/width, 1.0/height)

		uniform float focus;
		uniform float focusScale;
		uniform int iterations = 64;
		uniform int apertureBlades;

		const float MAX_BLUR_SIZE = 20.0;

		float getBlurSize(float depth) {
			float coc = clamp((1.0 / focus - 1.0 / depth) * focusScale, -1.0, 1.0);
			return abs(coc) * MAX_BLUR_SIZE;
		}

		float rand(vec2 co) {
			return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
		}

		vec2 rand2(vec2 co) {
			return vec2(rand(co), rand(co * 20.0)) * 2.0 - 1.0;
		}
	
		void main() {
			float depth = texture(zBufferLinear, TexCoords).r;
			depth = getBlurSize(depth);
			vec3 color = vec3(0);
			for (int i = 0; i < iterations; i++) {
				vec2 offset = rand2(TexCoords + i) * depth ;
				color += texture(gColor, TexCoords + offset * pixelSize).rgb;
			}
			color /= float(iterations);
			FragColor = color;
		}
	), __FILE__ };
	return shader;
}