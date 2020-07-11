#pragma once
#include "../wrapper/Shader.h"

inline Shader& getDofShaderTest() {
	static Shader shader = { Shader::getBillboardVertexShader() , GLSL(
		out vec4 FragColor;
		in vec2 TexCoords;

		uniform sampler2D gColor; //Image to be processed
		uniform sampler2D zBufferLinear; //Linear depth, where 1.0 == far plane
		uniform vec2 pixelSize; //The size of a pixel: vec2(1.0/width, 1.0/height)

		uniform float focus;
		uniform float focusScale;
		uniform int iterations = 64;

		float rand(vec2 co) {
			return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
		}

		vec2 rand2(vec2 co) {
			return vec2(rand(co), rand(co * 20.0)) * 2.0 - 1.0;
		}

		void main() {
			float depth = texture(zBufferLinear, TexCoords).r;
			if (abs(depth - focus) < 0.1) {
				FragColor = vec4(vec3(1.0), 1.0);
			} else {
				FragColor = vec4(vec3(0.0), 1.0);
			}

			//depth = (depth - focus) * 0.075 * focusScale;
			//vec3 color = vec3(0);
			//for (int i = 0; i < iterations; i++) {
			//	vec2 offset = rand2(TexCoords + i) * depth;
			//	color += texture(gColor, TexCoords + offset * pixelSize).rgb;
			//}
			//color /= float(iterations);
			//FragColor = vec4(color, 1.0);
		}
	), __FILE__ };
	return shader;
}