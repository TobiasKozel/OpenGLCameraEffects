#pragma once
#include "../wrapper/Shader.h"

inline Shader& getDofShaderShape() {
    static Shader shader = { Shader::getBillboardVertexShader() , GLSL(
        out vec3 FragColor;
        in vec2 TexCoords;

        uniform sampler2D shadedPass;
        uniform sampler2D linearDistance;
        uniform vec2 pixelSize; //The size of a pixel: vec2(1.0/width, 1.0/height)

        uniform float focus;
        uniform float aperture;
        uniform float focalLength;
        uniform int apertureBlades;
        uniform float bokehSqueeze;
        uniform float bokehSqueezeFalloff;
        uniform float aspectRatio = 1.777;
        uniform int iterations = 64;

        const float PI = 3.1415926f;
        const float PI_OVER_2 = 1.5707963f;
        const float PI_OVER_4 = 0.785398f;
        const float EPSILON = 0.000001f;

        /**
         * Projects coordinates on a unit square from -1.0 to 1.0 onto a circle
         * Ported GLSL version of
         * http://www.adriancourreges.com/blog/2018/12/02/ue4-optimized-post-effects/
         * Based on Shirley’s concentric mapping
         */
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

        /**
         * 2D Rotation matrix from a radian angle
         */
        mat2 rotate2d(float angle) {
            return mat2(
                cos(angle), -sin(angle),
                sin(angle),  cos(angle)
            );
        }


        float getBlurSize(float depth) {
            return abs(
                (focalLength * (focus - depth)) /
                (depth * (focus - focalLength))
            ) * (focalLength / aperture) * 1000.0;
        }

        void main() {
            vec2 uv = TexCoords;
            
            vec3 color = texture(shadedPass, uv).rgb;
            
            if (iterations == 0) {
                FragColor = color;
                return;
            }

            /**
             * Vector pointing away from the center
             */
            vec2 fromCenter = (vec2(0.5, 0.5) - uv) * vec2(aspectRatio, 1.0);
            /**
             * Calc squeeze distortion strength
             */
            float squeezeStrength = pow(length(fromCenter), bokehSqueezeFalloff) * bokehSqueeze;

            /**
             * This will cause the 'cats eye bokeh' at the edges of the
             * screen by squeezing it based on the direction to the center
             */
            float angle = acos(dot(vec2(1.0, 0.0), normalize(fromCenter)));

            /**
             * flip the lower half, so it bends symmetrically
             */
            if (fromCenter.y > 0.0) { angle = -angle; }

            /**
             * Build a matrix to squeeze in a single step and
             * also scale according to the focus scale
             */
            mat2 squeeze = rotate2d(angle) * mat2(1.0 + squeezeStrength, 0.0, 0.0, 1.0) * rotate2d(-angle);
            squeeze = squeeze * mat2(
                pixelSize.x, 0.0,
                0.0, pixelSize.y
            );

            float centerDepth = texture(linearDistance, TexCoords).r;
            float centerBlur = getBlurSize(centerDepth);

            /**
             * Handle the iterations 
             */
            int iterationsX = int(floor(sqrt(float(iterations))));
            float stepSize = 2.0f / float(iterationsX);
            float steps = 1.0;

            for (float x = -1.0; x <= 1.0; x += stepSize) {
                for (float y = -1.0; y <= 1.0; y += stepSize) {
                    /**
                     * This is the offset we'll use to get the depth and color
                     * It's already in the bokeh shape and squeezed
                     */
                    vec2 offset = SquareToPolygonMapping(x, y, float(apertureBlades), 0.0);
                    offset *= (focalLength / aperture) * 50.0;
                    offset *= squeeze;

                    float sampleDepth = texture(linearDistance, uv + offset).r;
                    float sampleBlur = getBlurSize(sampleDepth);

                    if (sampleDepth > centerDepth) {
                        sampleBlur = clamp(sampleBlur, 0.0, centerBlur * 2.0);
                    }

                    sampleBlur = clamp(sampleBlur, 0.0, 1.0);

                    /**
                     * Based on that we'll move the sample point closer to the center.
                     */
                    vec3 sampleColor = texture(shadedPass, uv + offset * sampleBlur).rgb;

                    color += sampleColor;
                    steps += 1.0;
                }
            }
            color /= steps;
            FragColor = color;
        }
    ), __FILE__ };
    return shader;
}