#pragma once
#include "../wrapper/Shader.h"

/**
 * A deferred shader which will combine all the passes into a single color buffer
 * https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/9.ssao/9.ssao_geometry.vs
 * https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/9.ssao/9.ssao_geometry.fs
 */
inline Shader &getGBufferShader() {
	static Shader shader = {GLSL(
		layout (location = 0) in vec3 aPos;
		layout (location = 1) in vec3 aNormal;
		layout (location = 2) in vec2 aTexCoords;
		
		out vec3 FragPos;
		out vec2 TexCoords;
		out vec3 Normal;
		
		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;
		
		void main() {
		    vec4 viewPos = view * model * vec4(aPos, 1.0);
		    FragPos = viewPos.xyz; 
		    TexCoords = aTexCoords;
		    
		    mat3 normalMatrix = transpose(inverse(mat3(view * model)));
		    Normal = normalMatrix * (aNormal);
		    
		    gl_Position = projection * viewPos;
		}
	), GLSL(
		layout (location = 0) out vec3 gPosition;
		layout (location = 1) out vec3 gNormal;
		layout (location = 2) out vec4 gColor;
		in vec2 TexCoords;
		in vec3 FragPos;
		in vec3 Normal;
		uniform bool use_color;
		uniform vec4 diffuse_color;

		uniform sampler2D texture_diffuse;
		
		void main() {
			vec4 color;
			if (use_color) {
				color = diffuse_color;
			} else {
				color = texture(texture_diffuse, TexCoords);
			}
			
			if (color.a < 0.01) {
				discard;
			} else {
			    gPosition = FragPos;
			    gNormal = normalize(Normal);
				gColor = color;
			}
		}
	), __FILE__ };
	return shader;
}
