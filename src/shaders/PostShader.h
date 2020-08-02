#pragma once
#include "../wrapper/Shader.h"


/**
 * Post shader
 * Sensor crop, Barrel Distortion, Dispersion, Vignetting, Film Grain and basic Exposure
 */
inline Shader& getPostShader() {
    static Shader shader = { Shader::getBillboardVertexShader() , GLSL(
        out vec3 FragColor;
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
        uniform float grain = 1.0;
        uniform float grainSize = 1.0;
        uniform float time = 0.0;
        uniform float exposure = 1.0;

        const float eps = 0.001;

        /**
         *  Noise from https://www.shadertoy.com/view/4sfGzS
         */
        float hash(vec3 p) {
            p  = fract( p*0.3183099+.1 );
            p *= 17.0;
            return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
        }

        float noise( in vec3 x ) {
            vec3 i = floor(x);
            vec3 f = fract(x);
            f = f * f * (3.0 - 2.0 * f);
    
            return mix(
                mix(
                    mix(hash(i + vec3(0, 0, 0)),  hash(i + vec3(1, 0, 0)), f.x),
                    mix(hash(i + vec3(0, 1, 0)),  hash(i + vec3(1, 1, 0)), f.x),
                    f.y
                ),
                mix(
                    mix(hash(i+vec3(0, 0, 1)),  hash(i+vec3(1, 0, 1)), f.x),
                    mix(hash(i+vec3(0, 1, 1)),  hash(i+vec3(1 ,1 ,1)), f.x),
                    f.y
                ),
                f.z
            );
        }

        vec3 ACESFilm(vec3 x) {
            float a = 2.51;
            float b = 0.03;
            float c = 2.43;
            float d = 0.59;
            float e = 0.14;
            return (x*(a*x+b))/(x*(c*x+d)+e);
        }

        void main() {
            vec2 uv = TexCoords;
            
            /**
             * Sensor Crop
             */
            uv = uv * (1.0 - crop) + crop * 0.5;
            
            // Vector pointing away from the center
            vec2 fromCenter = (vec2(0.5, 0.5) - uv) * vec2(aspectRatio, 1.0);
            float fromCenterLength = length(fromCenter);

            /**
             * Barrel Distortion
             */
            float barrel = pow(fromCenterLength, barrelDistortionFalloff) * barrelDistortion;
            // The Base UV before dispersion
            vec2 baseUv = uv + fromCenter * barrel;

            if (baseUv.x > 1.0 || baseUv.y > 1.0 || baseUv.x < 0.0 || baseUv.y < 0.0 ) {
                /**
                 * Black out the border of the screen, since it might
                 * be visible after ctopping or adding the barrel distortion
                 */
                FragColor = vec3(0);
                return;
            }

            /**
             * Dispersion/Chromatic aberration
             */
            vec3 color = vec3(0);

            if (abs(dispersion) < eps) {
                // No disperion
                color = texture(gColorSoft, baseUv).rgb;
            } else {
                vec2 disp = fromCenter;
                // Calc dispersion strength
                disp *= pow(fromCenterLength, dispersionFalloff) * dispersion * (1.0 + abs(dispersion - 1.0));
                color.r  += texture(gColorSoft, baseUv                 ).r  * 0.66;
                color.rg += texture(gColorSoft, baseUv + (disp * 0.005)).rg * 0.33;
                color.g  += texture(gColorSoft, baseUv + (disp * 0.001)).g  * 0.33;
                color.gb += texture(gColorSoft, baseUv + (disp * 0.007)).gb * 0.33;
                color.b  += texture(gColorSoft, baseUv + (disp * 0.003)).b  * 0.66;
            }

            /**
             * Vignette
             */
            float vignette = pow(fromCenterLength, vignetteFalloff) * vignetteStrength;
            // Apply the vignette by multiplying
            color *= 1.0 - vignette;
            // Desaturate the edges
            color = mix(color, vec3(length(color)), vignette * vignetteDesaturation);
            

            /**
             * Film Grain
             */
            if (grain > eps) {
                /**
                 * Create 3D coordinates for the noise by using the UVs and
                 * the time as the z coordinate so the noise changes between frames
                 */
                vec3 n = vec3(noise(vec3(uv * grainSize * 1000.0 * vec2(aspectRatio, 1.0), time * 1000.0)));
                /**
                 * Apply the noise by multipling and adding a small amount
                 * so the noise is also visible in darker areas. Sensor noise usually is
                 */
                color = mix(color, color * n + n * 0.2, grain);
            }

            color = (color * exposure);

            FragColor = color;
        }
    ), __FILE__ };
    return shader;
}