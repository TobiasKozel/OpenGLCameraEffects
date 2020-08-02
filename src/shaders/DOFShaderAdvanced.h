#pragma once
#include "../wrapper/Shader.h"


/**
 * Slightly altered version of this DOF Shader
 * http://tuxedolabs.blogspot.com/2018/05/bokeh-depth-of-field-in-single-pass.html
 * It's gathering surrounding samples in a spiral pattern
 * and performs blending based on the coc of these samples
 * which results in a smooth falloff
 */
inline Shader& getDOFShaderAdvanced() {
    static Shader shader = { Shader::getBillboardVertexShader(), GLSL(
        out vec3 FragColor;
        in vec2 TexCoords;

        uniform sampler2D shadedPass;
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

        const float PI = 3.1415926f;

        const float GOLDEN_ANGLE = 2.39996323;
        const float MAX_BLUR_SIZE = 20.0;

        float getBlurSize(float depth) {
            return abs(
                (focalLength * (focus - depth)) /
                (depth * (focus - focalLength))
            ) * (focalLength / aperture) * 10000.0;
        }

        void main() {
            vec2 uv = TexCoords;

            float centerDepth = texture(linearDistance, TexCoords).r;
            float centerBlur = getBlurSize(centerDepth);
            vec3 color = texture(shadedPass, TexCoords).rgb;
            float steps = 1.0;
            
            // Smaller = nicer blur, larger = faster. Will roughly reach the max blur size with the samples given
            float RAD_SCALE = 12.5 / (float(iterations) + 11.0) * MAX_BLUR_SIZE; 
            float radius = RAD_SCALE;
            float n = apertureBlades;
            for (float ang = 0.0; radius < MAX_BLUR_SIZE; ang += GOLDEN_ANGLE) {
                float r = radius * cos(PI / n);
                r /= cos(ang - (2.0f * PI / n) * floor((n * ang + PI) / 2.0f / PI));
                vec2 offset =    vec2(cos(ang), sin(ang)) * pixelSize * r;

                vec3 sampleColor = texture(shadedPass, uv + offset).rgb;
                float sampleDepth = texture(linearDistance, uv + offset).r;
                float sampleBlur = getBlurSize(sampleDepth);
                
                if (sampleDepth > centerDepth) {
                    sampleBlur = clamp(sampleBlur, 0.0, centerBlur * 2.0);
                }

                float m = smoothstep(radius - 0.5, radius + 0.5, sampleBlur);
                color += mix(color / steps, sampleColor, m);
                steps += 1.0;
                radius += RAD_SCALE / radius;
            }
            color /= steps;
            FragColor = color;
        }
    ), __FILE__ };
    return shader;
}

