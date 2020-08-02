#pragma once
#include "../wrapper/Shader.h"

/**
 * Slightly altered version of
 * https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/9.ssao/9.ssao.fs
 * Doens't use the noise texture and kernel for convenience
 */
inline Shader& getSsaoShader() {
    static Shader shader = { Shader::getBillboardVertexShader(), GLSL(
        layout (location = 0) out float ssaoPass;
        in vec2 TexCoords;
        uniform sampler2D gPosition;
        uniform sampler2D gNormal;
        uniform float strength = 1.0;
        uniform float radius = 0.3;
        uniform float bias = 0.025;
        uniform int count = 32;

        uniform mat4 projection;

        float rand(vec2 co) {
            return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
        }

        /**
         * To lazy to provide a kernel from outside, so just generate something similar
         * This this is nosier but works
         */
        vec3 randKernel(vec2 co) {
            return vec3(1.0 - rand(co) * 2.0, 1.0 - rand(co * 20.0) * 2.0, rand(co - 20.0));
        }

        float ssao() {
            vec3 fragPos = texture(gPosition, TexCoords).xyz;
            if (fragPos.z > 0.0) { return 1.0; } // Sky can be skipped
            vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
            vec3 randomVec = normalize(vec3(randKernel(fragPos.xy).xy, 0.0));
            vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
            vec3 bitangent = cross(normal, tangent);
            mat3 TBN = mat3(tangent, bitangent, normal);

            float occlusion = 0.0;
            for (int i = 0; i < count; i++) {
                // get sample position
                vec3 sample = TBN * randKernel(fragPos.xy * float(i)); // from tangent to view-space
                sample = fragPos + sample * radius;

                // project sample position (to sample texture) (to get position on screen/texture)
                vec4 offset = vec4(sample, 1.0);
                offset = projection * offset; // from view to clip-space
                offset.xyz /= offset.w; // perspective divide
                offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

                // get sample depth
                float sampleDepth = texture(gPosition, offset.xy).z; // get depth value of kernel sample

                // range check & accumulate
                float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
                occlusion += (sampleDepth >= sample.z + bias ? 1.0 : 0.0) * rangeCheck;
            }
            occlusion = 1.0 - (occlusion / float(count));
            return occlusion;
        }

        void main() {
            float ao = ssao();
            ssaoPass = pow(ao, strength);
        }
    ), __FILE__ };
    return shader;
}
