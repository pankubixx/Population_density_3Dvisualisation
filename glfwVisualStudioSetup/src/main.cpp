#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "MapPlane.h"
#include "PopulationBars.h"
// ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

static void error_callback(int error, const char *description)
{
	std::cout << "Error: " << description << "\n";
}

constexpr float MAP_WIDTH = 4.592f;
constexpr float MAP_HEIGHT = 3.196f;
constexpr float MAP_THICKNESS = 0.02f;
MapPlane* g_mapPlane = nullptr;
PopulationBars* g_populationBars = nullptr;

int main()
{
	glfwSetErrorCallback(error_callback);
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	GLFWwindow *window = glfwCreateWindow(1280, 800, "Population Density Map", NULL, NULL);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { glfwTerminate(); return -1; }

	// ImGui setup
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	// Map and bars
	g_mapPlane = new MapPlane(MAP_WIDTH, MAP_HEIGHT, MAP_THICKNESS);
	if (!g_mapPlane->loadTexture("assets/map.png") || !g_mapPlane->initialize()) {
		std::cerr << "Failed to load or initialize map plane!\n";
		return -1;
	}
	g_populationBars = new PopulationBars();
	if (!g_populationBars->loadFromCSV("dataset/data_2025.csv") ||
		!g_populationBars->initialize(MAP_WIDTH, MAP_HEIGHT, MAP_THICKNESS)) {
		std::cerr << "Failed to load or initialize population bars!\n";
		return -1;
	}

	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		// Camera
		glm::mat4 view = glm::lookAt(
			glm::vec3(0, -4, 2), // Camera position
			glm::vec3(0, 0, 0),  // Look at center
			glm::vec3(0, 0, 1)   // Up vector
		);
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width/height, 0.1f, 100.0f);
		glm::mat4 viewProj = proj * view;

		// --- ImGui frame start ---
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// --- Picking ---
		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);
		int hoveredBar = g_populationBars->pickBar((float)mouseX, (float)mouseY, view, proj, width, height);

		// --- Render scene ---
		g_mapPlane->draw(viewProj);
		g_populationBars->draw(viewProj, hoveredBar);

		// --- Tooltip ---
		if (hoveredBar >= 0) {
			std::string label = g_populationBars->getBarName(hoveredBar) + "\nDensity: " + std::to_string(g_populationBars->getBarDensity(hoveredBar));
			ImGui::SetNextWindowBgAlpha(0.8f);
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(label.c_str());
			ImGui::EndTooltip();
		}

		// --- ImGui render ---
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	delete g_mapPlane;
	delete g_populationBars;
	glfwTerminate();
	return 0;
}

