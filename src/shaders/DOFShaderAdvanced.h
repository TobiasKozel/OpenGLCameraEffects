#pragma once
#include "../wrapper/Shader.h"


/**
 * Slightly alter versio nof this DOF Shader
 * http://tuxedolabs.blogspot.com/2018/05/bokeh-depth-of-field-in-single-pass.html
 */
inline Shader& getDOFShaderAdvanced() {
	static Shader shader = { Shader::getBillboardVertexShader(), GLSL(
		out vec4 FragColor;
		in vec2 TexCoords;

		uniform sampler2D gColor; //Image to be processed
		uniform sampler2D zBufferLinear; //Linear depth, where 1.0 == far plane
		uniform vec2 pixelSize; //The size of a pixel: vec2(1.0/width, 1.0/height)

		uniform float focus;
		uniform float focusScale;
		uniform int iterations = 64;

		const float GOLDEN_ANGLE = 2.39996323;
		const float MAX_BLUR_SIZE = 20.0;

		float getBlurSize(float depth, float focusPoint, float focusScale) {
			float coc = clamp((1.0 / focusPoint - 1.0 / depth) * focusScale, -1.0, 1.0);
			return abs(coc) * MAX_BLUR_SIZE;
		}

		vec3 depthOfField(float focusPoint, float focusScale) {
			float centerDepth = texture(zBufferLinear, TexCoords).r;
			float centerSize = getBlurSize(centerDepth, focusPoint, focusScale);
			vec3 color = texture(gColor, TexCoords).rgb;
			float tot = 1.0;
			
			// Smaller = nicer blur, larger = faster. Will roughly reach the max blur size with the samples given
			float RAD_SCALE = 12.5 / (float(iterations) + 11.0) * MAX_BLUR_SIZE; 
			float radius = RAD_SCALE;
			float ang = 0.0;
			for (int i = 0; i < iterations; i++) {
				vec2 tc = TexCoords + vec2(cos(ang), sin(ang)) * pixelSize * radius;

				vec3 sampleColor = texture(gColor, tc).rgb;
				float sampleDepth = texture(zBufferLinear, tc).r;
				float sampleSize = getBlurSize(sampleDepth, focusPoint, focusScale);
				
				if (sampleDepth > centerDepth) {
					sampleSize = clamp(sampleSize, 0.0, centerSize * 2.0);
				}

				float m = smoothstep(radius - 0.5, radius + 0.5, sampleSize);
				color += mix(color / tot, sampleColor, m);
				tot += 1.0;
				radius += RAD_SCALE / radius;
				ang += GOLDEN_ANGLE;
			}
			return color /= tot;
		}

		void main() {
			FragColor = vec4(depthOfField(focus, focusScale), 1.0);
		}
	), __FILE__ };
	return shader;
}

