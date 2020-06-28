#pragma once
#include "../wrapper/Shader.h"


/**
 * Shader which will draw a billboard and assemble the g buffer
 */
inline Shader& getPostShader() {
	static Shader shader = { Shader::getBillboardVertexShader() , GLSL(
		out vec4 FragColor;
		in vec2 TexCoords;

		uniform sampler2D gColorSoft; //Image to be processed

		uniform float vignetteStrength = 1.0;
		uniform float vignetteFalloff = 1.0;
		uniform float vignetteDesaturation = 0.0;
		uniform float dispersionFalloff = 1.0;
		uniform float dispersion = 1.0;
		uniform float barrelDistortion = 1.0;
		uniform float barrelDistortionFalloff = 1.0;
		uniform float aspectRatio = 1.777;
		uniform float crop = 0.0;

		void main() {
			// Crop in
			vec2 uv = TexCoords;
			uv = uv * (1.0 - crop) + crop * 0.5;
			
			// Vector pointing away from the center
			vec2 fromCenter = (vec2(0.5, 0.5) - uv) * vec2(aspectRatio, 1.0);
			float fromCenterLength = length(fromCenter);
			// Calc barrel distortion strength
			float barrel = pow(fromCenterLength, barrelDistortionFalloff) * barrelDistortion;
			// The Base UV before dispersion
			vec2 baseUv = uv + fromCenter * barrel;

			if (baseUv.x > 1.0 || baseUv.y > 1.0 || baseUv.x < 0.0 || baseUv.y < 0.0 ) {
				FragColor = vec4(vec3(0), 1.0); // Black out the border of the screen
				return;
			}

			vec3 color = vec3(0);

			if (abs(dispersion) < 0.01) { // No disperion
				color = texture(gColorSoft, baseUv).rgb;
			} else {
				// Calc dispersion strength
				float disp = pow(fromCenterLength, dispersionFalloff) * dispersion * (1.0 + abs(dispersion - 1.0));
				color.x += texture(gColorSoft, baseUv).x * .66;
				color.xy += texture(gColorSoft, baseUv + (fromCenter * disp * 0.005)).xy * .33;
				color.y += texture(gColorSoft, baseUv + (fromCenter * disp * 0.001)).y * .33;
				color.yz += texture(gColorSoft, baseUv + (fromCenter * disp * .007)).yz * .33;
				color.z += texture(gColorSoft, baseUv + (fromCenter * disp * 0.003)).z * .66;
			}

			// Calc vignette strength
			float vignette = pow(fromCenterLength, vignetteFalloff) * vignetteStrength;
			color *= 1.0 - vignette;
			//            Desaturate the edges            Apply the Vignette
			color = mix(color, vec3(length(color)), vignette * vignetteDesaturation);
			
			FragColor = vec4(color, 1.0);
		}
	), __FILE__ };
	return shader;
}