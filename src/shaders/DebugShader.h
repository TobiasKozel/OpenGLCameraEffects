#pragma once

#include "../wrapper/Shader.h"

/**
 * Simple shader to look at the FBOs and the red channel on its own
 */
inline Shader& getDebugShader() {
	static Shader shader = { Shader::getBillboardVertexShader(), GLSL(
		out vec4 FragColor;
		in vec2 TexCoords;
		uniform sampler2D gNormal;
		uniform float scale = 1.0;
		uniform bool red = false;

		void main() {
			if (red) {
				FragColor = vec4(vec3(texture(gNormal, TexCoords).r), 1.0);
			} else {
				FragColor = vec4(texture(gNormal, TexCoords).rgb, 1.0);
			}
			FragColor.rgb *= scale;
		}
	), __FILE__ };
	return shader;
}