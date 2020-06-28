#pragma once

#include "../wrapper/Shader.h"

/**
 * Default shader which draws a shadeless object with a texture
 */
inline Shader& getDefaultShader() {
	static Shader shader = { GLSL(
		layout(location = 0) in vec3 aPos;
		layout(location = 1) in vec3 aNormal;
		layout(location = 2) in vec2 aTexCoords;

		out vec2 TexCoords;

		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

		void main() {
			TexCoords = aTexCoords;
			// Normal = transpose(inverse(mat3(view * model)) * aNormal;
			gl_Position = projection * view * model * vec4(aPos, 1.0);
		}
	), GLSL(
		out vec4 FragColor;
		in vec2 TexCoords;
		uniform sampler2D texture_diffuse1;

		void main() {
			FragColor = texture(texture_diffuse1, TexCoords);
		}
	), __FILE__ };
	return shader;
}