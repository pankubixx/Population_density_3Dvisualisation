# Mapa Cieplna Gęstości Zaludnienia
## Zaimplementowane funkcje
1. Wyświetlanie mapy

#include "Skybox.h"
#include <stb_image/stb_image.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Skybox::Skybox() {}
Skybox::~Skybox() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (texture) glDeleteTextures(1, &texture);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

bool Skybox::loadTexture(const std::string& path) {
    int texWidth, texHeight, nrChannels;
    stbi_set_flip_vertically_on_load(0);
    unsigned char* data = stbi_load(path.c_str(), &texWidth, &texHeight, &nrChannels, 4);
    if (!data) {
        std::cerr << "Failed to load background texture: " << path << std::endl;
        return false;
    }
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    this->texWidth = texWidth;
    this->texHeight = texHeight;
    return true;
}

bool Skybox::initialize() {
    // Fullscreen quad (NDC)
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
    return createShaders();
}

void Skybox::draw(const glm::mat4& viewMatrix) const {
    if (!initialized) return;
    glDepthMask(GL_FALSE);
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
    glUseProgram(0);
    glDepthMask(GL_TRUE);
}

static const char* quadVertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aPos; // Use NDC for direction
}
)";

static const char* quadFragmentShaderSrc = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
uniform mat4 uView;

void main() {
    // Reconstruct NDC direction (z = -1 for OpenGL camera looking down -Z)
    vec3 dir = normalize(vec3(vTexCoord.x, vTexCoord.y, -1.0));
    // Use the inverse rotation (transpose of upper-left 3x3 of view matrix)
    mat3 rot = transpose(mat3(uView));
    dir = rot * dir;
    // Equirectangular mapping
    float longitude = atan(dir.z, dir.x);
    float latitude = asin(clamp(dir.y, -1.0, 1.0));
    float u = 0.5 + longitude / (2.0 * 3.14159265);
    float v = 0.5 - latitude / 3.14159265;
    vec3 color = texture(uTexture, vec2(u, v)).rgb;
    color = pow(color, vec3(1.0/2.2));
    FragColor = vec4(color, 1.0);
}
)";

static GLuint compileQuadShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Quad shader compilation error: " << infoLog << std::endl;
        return 0;
    }
    return shader;
}

bool Skybox::createShaders() {
    GLuint vs = compileQuadShader(GL_VERTEX_SHADER, quadVertexShaderSrc);
    GLuint fs = compileQuadShader(GL_FRAGMENT_SHADER, quadFragmentShaderSrc);
    if (!vs || !fs) return false;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Quad shader link error: " << infoLog << std::endl;
        return false;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    initialized = true;
    return true;
} 

