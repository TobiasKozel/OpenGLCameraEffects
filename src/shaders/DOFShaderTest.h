#pragma once
#include "../wrapper/Shader.h"

inline Shader& getDofShaderTest() {
	static Shader shader = { Shader::getBillboardVertexShader() , GLSL(
		out vec3 FragColor;
		in vec2 TexCoords;

		uniform sampler2D gColor; //Image to be processed
		uniform sampler2D zBufferLinear;
		uniform vec2 pixelSize; //The size of a pixel: vec2(1.0/width, 1.0/height)

		uniform float focus;
		uniform float focusScale;
		uniform int apertureBlades;
		uniform int iterations = 64;

		uniform float vignetteStrength = 1.0;
		uniform float vignetteFalloff = 1.0;
		uniform float vignetteDesaturation = 0.0;
		uniform float dispersionFalloff = 1.0;
		uniform float dispersion = 1.0;
		uniform float barrelDistortion = 1.0;
		uniform float barrelDistortionFalloff = 1.0;
		uniform float aspectRatio = 1.777;
		uniform float crop = 0.0;

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

		float map(float value, float min1, float max1, float min2, float max2) {
			return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
		}

		mat2 rotate2d(float angle) {
			return mat2(
				cos(angle), -sin(angle),
				sin(angle),  cos(angle)
			);
		}

		void main() {
			vec2 uv = TexCoords;
			uv = uv * (1.0 - crop) + crop * 0.5;
			// Vector pointing away from the center
			vec2 fromCenter = (vec2(0.5, 0.5) - uv) * vec2(aspectRatio, 1.0);
			float fromCenterLength = length(fromCenter);
			// Calc barrel distortion strength
			float barrel = pow(fromCenterLength, barrelDistortionFalloff) * barrelDistortion;
			// The Base UV before dispersion
			// uv = uv + fromCenter * barrel;

			if (uv.x > 1.0 || uv.y > 1.0 || uv.x < 0.0 || uv.y < 0.0) {
				FragColor = vec3(0); // Black out the border of the screen
				return;
			}
			
			vec3 color = texture(gColor, uv).rgb;
			
			if (iterations == 0) {
				FragColor = color;
				return;
			}

			float depth = texture(zBufferLinear, uv).r;
			float coc = getBlurSize(depth);
			
			int iterationsX = int(floor(sqrt(float(iterations))));
			float stepSize = 2.0f / float(iterationsX);
			float steps = 1.0;
			float angle = acos(dot(vec2(1.0, 0.0), normalize(fromCenter)));

			mat2 test = rotate2d(angle) * mat2(1.0, 0.0, 0.0, 1.0 - barrel) * rotate2d(-angle);
			test = test * mat2(pixelSize.x * focusScale, 0.0, 0.0, pixelSize.y * focusScale);
			for (float x = -1.0; x <= 1.0; x += stepSize) {
				for (float y = -1.0; y <= 1.0; y += stepSize) {
					vec2 offset = SquareToPolygonMapping(x, y, float(apertureBlades), 0.f) * test;
					float sdepth = texture(zBufferLinear, uv + offset).r;
					float scoc = getBlurSize(sdepth);
					//if (sdepth < depth) {
					//	scoc = clamp(scoc, 0.0, coc * 2.0);
					//}
					float m = clamp(scoc, 0.0, 1.0);
					vec3 scolor = texture(gColor, uv + offset * m).rgb;
					//color += scolor;

					float dist = length(offset);
					//float m = smoothstep(dist - 0.5, dist + 0.5, scoc);
					//vec3 scolor = texture(gColor, uv + offset).rgb;
					color += mix(color / steps, scolor, m);
					steps += 1.0;
				}
			}
			//color /= steps;
 
			// Calc vignette strength
			float vignette = pow(fromCenterLength, vignetteFalloff) * vignetteStrength;
			color *= 1.0 - vignette;
			//            Desaturate the edges            Apply the Vignette
			color = mix(color, vec3(length(color)), vignette * vignetteDesaturation);
			FragColor = color;
		}
	), __FILE__ };
	return shader;
}