#pragma once
#include <string>

/**
 * A macro to disable all kinds of implicit copy mechanisms
 */
#define NO_COPY(name) \
name(const name&) = delete; \
name(const name*) = delete; \
name(name&&) = delete; \
name& operator= (const name&) = delete; \
name& operator= (name&&) = delete;

template <typename T>
inline T clamp(const T v, const T min, const T max) {
    return std::max(min, std::min(v, max));
}

inline std::string platformPath(std::string path) {
    if (path.find(":/") != std::string::npos || path.find('/') == 0) {
        return path; // Don't touch absolute paths
    }
    #ifdef ASSET_PATH
        return ASSET_PATH + path;
    #endif
    return path;
}

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

inline void CheckOpenGLError(const char* stmt, const char* fname, int line) {
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "OpenGL error " << err << " in file " << fname << ":" << line << "\n" << stmt << "\n";
    }
}

// helper macro that checks for GL errors.
#ifdef NDEBUG
	#define GLC(call) call
#else
	#define GLC(call) call; CheckOpenGLError(#call, __FILE__, __LINE__)
#endif

