#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <gl2d/gl2d.h>
#include <openglErrorReporting.h>

#include "MapPlane.h"
#include <glm/gtc/matrix_transform.hpp>

static void error_callback(int error, const char *description)
{
	std::cout << "Error: " << description << "\n";
}

// --- MapPlane instance ---
// Map proportions from map.png: 4592x3196
constexpr float MAP_WIDTH = 4.592f;
constexpr float MAP_HEIGHT = 3.196f;
constexpr float MAP_THICKNESS = 0.02f;
MapPlane* g_mapPlane = nullptr;

int main(void)
{

	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

	GLFWwindow *window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	enableReportGlErrors();

	//glfwSwapInterval(1); //vsync


	gl2d::init();
	gl2d::Renderer2D renderer;
	renderer.create();

	// Create map plane
	g_mapPlane = new MapPlane(MAP_WIDTH, MAP_HEIGHT, MAP_THICKNESS);
	if (!g_mapPlane->loadTexture("assets/map.png")) {
		fprintf(stderr, "Failed to load map texture!\n");
		return -1;
	}
	if (!g_mapPlane->initialize()) {
		fprintf(stderr, "Failed to initialize map plane!\n");
		return -1;
	}

	while (!glfwWindowShouldClose(window))
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);



		renderer.updateWindowMetrics(width, height);

		glad_glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		static glm::vec3 color = {1,0,0};

		// Camera setup: look at the map from an angle
		glm::mat4 view = glm::lookAt(
			glm::vec3(0, -4, 2), // Camera position
			glm::vec3(0, 0, 0),  // Look at center
			glm::vec3(0, 0, 1)   // Up vector
		);
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width/height, 0.1f, 100.0f);
		glm::mat4 viewProj = proj * view;
		// Render map
		g_mapPlane->draw(viewProj);

		renderer.flush();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();

	delete g_mapPlane;

	return 0;
}

