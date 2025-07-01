# Mapa Cieplna Gęstości Zaludnienia
## Zaimplementowane funkcje
1. Wyświetlanie mapy

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include "MapPlane.h"
#include "PopulationBars.h"
// ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "Skybox.h"

static void error_callback(int error, const char *description)
{
	std::cout << "Error: " << description << "\n";
}

constexpr float MAP_WIDTH = 4.592f;
constexpr float MAP_HEIGHT = 3.196f;
constexpr float MAP_THICKNESS = 0.02f;
MapPlane* g_mapPlane = nullptr;
PopulationBars* g_populationBars = nullptr;
Skybox skybox;

struct CameraState {
	glm::vec3 position = glm::vec3(0, -4, 2);
	float yaw = 0.0f;   // left/right (in radians)
	float pitch = -0.4f; // up/down (in radians)
	glm::vec3 up = glm::vec3(0, 0, 1);
	void reset() {
		position = glm::vec3(0, -4, 2);
		yaw = 0.0f;
		pitch = -0.4f;
	}
};

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
	if (!g_populationBars->loadFromCSV("dataset/dataset.csv") ||
		!g_populationBars->initialize(MAP_WIDTH, MAP_HEIGHT, MAP_THICKNESS)) {
		std::cerr << "Failed to load or initialize population bars!\n";
		return -1;
	}

	// Initialize skybox
	if (!skybox.loadTexture("assets/skybox.jpg") || !skybox.initialize()) {
		std::cerr << "Failed to initialize skybox!" << std::endl;
		return -1;
	}

	CameraState camera;
	static int selectedYear = 2025;
	static bool sliderActive = false;
	int minYear = g_populationBars->minYear;
	int maxYear = g_populationBars->maxYear;

	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		// --- Camera controls ---
		bool blockCamera = sliderActive;
		float moveSpeed = 0.05f;
		float rotSpeed = 0.02f;
		glm::vec3 forward = glm::normalize(glm::vec3(
			cos(camera.pitch) * sin(camera.yaw),
			cos(camera.pitch) * cos(camera.yaw),
			sin(camera.pitch)
		));
		glm::vec3 right = glm::normalize(glm::cross(forward, camera.up));
		glm::vec3 upMove = camera.up;
		if (!blockCamera) {
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.position += moveSpeed * glm::vec3(forward.x, forward.y, 0.0f);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.position -= moveSpeed * glm::vec3(forward.x, forward.y, 0.0f);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.position -= moveSpeed * right;
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.position += moveSpeed * right;
			if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.position += moveSpeed * upMove;
			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.position -= moveSpeed * upMove;
			if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) camera.reset();
			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.pitch += rotSpeed;
			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.pitch -= rotSpeed;
			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.yaw -= rotSpeed;
			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.yaw += rotSpeed;
		}
		// Clamp pitch
		camera.pitch = glm::clamp(camera.pitch, -glm::half_pi<float>() + 0.1f, glm::half_pi<float>() - 0.1f);
		// Calculate direction
		glm::vec3 dir = glm::vec3(
			cos(camera.pitch) * sin(camera.yaw),
			cos(camera.pitch) * cos(camera.yaw),
			sin(camera.pitch)
		);
		glm::vec3 target = camera.position + dir;
		glm::mat4 view = glm::lookAt(camera.position, target, camera.up);
		glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width/height, 0.1f, 100.0f);
		glm::mat4 viewProj = proj * view;

		// Remove translation from view matrix for skybox
		glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
		glm::mat4 skyboxVP = proj * viewNoTrans;
		skybox.draw(viewNoTrans);

		// --- ImGui frame start ---
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// --- ImGui sidebar on the right ---
		ImGui::SetNextWindowPos(ImVec2(width - 300, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(300, (float)height), ImGuiCond_Always);
		ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		bool logScale = g_populationBars->getLogScale();
		if (ImGui::Checkbox("Logarithmic scale", &logScale)) {
			g_populationBars->setLogScale(logScale);
		}
		if (ImGui::Button("Reset Camera") || glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			camera.reset();
		}
		ImGui::Text("Camera controls:");
		ImGui::BulletText("WASD: Move in view plane");
		ImGui::BulletText("Arrow keys or Mouse drag: Rotate");
		ImGui::BulletText("Q/E: Up/Down");
		ImGui::BulletText("R: Reset camera");
		ImGui::End();

		// --- ImGui bottom bar for year slider ---
		ImGui::SetNextWindowPos(ImVec2(0, height - 100), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2((float)width, 100), ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(24, 32));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(60, 80, 180, 255));
		ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL32(255, 180, 60, 255));
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(255, 220, 100, 255));
		ImGui::PushFont(io.Fonts->Fonts[0]);
		ImGui::Begin("YearBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
		ImGui::SetCursorPosY(32);
		ImGui::SetCursorPosX(40);
		ImGui::Text("Population Density Year");
		ImGui::SetCursorPosY(60);
		ImGui::SetCursorPosX(40);
		ImGui::PushItemWidth(width - 160);
		sliderActive = ImGui::SliderInt("##YearSlider", &selectedYear, minYear, maxYear);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::SetCursorPosY(60);
		ImGui::Text("%d", selectedYear);
		ImGui::End();
		ImGui::PopFont();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
		g_populationBars->setYear(selectedYear);

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

