#include "PopulationBars.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

bool PopulationBars::loadFromCSV(const std::string& path) {
    bars.clear();
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open CSV: " << path << std::endl;
        return false;
    }
    std::string line;
    std::getline(file, line); // skip header
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string name, code, year, density, x, y;
        std::getline(ss, name, ',');
        std::getline(ss, code, ',');
        std::getline(ss, year, ',');
        std::getline(ss, density, ',');
        std::getline(ss, x, ',');
        std::getline(ss, y, ',');
        PopulationBarData bar;
        bar.name = name;
        bar.density = std::stof(density);
        bar.x = std::stof(x);
        bar.y = std::stof(y);
        bars.push_back(bar);
    }
    return true;
}

bool PopulationBars::initialize(float mapWidth_, float mapHeight_, float mapThickness_) {
    mapWidth = mapWidth_;
    mapHeight = mapHeight_;
    mapThickness = mapThickness_;
    if (!createShaders()) return false;
    createBarGeometry();
    initialized = true;
    return true;
}

void PopulationBars::createBarGeometry() {
    // 8 vertices, 12 triangles (36 indices)
    float v[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 1.0f,
         0.5f, -0.5f, 1.0f,
         0.5f,  0.5f, 1.0f,
        -0.5f,  0.5f, 1.0f
    };
    unsigned int idx[] = {
        0,1,2, 2,3,0,
        4,5,6, 6,7,4,
        0,1,5, 5,4,0,
        1,2,6, 6,5,1,
        2,3,7, 7,6,2,
        3,0,4, 4,7,3
    };
    std::vector<float> vertices;
    for (int i = 0; i < 36; ++i) {
        int vi = idx[i];
        vertices.push_back(v[vi*3+0]);
        vertices.push_back(v[vi*3+1]);
        vertices.push_back(v[vi*3+2]);
    }
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    // Instance data: model matrix (position, scale)
    instanceMatrices.clear();
    instanceHeights.clear();
    float maxDensity = 0.0f;
    for (const auto& bar : bars) if (bar.density > maxDensity) maxDensity = bar.density;
    const float maxBarHeight = 1.5f;
    const float IMAGE_WIDTH = 4592.0f;
    const float IMAGE_HEIGHT = 3196.0f;
    for (const auto& bar : bars) {
        float px = (bar.x / IMAGE_WIDTH * mapWidth) - (mapWidth * 0.5f);
        float py = (mapHeight * 0.5f) - (bar.y / IMAGE_HEIGHT * mapHeight);
        float h = (std::log(bar.density + 1.0f) / std::log(maxDensity + 1.0f)) * maxBarHeight;
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(px, py, mapThickness/2.0f));
        model = glm::scale(model, glm::vec3(0.08f, 0.08f, h));
        // Remove: model = glm::translate(model, glm::vec3(0,0,0.5f));
        instanceMatrices.push_back(model);
        instanceHeights.push_back(h / maxBarHeight); // normalized height for color
    }
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size()*sizeof(glm::mat4), instanceMatrices.data(), GL_STATIC_DRAW);
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(1 + i);
        glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(float)*i*4));
        glVertexAttribDivisor(1 + i, 1);
    }
    glGenBuffers(1, &heightVBO);
    glBindBuffer(GL_ARRAY_BUFFER, heightVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceHeights.size()*sizeof(float), instanceHeights.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glVertexAttribDivisor(5, 1);
    glBindVertexArray(0);
}

// Vertex Shader
static const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in mat4 instanceModel;
layout(location = 5) in float instanceHeight; // Assuming instanceHeight is normalized (0.0 to 1.0)
uniform mat4 uViewProj;
out float vZ; // Pass model-space Z-coordinate (0.0 at bottom, 1.0 at top of bar)
out float vHeight; // Pass instanceHeight to fragment shader
void main() {
    gl_Position = uViewProj * instanceModel * vec4(aPos, 1.0);
    vZ = aPos.z; // Model-space Z-coordinate (0.0 at bottom, 1.0 at top of bar)
    vHeight = instanceHeight; // Pass the height of the current bar instance
}
)";

