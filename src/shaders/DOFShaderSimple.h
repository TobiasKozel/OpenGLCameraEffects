#pragma once
#include "../wrapper/Shader.h"


/**
 * Simple DOF
 * Blurs the image with a blur size directly based of its own circle of confusion
 */
inline Shader &getDofShaderSimple() {
    static Shader shader = { Shader::getBillboardVertexShader() , GLSL(
        out vec3 FragColor;
        in vec2 TexCoords;

        uniform sampler2D shadedPass; //Image to be processed
        uniform sampler2D linearDistance;
        uniform vec2 pixelSize; //The size of a pixel: vec2(1.0/width, 1.0/height)

        uniform float focus;
        uniform float aperture;
        uniform float focalLength;
        uniform int iterations = 64;
        uniform int apertureBlades;
        uniform float bokehSqueeze;
        uniform float bokehSqueezeFalloff;
        uniform float aspectRatio = 1.777;

        const float MAX_BLUR_SIZE = 20.0;

        float getBlurSize(float depth) {
            return abs(
                (focalLength * (focus - depth)) /
                (depth * (focus - focalLength))
            ) * (focalLength / aperture) * 10000.0;
        }

        float rand(vec2 co) {
            return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
        }

        vec2 rand2(vec2 co) {
            return vec2(rand(co), rand(co * 20.0)) * 2.0 - 1.0;
        }

        void main() {
            float centerDepth = texture(linearDistance, TexCoords).r;
            float centerBlur = getBlurSize(centerDepth);
            vec3 color = texture(shadedPass, TexCoords).rgb;
            for (int i = 0; i < iterations; i++) {
                vec2 offset = rand2(TexCoords + i) * centerBlur;
                vec2 uv = TexCoords + offset * pixelSize;
                color += texture(shadedPass, uv).rgb;
            }
            color /= float(iterations);
            FragColor = color;
            // FragColor = vec3(centerBlur);
        }
    ), __FILE__ };
    return shader;
}