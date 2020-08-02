#include <string>
#include <iostream>
#include <cmath>

#define TINYEXR_IMPLEMENTATION
#include "../../external/headeronly/tinyexr.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../external/headeronly/stb_image_write.h"

struct Color {
    unsigned char r = 0, g = 0, b = 0;
    Color() { }
    Color(float _r, float _g, float _b) {
        r = _r; g = _g; b = _b;
    }
};

struct ColorF {
    float r = 0, g = 0, b = 0;
    ColorF() { }
    ColorF(const Color& c) {
        r = c.r; g = c.g; b = c.b;
    }
    void add(const ColorF& c) {
        r += c.r; g += c.g; b += c.b;
    }
    void scale(const float s) {
        r *= s; g *= s; b *= s;
    }
    Color get() {
        return Color(r, g, b);
    }
};

float focus = 12.2;
float focusScale = 150;
float bias = 1;
float bias2 = 5;

float calcBlur(float depth) {
    return abs(((1.0 / focus) - (1.0 / depth))) * focusScale;
}


int main() {
    float *rgba = nullptr;
    int width, height;
    const char *err = nullptr;
    std::vector<float> depthBuffer;
    std::vector<Color> colorBuffer;
    std::vector<ColorF> result;
    std::vector<int> resultAdd;
    int ret;

    ret = LoadEXR(&rgba, &width, &height, "/home/usr/git/dreier/example/src/scatter/pos.exr", &err);
    if (ret != 0) { printf("err: %s\n", err); return -1; }
    depthBuffer.resize(size_t(width * height));
    for (size_t i = 0; i < size_t(width * height); i++) {
        depthBuffer[i] = -rgba[4 * i + 2];
    }
    free(rgba);

    ret = LoadEXR(&rgba, &width, &height, "/home/usr/git/dreier/example/src/scatter/color.exr", &err);
    if (ret != 0) { printf("err: %s\n", err); return -1; }
    colorBuffer.resize(size_t(width * height));
    for (size_t i = 0; i < size_t(width * height); i++) {
        colorBuffer[i] = {
            rgba[4 * i + 0] * 255.f,
            rgba[4 * i + 1] * 255.f,
            rgba[4 * i + 2] * 255.f
        };
    }
    free(rgba);

    result.resize(width * height);
    resultAdd.resize(width * height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float centerDepth = depthBuffer[y * width + x];
            float centerBlurSize = calcBlur(centerDepth);
            const ColorF& centerColor = colorBuffer[y * width + x];
            result[y * width + x].add(centerColor);
            resultAdd[y * width + x]++;
            for(int y1 = -centerBlurSize; y1 <= centerBlurSize; y1++) {
                for(int x1 = -centerBlurSize; x1 <= centerBlurSize; x1++) {
                    if(x1 * x1 + y1 * y1 >= centerBlurSize * centerBlurSize) {
                        continue;
                    }
                    int x2 = x + x1, y2 = y + y1;
                    if (x2 >= width || x2 < 0 || y2 >= height || y2 < 0) {
                        continue;
                    }
                    float sampleDepth = depthBuffer[y2 * width + x2];
                    if (calcBlur(sampleDepth) < bias2
                        && sampleDepth < centerDepth - bias)
                        {
                            continue;
                    }
                    result[y2 * width + x2].add(centerColor);
                    resultAdd[y2 * width + x2]++;
                }
            }
        }
    }

    for (size_t i = 0; i < size_t(width * height); i++) {
        ColorF& c = result[i];
        c.scale(1.f / resultAdd[i]);
        colorBuffer[i] = c.get();
    }


    ret = stbi_write_png(
        "/home/usr/git/dreier/example/src/scatter/out.png",
        width, height, 3, static_cast<const void*>(colorBuffer.data()), width * 3
    );
}