// Fragment Shader
static const char* fragmentShaderSrc = R"(
#version 330 core
in float vZ;
in float vHeight; // Height of the current bar instance
out vec4 FragColor;
void main() {
    // Define the global gradient colors for a more appealing look:
    // Transition from a light, desaturated blue to a rich, slightly muted orange-red.
    vec3 low = vec3(0.7, 0.85, 0.95); // Light, desaturated blue
    vec3 high = vec3(0.8, 0.3, 0.1);   // Rich, slightly muted orange-red

    // Calculate the effective mix factor for the gradient.
    // This scales the Z-coordinate by the bar's height.
    // - For short bars (vHeight close to 0), effectiveMix will be small,
    //   keeping the color closer to 'low' (light blue).
    // - For tall bars (vHeight close to 1), effectiveMix will be closer to vZ,
    //   allowing the full gradient spectrum to be used.
    float effectiveMix = vZ * vHeight;

    // Clamp the mix factor to ensure it stays within [0, 1]
    effectiveMix = clamp(effectiveMix, 0.0, 1.0);

    // Interpolate between the low and high colors using the effective mix factor
    vec3 color = mix(low, high, effectiveMix);

    FragColor = vec4(color, 1.0);
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

bool PopulationBars::createShaders() {
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

void PopulationBars::draw(const glm::mat4& viewProjMatrix, int hoveredBarIdx) const {
    if (!initialized) return;
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uViewProj"), 1, GL_FALSE, glm::value_ptr(viewProjMatrix));
    glDrawArraysInstanced(GL_TRIANGLES, 0, 36, (GLsizei)bars.size());
    glBindVertexArray(0);
    glUseProgram(0);
}

// Ray picking for bar selection
int PopulationBars::pickBar(float mouseX, float mouseY, const glm::mat4& view, const glm::mat4& proj, int screenWidth, int screenHeight) const {
    // Convert mouse to NDC
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;
    glm::vec4 rayClip(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
    glm::vec3 rayOrigin = glm::vec3(glm::inverse(view)[3]);
    // Test intersection with each bar's AABB
    for (int i = 0; i < (int)instanceMatrices.size(); ++i) {
        glm::mat4 model = instanceMatrices[i];
        glm::vec3 min = glm::vec3(model * glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f));
        glm::vec3 max = glm::vec3(model * glm::vec4(0.5f, 0.5f, 1.0f, 1.0f));
        // Ray-AABB intersection
        glm::vec3 t1 = (min - rayOrigin) / rayWorld;
        glm::vec3 t2 = (max - rayOrigin) / rayWorld;
        glm::vec3 tmin = glm::min(t1, t2);
        glm::vec3 tmax = glm::max(t1, t2);
        float tNear = std::max(std::max(tmin.x, tmin.y), tmin.z);
        float tFar = std::min(std::min(tmax.x, tmax.y), tmax.z);
        if (tNear < tFar && tFar > 0) return i;
    }
    return -1;
}

std::string PopulationBars::getBarName(int idx) const {
    if (idx < 0 || idx >= (int)bars.size()) return "";
    return bars[idx].name;
}

float PopulationBars::getBarDensity(int idx) const {
    if (idx < 0 || idx >= (int)bars.size()) return 0.0f;
    return bars[idx].density;
}

glm::vec2 PopulationBars::getBarScreenPos(int idx, const glm::mat4& viewProj, int screenWidth, int screenHeight) const {
    if (idx < 0 || idx >= (int)instanceMatrices.size()) return glm::vec2(0,0);
    glm::vec4 pos = viewProj * instanceMatrices[idx] * glm::vec4(0,0,1,1);
    pos /= pos.w;
    float x = (pos.x * 0.5f + 0.5f) * screenWidth;
    float y = (1.0f - (pos.y * 0.5f + 0.5f)) * screenHeight;
    return glm::vec2(x, y);
} 