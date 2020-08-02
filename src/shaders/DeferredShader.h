#pragma once

#include "../wrapper/Shader.h"

/**
 * Simple deferred shader which only filters and applies the SSAO and does FXAA
 */
inline Shader& getDeferredShader() {
    static Shader shader = { Shader::getBillboardVertexShader(), GLSL(
        layout(location = 0) out vec4 shadedPass;
        layout(location = 1) out float linearDistance;

        in vec2 TexCoords;
        uniform sampler2D gPosition;
        uniform sampler2D gNormal;
        uniform sampler2D gColor;
        uniform sampler2D ssaoPass; // The unblurred SSAO

        uniform float zNear;
        uniform float zFar;
        uniform int blur = 4;

        /**
         * Based on
         * https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/9.ssao/9.ssao_blur.fs
         */
        float blurredSSAO(float depth) {
            if (blur == 0) { // Skip the whole thing and return the unblurred ssao
                return texture(ssaoPass, TexCoords).r;
            }
            vec2 texelSize = 1.0 / vec2(textureSize(ssaoPass, 0));
            float result = 0.0;
            for (int x = -blur; x < blur; ++x) {
                for (int y = -blur; y < blur; ++y) {
                    vec2 offset = vec2(float(x), float(y)) * texelSize;
                    result += texture(ssaoPass, TexCoords + offset).r;
                }
            }
            int d = blur * 2;
            return result / (d * d);
        }

        void main() {

            /**
             * Apply FXAA since the supersampling is problematic in deferred renderers
             * based on https://github.com/mattdesl/glsl-fxaa
             */
            const float FXAA_SPAN_MAX = 8.0;
            const float FXAA_REDUCE_MUL = 1.0/8.0;
            const float FXAA_REDUCE_MIN = 1.0/128.0;

            vec2 frameBufSize = vec2(textureSize(gColor, 0));

            vec3 rgbNW=texture2D(gColor,TexCoords+(vec2(-1.0,-1.0)/frameBufSize)).xyz;
            vec3 rgbNE=texture2D(gColor,TexCoords+(vec2(1.0,-1.0)/frameBufSize)).xyz;
            vec3 rgbSW=texture2D(gColor,TexCoords+(vec2(-1.0,1.0)/frameBufSize)).xyz;
            vec3 rgbSE=texture2D(gColor,TexCoords+(vec2(1.0,1.0)/frameBufSize)).xyz;
            vec3 rgbM=texture2D(gColor,TexCoords).xyz;

            vec3 luma=vec3(0.299, 0.587, 0.114);
            float lumaNW = dot(rgbNW, luma);
            float lumaNE = dot(rgbNE, luma);
            float lumaSW = dot(rgbSW, luma);
            float lumaSE = dot(rgbSE, luma);
            float lumaM  = dot(rgbM,  luma);

            float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
            float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

            vec2 dir;
            dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
            dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

            float dirReduce = max(
                (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
                FXAA_REDUCE_MIN);

            float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);

            dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
                max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
                dir * rcpDirMin)) / frameBufSize;

            vec3 rgbA = (1.0/2.0) * (
                texture2D(gColor, TexCoords.xy + dir * (1.0/3.0 - 0.5)).xyz +
                texture2D(gColor, TexCoords.xy + dir * (2.0/3.0 - 0.5)).xyz);
            vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
                texture2D(gColor, TexCoords.xy + dir * (0.0/3.0 - 0.5)).xyz +
                texture2D(gColor, TexCoords.xy + dir * (3.0/3.0 - 0.5)).xyz);
            float lumaB = dot(rgbB, luma);

            if((lumaB < lumaMin) || (lumaB > lumaMax)){
                shadedPass.xyz=rgbA;
            } else {
                shadedPass.xyz=rgbB;
            }

            /**
             * Retrieve the depth from the gbuffer
             * and apply ambient occlusion
             */
            linearDistance = -texture(gPosition, TexCoords).z;

            if (linearDistance < zFar) {
                // No ssao for the sky
                shadedPass.rgb *= blurredSSAO(linearDistance);
            }
        }
    ), __FILE__ };
    return shader;
}