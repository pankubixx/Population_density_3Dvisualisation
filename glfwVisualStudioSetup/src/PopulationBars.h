#pragma once
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <unordered_map>

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
    void setLogScale(bool logScale);
    bool getLogScale() const { return logScale; }
    void setYear(int year);
    int getCurrentYear() const { return currentYear; }
    int minYear = 1900;
    int maxYear = 2100;
    std::pair<int, int> getYearRange() const { return {minYear, maxYear}; }
    std::unordered_map<int, std::vector<PopulationBarData>> yearToBars;
    int currentYear = 2025;
    std::vector<PopulationBarData> allBarsForYear; // All bars for the current year (public)
    void updateVisibleBars(const std::unordered_map<std::string, bool>& visibility);
private:
    std::vector<PopulationBarData> bars; // Only one bars vector, used everywhere
    std::vector<glm::mat4> instanceMatrices;
    std::vector<float> instanceHeights;
    GLuint vao = 0, vbo = 0, instanceVBO = 0, heightVBO = 0;
    GLuint shaderProgram = 0;
    bool initialized = false;
    float mapWidth = 1.0f, mapHeight = 1.0f, mapThickness = 0.01f;
    bool logScale = true;
    float globalMaxDensity = 0.0f;
    void createBarGeometry();
    bool createShaders();
}; 