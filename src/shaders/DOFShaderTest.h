#pragma once
#include "../wrapper/Shader.h"

inline Shader& getDofShaderTest() {
	static Shader shader = { Shader::getBillboardVertexShader() , GLSL(
		out vec4 FragColor;
		in vec2 TexCoords;

		uniform sampler2D gColor; //Image to be processed
		uniform sampler2D zBufferLinear;
		uniform vec2 pixelSize; //The size of a pixel: vec2(1.0/width, 1.0/height)

		uniform float focus;
		uniform float focusScale;
		uniform int apertureBlades;
		uniform int iterations = 64;

		const float PI = 3.1415926f;
		const float PI_OVER_2 = 1.5707963f;
		const float PI_OVER_4 = 0.785398f;
		const float EPSILON = 0.000001f;
		const float MAX_BLUR_SIZE = 20.0;

		vec2 UnitSquareToUnitDiskPolar(float a, float b) {
			float radius;
			float angle;
			if (abs(a) > abs(b)) { // First region (left and right quadrants of the disk)
				radius = a;
				angle = b / (a + EPSILON) * PI_OVER_4;
			} else { // Second region (top and botom quadrants of the disk)
				radius = b;
				angle = PI_OVER_2 - (a / (b + EPSILON) * PI_OVER_4);
			}
			if (radius < 0) { // Always keep radius positive
				radius *= -1.0f;
				angle += PI;
			}
			return vec2(radius, angle);
		}

		vec2 SquareToDiskMapping(float a, float b) {
			vec2 PolarCoord = UnitSquareToUnitDiskPolar(a, b);
			return vec2(PolarCoord.x * cos(PolarCoord.y), PolarCoord.x * sin(PolarCoord.y));
		}

		vec2 SquareToPolygonMapping(float a, float b, float edgeCount, float shapeRotation) {
			vec2 PolarCoord = UnitSquareToUnitDiskPolar(a, b); // (radius, angle)

			// Re-scale radius to match a polygon shape
			PolarCoord.x *= cos(PI / edgeCount) /
				cos(PolarCoord.y - (2.0f * PI / edgeCount) * floor((edgeCount * PolarCoord.y + PI) / 2.0f / PI));

			// Apply a rotation to the polygon shape
			PolarCoord.y += shapeRotation;

			return vec2(PolarCoord.x * cos(PolarCoord.y), PolarCoord.x * sin(PolarCoord.y));
		}

		float getBlurSize(float depth) {
			float coc = clamp((1.0 / focus - 1.0 / depth), -1.0, 1.0);
			return abs(coc) * focusScale;
		}

		void main() {
			vec3 color = texture(gColor, TexCoords).rgb;
			
			if (iterations == 0) {
				FragColor = vec4(color, 1.0);
				return;
			}

			vec2 uv = TexCoords;

			
			float depth = texture(zBufferLinear, TexCoords).r;
			float coc = getBlurSize(depth);
			
			int iterationsX = int(floor(sqrt(float(iterations))));
			float stepSize = 2.0f / float(iterationsX);
			float steps = 1.0;
			for (float x = -1.0; x <= 1.0; x += stepSize) {
				for (float y = -1.0; y <= 1.0; y += stepSize) {
					vec2 offset = SquareToPolygonMapping(x, y, float(apertureBlades), 0.f) * pixelSize * focusScale;
					vec2 uv = TexCoords + offset;
					float sdepth = texture(zBufferLinear, uv).r;
					float scoc = getBlurSize(sdepth);
					//if (sdepth > depth) {
					//	scoc = clamp(scoc, 0.0, coc * 2.0);
					//}
					
					float m = clamp(scoc, 0.0, 1.0);
					uv = TexCoords + offset * m;
					vec3 scolor = texture(gColor, uv).rgb;
					color += mix(color / steps, scolor, m);
					//color += scolor;
					steps += 1.0;
				}
			}
			color /= steps;
			FragColor = vec4(color, 1.0);
		}
	), __FILE__ };
	return shader;
}