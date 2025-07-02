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
#include "imguiThemes.h"
#include <unordered_map>
#include <set>

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

	// Apply a custom ImGui theme
	imguiThemes::embraceTheDarkness();

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

	// --- Country visibility state ---
	static std::unordered_map<std::string, bool> countryVisibility;
	static int lastYearForVisibility = -1;
	static std::vector<std::string> countryNames;
	// Helper to update country list and visibility when year changes
	auto updateCountryList = [&]() {
		countryNames.clear();
		std::set<std::string> uniqueNames;
		for (const auto& bar : g_populationBars->allBarsForYear) {
			if (uniqueNames.insert(bar.name).second) {
				countryNames.push_back(bar.name);
				// Only set to true if not already present (preserve user toggles)
				if (countryVisibility.find(bar.name) == countryVisibility.end()) {
					countryVisibility[bar.name] = true;
				}
			}
		}
	};
	if (lastYearForVisibility != selectedYear) {
		updateCountryList();
		lastYearForVisibility = selectedYear;
		g_populationBars->updateVisibleBars(countryVisibility);
	}

	static int lastAppliedYear = -1;
	if (lastAppliedYear != selectedYear) {
		g_populationBars->setYear(selectedYear);
		updateCountryList();
		lastAppliedYear = selectedYear;
	}
	g_populationBars->updateVisibleBars(countryVisibility);

	// Declare and assign all layout variables before use
	float topMargin = 10.0f;
	float verticalSpacingBetweenTextAndSlider = 8.0f;
	float bottomMargin = 6.0f;
	float textLineHeight = ImGui::GetTextLineHeight();
	float calculatedWindowHeight = 0.0f;
	calculatedWindowHeight += topMargin;
	calculatedWindowHeight += textLineHeight;
	calculatedWindowHeight += verticalSpacingBetweenTextAndSlider;
	calculatedWindowHeight += (ImGui::GetFontSize() + 2 * ImGui::GetStyle().FramePadding.y);
	calculatedWindowHeight += bottomMargin;

	while (!glfwWindowShouldClose(window))
	{
		static float timelapseYear = 0.0f;
		static double lastTime = 0.0;
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

		// Camera animation logic
		static float animationTime = 0.0f;
		static bool animateCamera = false;
		if (animateCamera) {
			animationTime += 0.003f; // Speed of animation
			float radius = 4.0f; // Distance from map center
			float height = 2.0f; // Height above map
			glm::vec3 mapCenter = glm::vec3(0.0f, 0.0f, 0.0f);
			camera.position = glm::vec3(
				radius * sin(animationTime),
				radius * cos(animationTime),
				height
			);
			// Look at map center
			glm::vec3 dir = glm::normalize(mapCenter - camera.position);
			camera.yaw = atan2(dir.x, dir.y);
			camera.pitch = asin(dir.z);
		} else if (!blockCamera) {
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
		ImGui::SetNextWindowPos(ImVec2(static_cast<float>(width) - 300.0f, 0.0f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(300.0f, static_cast<float>(height) - calculatedWindowHeight), ImGuiCond_Always);
		ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		bool logScale = g_populationBars->getLogScale();
		if (ImGui::Checkbox("Logarithmic scale", &logScale)) {
			g_populationBars->setLogScale(logScale);
		}
		ImGui::Checkbox("Animate camera around map", &animateCamera);
		static bool timelapse = false;
		ImGui::Checkbox("Timelapse year", &timelapse);
		if (ImGui::Button("Reset Camera") || glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			camera.reset();
		}
		ImGui::Text("Camera controls:");
		ImGui::BulletText("WASD: Move in view plane");
		ImGui::BulletText("Arrow keys: Rotate");
		ImGui::BulletText("Q/E: Up/Down");
		ImGui::BulletText("R: Reset camera");
		// --- Country checkboxes ---
		if (ImGui::CollapsingHeader("Country Visibility", ImGuiTreeNodeFlags_DefaultOpen)) {
			// Add Select All / Uncheck All buttons
			if (ImGui::Button("Select All")) {
				for (auto& kv : countryVisibility) kv.second = true;
				g_populationBars->updateVisibleBars(countryVisibility);
			}
			ImGui::SameLine();
			if (ImGui::Button("Uncheck All")) {
				for (auto& kv : countryVisibility) kv.second = false;
				g_populationBars->updateVisibleBars(countryVisibility);
			}
			ImGui::BeginChild("CountryList", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
			for (const auto& name : countryNames) {
				bool& visible = countryVisibility[name];
				ImGui::Checkbox(name.c_str(), &visible);
			}
			ImGui::EndChild();
		}
		ImGui::End();

		// Ensure bar visibility matches checkboxes every frame
		g_populationBars->updateVisibleBars(countryVisibility);

		// --- ImGui bottom bar for year slider ---
		ImGui::SetNextWindowPos(ImVec2(0.0f, static_cast<float>(height) - calculatedWindowHeight), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), calculatedWindowHeight), ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(34, 10));
		ImGui::PushFont(io.Fonts->Fonts[0]);
		ImGui::Begin("YearBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
		ImGui::SetCursorPosY(topMargin);
		ImGui::SetCursorPosX(40);
		ImGui::Text("Population Density Year");
		ImGui::SetCursorPosY(topMargin + textLineHeight + verticalSpacingBetweenTextAndSlider);
		ImGui::SetCursorPosX(40);
		ImGui::PushItemWidth(static_cast<float>(width - 160));
		sliderActive = ImGui::SliderInt("##YearSlider", &selectedYear, minYear, maxYear);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::SetCursorPosY(topMargin + textLineHeight + verticalSpacingBetweenTextAndSlider);
		ImGui::Text("%d", selectedYear);
		ImGui::End();
		ImGui::PopFont();
		ImGui::PopStyleVar();

		// Timelapse logic
		double currentTime = glfwGetTime();
		float timelapseSpeed = 5.0f; // years per second
		static bool prevTimelapse = false;
		if (timelapse && !prevTimelapse) {
			timelapseYear = static_cast<float>(minYear);
			selectedYear = minYear;
		}
		prevTimelapse = timelapse;
		if (timelapse && !sliderActive) {
			float delta = static_cast<float>(currentTime - lastTime);
			lastTime = currentTime;
			timelapseYear += timelapseSpeed * delta;
			if (timelapseYear > static_cast<float>(maxYear) + 0.999f) timelapseYear = static_cast<float>(minYear);
			selectedYear = static_cast<int>(timelapseYear);
		} else {
			lastTime = currentTime;
			timelapseYear = static_cast<float>(selectedYear);
		}

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

