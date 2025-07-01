#pragma once
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct PopulationBarData {
    std::string name;
    float density;
    float x, y; // image-relative coordinates
};

class PopulationBars {
public:
    bool loadFromCSV(const std::string& path);
    bool initialize(float mapWidth, float mapHeight, float mapThickness);
    void draw(const glm::mat4& viewProjMatrix, int hoveredBarIdx = -1) const;
    int pickBar(float mouseX, float mouseY, const glm::mat4& view, const glm::mat4& proj, int screenWidth, int screenHeight) const;
    std::string getBarName(int idx) const;
    float getBarDensity(int idx) const;
    glm::vec2 getBarScreenPos(int idx, const glm::mat4& viewProj, int screenWidth, int screenHeight) const;
    int getBarCount() const { return (int)bars.size(); }
private:
    std::vector<PopulationBarData> bars;
    std::vector<glm::mat4> instanceMatrices;
    std::vector<float> instanceHeights;
    GLuint vao = 0, vbo = 0, instanceVBO = 0, heightVBO = 0;
    GLuint shaderProgram = 0;
    bool initialized = false;
    float mapWidth = 1.0f, mapHeight = 1.0f, mapThickness = 0.01f;
    void createBarGeometry();
    bool createShaders();
}; 