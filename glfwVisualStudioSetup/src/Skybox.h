#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Class responsible for rendering a panoramic sky background
class Skybox {
public:
    Skybox();
    ~Skybox();

    // Loads the background texture from file (returns true on success)
    bool loadTexture(const std::string& path);

    // Initializes OpenGL buffers and shaders
    bool initialize();

    // Renders the panoramic background using the camera view matrix
    void draw(const glm::mat4& viewMatrix) const;

private:
    GLuint vao = 0, vbo = 0;
    GLuint texture = 0;
    GLuint shaderProgram = 0;
    bool initialized = false;
    int texWidth = 0, texHeight = 0;

    bool createShaders();
}; 