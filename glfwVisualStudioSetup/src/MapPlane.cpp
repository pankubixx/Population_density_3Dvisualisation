#include "MapPlane.h"
#include <stb_image/stb_image.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

// Vertex structure for the box
struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

MapPlane::MapPlane(float width, float height, float thickness)
    : width(width), height(height), thickness(thickness) {}

MapPlane::~MapPlane() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
    if (texture) glDeleteTextures(1, &texture);
    if (shaderProgram) glDeleteProgram(shaderProgram);
}

bool MapPlane::loadTexture(const std::string& path) {
    int texWidth, texHeight, nrChannels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(path.c_str(), &texWidth, &texHeight, &nrChannels, 4);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return false;
    }
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return true;
}

bool MapPlane::initialize() {
    if (!createShaders()) return false;
    createBoxGeometry();
    initialized = true;
    return true;
}

void MapPlane::draw(const glm::mat4& viewProjMatrix) const {
    if (!initialized) return;
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uViewProj"), 1, GL_FALSE, glm::value_ptr(viewProjMatrix));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // Only top face
    glBindVertexArray(0);
    glUseProgram(0);
}

void MapPlane::createBoxGeometry() {
    // Only top face (z = thickness/2)
    float z = thickness * 0.5f;
    std::vector<Vertex> vertices = {
        {{-width/2, -height/2, z}, {0.0f, 0.0f}},
        {{ width/2, -height/2, z}, {1.0f, 0.0f}},
        {{ width/2,  height/2, z}, {1.0f, 1.0f}},
        {{-width/2,  height/2, z}, {0.0f, 1.0f}},
    };
    std::vector<unsigned int> indices = {0, 1, 2, 2, 3, 0};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glBindVertexArray(0);
}

static const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
uniform mat4 uViewProj;
out vec2 vTexCoord;
void main() {
    gl_Position = uViewProj * vec4(aPos, 1.0);
    vTexCoord = aTexCoord;
}
)";

static const char* fragmentShaderSrc = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
void main() {
    FragColor = texture(uTexture, vTexCoord);
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error: " << infoLog << std::endl;
        return 0;
    }
    return shader;
}

bool MapPlane::createShaders() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
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
        std::cerr << "Shader link error: " << infoLog << std::endl;
        return false;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return true;
} 