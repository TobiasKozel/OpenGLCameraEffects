#pragma once
#include "glad/glad.h"
#include <glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

#include "Texture.h"
#include "FrameBufferObject.h"

#define GLSL(shader)  "#version 330 core\n" #shader

class Shader {
    GLuint sId;

public:
    NO_COPY(Shader)

    Shader(std::string vertexCode, std::string fragmentCode, std::string file = "") { 	
        if (vertexCode.find(".vert") != std::string::npos) {
            vertexCode = readFile(vertexCode);
        }
        if (fragmentCode.find(".frag") != std::string::npos) {
            fragmentCode = readFile(fragmentCode);
        }
        
        sId = glCreateProgram();

        auto loadShader = [&](const std::string& shader, unsigned int type) {
            const unsigned int sid = glCreateShader(type);
            const char* shaderChar = shader.c_str();
            glShaderSource(sid, 1, &shaderChar, nullptr);
            glCompileShader(sid);
            checkCompileErrors(sid, std::to_string(type), file);
            glAttachShader(sId, sid);
            glDeleteShader(sid);
        };

        loadShader(vertexCode, GL_VERTEX_SHADER);
        loadShader(fragmentCode, GL_FRAGMENT_SHADER);

        glLinkProgram(sId);
        
        if (!checkCompileErrors(sId, "PROGRAM", file)) { return; }
    }

    void use(const Textures &textures) const {
        use(&textures);
    }

    /**
     * Use the shaders and make the textures usable in the shader if needed
     */
    void use(const Textures* textures = nullptr) const {
        /**
         * The currently active program
         * Used to avoid switching shader if not needed
         */
        static unsigned int currentId = 0;
        if (currentId != sId) {
            glUseProgram(sId);
            currentId = sId;
        }
        
        if (textures != nullptr) {
            int index = 0;
            for (auto &i : *textures) {
                std::string &name = i->getName();
                if (!name.empty()) {
                    setInt(name, index);
                    glActiveTexture(GL_TEXTURE0 + (index++));
                }
                i->use();
            }
        }
    }

    GLuint getId() const { return sId; }
    
    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(sId, name.c_str()), int(value)); 
    }
    
    void setInt(const std::string &name, int value) const { 
        glUniform1i(glGetUniformLocation(sId, name.c_str()), value); 
    }
    
    void setFloat(const std::string &name, float value) const { 
        glUniform1f(glGetUniformLocation(sId, name.c_str()), value); 
    }
    
    void setVec2(const std::string &name, const glm::vec2 &value) const { 
        glUniform2fv(glGetUniformLocation(sId, name.c_str()), 1, &value[0]); 
    }
    void setVec2(const std::string &name, float x, float y) const { 
        glUniform2f(glGetUniformLocation(sId, name.c_str()), x, y); 
    }
    
    void setVec3(const std::string &name, const glm::vec3 &value) const { 
        glUniform3fv(glGetUniformLocation(sId, name.c_str()), 1, &value[0]); 
    }
    
    void setVec3(const std::string &name, float x, float y, float z) const { 
        glUniform3f(glGetUniformLocation(sId, name.c_str()), x, y, z); 
    }
    
    void setVec4(const std::string &name, const glm::vec4 &value) const {
        glUniform4fv(glGetUniformLocation(sId, name.c_str()), 1, &value[0]); 
    }
    
    void setVec4(const std::string &name, float x, float y, float z, float w) const { 
        glUniform4f(glGetUniformLocation(sId, name.c_str()), x, y, z, w); 
    }
    
    void setMat2(const std::string &name, const glm::mat2 &mat) const {
        glUniformMatrix2fv(glGetUniformLocation(sId, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    
    void setMat3(const std::string &name, const glm::mat3 &mat) const {
        glUniformMatrix3fv(glGetUniformLocation(sId, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    
    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        glUniformMatrix4fv(glGetUniformLocation(sId, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    static std::string getBillboardVertexShader() {
        return GLSL(
            layout(location = 0) in vec2 aPos;
            layout(location = 1) in vec2 aTexCoords;
            out vec2 TexCoords;
            void main() {
                TexCoords = aTexCoords;
                gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
            }
        );
    }

private:
    static std::string readFile(const std::string& path) {
        try {
            std::ifstream file;
            file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            file.open(path);
            std::stringstream stream;
            stream << file.rdbuf();
            file.close();
            return stream.str();
        }
        catch (std::ifstream::failure& e) {
            std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n";
        }
        return std::string();
    }
    
    static bool checkCompileErrors(GLuint shader, const std::string &type, std::string file = "") {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (success) { return true; }
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (success) { return true; }
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
        }
        std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n";
        std::cerr << infoLog << " " << file;
        std::cerr << "\n -- --------------------------------------------------- -- \n";
        return false;
    }
};
