#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Class responsible for rendering a flat box (plane) with a texture on the top face
class MapPlane {
public:
    // Constructor: takes width, height, and thickness (very small)
    MapPlane(float width, float height, float thickness = 0.01f);
    ~MapPlane();

    // Loads the texture from file (returns true on success)
    bool loadTexture(const std::string& path);

    // Initializes OpenGL buffers and shaders
    bool initialize();

    // Renders the map plane
    void draw(const glm::mat4& viewProjMatrix) const;

    // Returns aspect ratio (width/height)
    float getAspectRatio() const { return width / height; }

private:
    float width, height, thickness;
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint texture = 0;
    GLuint shaderProgram = 0;
    bool initialized = false;

    // Helper to create geometry
    void createBoxGeometry();
    // Helper to load and compile shaders
    bool createShaders();
}; 