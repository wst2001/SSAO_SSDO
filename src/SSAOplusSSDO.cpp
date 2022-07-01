#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "includes/shader.h"
#include "includes/camera.h"
#include "includes/model.h"

#include <iostream>
#include <random>
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void renderQuad();
void renderCube();
void setImgui(GLFWwindow* window);
const float M_PI = glm::acos(-1);

// settings
const unsigned int SCR_WIDTH = 1333;
const unsigned int SCR_HEIGHT = 800;

bool MC_mode = 0;

//模型参数
float modelXYZ[3] = { 0.0f, 0.2f, 0.0f };
float modelAngle = 0;

//光照参数
float lightXYZ[3] = { 2.0f, 4.0f, 2.0f };
float lightColor[3] = { 1.0f, 1.0f, 1.0f };

//SSDO参数
int kernelSize = 64;
float radius = 0.5f;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 7.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

unsigned int loadCubemap(vector<std::string> faces);
static const std::vector<std::string> SKYBOX_TEXTURE = {
	"../resources/textures/skybox/right.jpg",
	"../resources/textures/skybox/left.jpg",
	"../resources/textures/skybox/top.jpg",
	"../resources/textures/skybox/bottom.jpg",
	"../resources/textures/skybox/back.jpg",
	"../resources/textures/skybox/front.jpg",
};

int main()
{
	// GLFW初始化
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW window
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SSDO", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);


	// GLAD初始化
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// OpenGL渲染设置
	// -----------------------------
	glEnable(GL_DEPTH_TEST); // 启动深度测试

	// ImGui初始化
	// -----------------------------	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui::StyleColorsClassic();
	ImGui_ImplOpenGL3_Init("#version 330");

	// 编译shader
	// -------------------------
	Shader GeometryShader("../GLSL/geometry.vs", "../GLSL/geometry.fs");
	Shader LightingShader("../GLSL/ssdo_pass.vs", "../GLSL/ssdo_lighting.fs");
	Shader DirectShader("../GLSL/ssdo_pass.vs", "../GLSL/ssdo_direct.fs");
	Shader DirectBlurShader("../GLSL/ssdo_pass.vs", "../GLSL/ssdo_blur.fs");
	Shader IndirectShader("../GLSL/ssdo_pass.vs", "../GLSL/ssdo_indirect.fs");
	Shader IndirectBlurShader("../GLSL/ssdo_pass.vs", "../GLSL/ssdo_blur.fs");
	Shader MixerShader("../GLSL/ssdo_pass.vs", "../GLSL/ssdo_mixer.fs");
	Shader SkyboxShader("../GLSL/ssdo_skybox.vs", "../GLSL/ssdo_skybox.fs");
	// 载入模型
	// -----------
	Model backpack("../resources/objects/dragon.obj");

	// 设置Gbuffer
	// ------------------------------
	unsigned int gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	unsigned int gPositionDepth, gNormal, gAlbedo;
	// 位置+深度buffer
	glGenTextures(1, &gPositionDepth);
	glBindTexture(GL_TEXTURE_2D, gPositionDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPositionDepth, 0);
	// 法线buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
	// 颜色buffer
	glGenTextures(1, &gAlbedo);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
	// Color attachment
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);
	// 深度buffer
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	// 检查buffer是否完整
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// SSDO buffer
	// -----------------------------------------------------
	unsigned int
		ssdoFBO, ssdoBlurFBO, ssdoLightingFBO, ssdoIndirectFBO, ssdoIndirectBlurFBO, skyboxFBO,
		ssdoTex, ssdoBlurTex, ssdoLightingTex, ssdoIndirectTex, ssdoIndirectBlurTex, skyboxTex;
	unsigned int* FBOs[] = {
		&ssdoFBO,
		&ssdoBlurFBO,
		&ssdoLightingFBO,
		&ssdoIndirectFBO,
		&ssdoIndirectBlurFBO,
		&skyboxFBO,
	};
	unsigned int* Texs[] = {
		&ssdoTex,
		&ssdoBlurTex,
		&ssdoLightingTex,
		&ssdoIndirectTex,
		&ssdoIndirectBlurTex,
		&skyboxTex,
	};
	for (int i = 0; i < 6; i++) {
		auto FBO = FBOs[i];
		auto Tex = Texs[i];
		glGenFramebuffers(1, FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, *FBO);
		glGenTextures(1, Tex);
		glBindTexture(GL_TEXTURE_2D, *Tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *Tex, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 生成采样核心
	// ----------------------
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); 
	std::default_random_engine generator;
	std::vector<glm::vec3> ssdoKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0f;

		// 调整采样点，使其更靠近中心
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssdoKernel.push_back(sample);
	}

	// 生成噪音texture
	// ----------------------
	std::vector<glm::vec3> ssdoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
		ssdoNoise.push_back(noise);
	}
	unsigned int noiseTexture;
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssdoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	unsigned int skyboxMap = loadCubemap(SKYBOX_TEXTURE);

	// shader 配置
	// --------------------
	LightingShader.use();
	LightingShader.setInt("gPositionDepth", 0);
	LightingShader.setInt("gNormal", 1);
	LightingShader.setInt("gAlbedo", 2);

	DirectShader.use();
	DirectShader.setInt("gPositionDepth", 0);
	DirectShader.setInt("gNormal", 1);
	DirectShader.setInt("texNoise", 2);
	DirectShader.setInt("skybox", 3);
	for (unsigned int i = 0; i < 64; ++i)
		DirectShader.setVec3("samples[" + std::to_string(i) + "]", ssdoKernel[i]);

	DirectBlurShader.use();
	DirectBlurShader.setInt("tex", 0);

	IndirectShader.use();
	IndirectShader.setInt("gPositionDepth", 0);
	IndirectShader.setInt("gNormal", 1);
	IndirectShader.setInt("texNoise", 2);
	IndirectShader.setInt("texLighting", 3);
	for (unsigned int i = 0; i < 64; ++i)
		IndirectShader.setVec3("samples[" + std::to_string(i) + "]", ssdoKernel[i]);

	IndirectBlurShader.use();
	IndirectBlurShader.setInt("tex", 0);

	SkyboxShader.use();
	SkyboxShader.setInt("skybox", 0);

	MixerShader.use();
	MixerShader.setInt("gPositionDepth", 0);
	MixerShader.setInt("gNormal", 1);
	MixerShader.setInt("ssdo", 2);
	MixerShader.setInt("ssdoBlur", 3);
	MixerShader.setInt("texLighting", 4);
	MixerShader.setInt("texIndirectLight", 5);
	MixerShader.setInt("texIndirectLightBlur", 6);
	MixerShader.setInt("texSkybox", 7);
	// 渲染循环
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		if (MC_mode)
			glfwSetCursorPosCallback(window, mouse_callback);
		else
			glfwSetCursorPosCallback(window, NULL);

		processInput(window);


		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 1. 几何渲染
		// -----------------------------------------------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 50.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
		GeometryShader.use();
		GeometryShader.setMat4("projection", projection);
		GeometryShader.setMat4("view", view);
		// 地面
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0, -0.5f, 0.0f));
		model = glm::scale(model, glm::vec3(2.0f));
		model = glm::rotate(model, M_PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		GeometryShader.setMat4("model", model);
		renderQuad();

		// 地面上的模型
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.2f, 0.0));
		model = glm::rotate(model, glm::radians(modelAngle), glm::vec3(1.0, 0.0, 0.0));
		model = glm::scale(model, glm::vec3(1.0f));
		GeometryShader.setMat4("model", model);
		backpack.Draw(GeometryShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 2. Blinn-Phong光照模型
		// -----------------------------------------------------------------------------------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, ssdoLightingFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		LightingShader.use();
		// send light relevant uniforms
		glm::vec3 lightPosView = glm::vec3(camera.GetViewMatrix() * glm::vec4(lightXYZ[0], lightXYZ[1], lightXYZ[2], 1.0));
		LightingShader.setVec3("light.Position", lightPosView);
		LightingShader.setVec3("light.Color", glm::vec3(lightColor[0], lightColor[1], lightColor[2]));
		// Update attenuation parameters
		const float linear = 0.09f;
		const float quadratic = 0.032f;
		LightingShader.setFloat("light.Linear", linear);
		LightingShader.setFloat("light.Quadratic", quadratic);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPositionDepth);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 3. SSDO直接光照
		// ------------------------
		glBindFramebuffer(GL_FRAMEBUFFER, ssdoFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		DirectShader.use();
		DirectShader.setMat4("projection", projection);
		DirectShader.setMat4("iview", glm::inverse(view));
		DirectShader.setInt("kernelSize", kernelSize);
		DirectShader.setFloat("radius", radius);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPositionDepth);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxMap);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 4. SSDO直接光照模糊
		glBindFramebuffer(GL_FRAMEBUFFER, ssdoBlurFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		DirectBlurShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssdoTex);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 5. SSDO间接光照
		glBindFramebuffer(GL_FRAMEBUFFER, ssdoIndirectFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		IndirectShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPositionDepth);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, ssdoLightingTex);
		IndirectShader.setMat4("projection", projection);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		// 6. SSDO间接光照模糊
		glBindFramebuffer(GL_FRAMEBUFFER, ssdoIndirectBlurFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		IndirectBlurShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssdoIndirectTex);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 7. 天空盒
		glBindFramebuffer(GL_FRAMEBUFFER, skyboxFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
		SkyboxShader.use();
		auto viewM2 = glm::mat4(glm::mat3(view)); // no translation
		SkyboxShader.setMat4("view", viewM2);
		SkyboxShader.setMat4("projection", projection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxMap);
		renderCube();
		glDepthFunc(GL_LESS);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 8. 组合所有光照
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		MixerShader.use();
		MixerShader.setInt("mode", 8);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPositionDepth);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, ssdoTex);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, ssdoBlurTex);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, ssdoLightingTex);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, ssdoIndirectTex);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, ssdoIndirectBlurTex);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, skyboxTex);
		renderQuad();

		// Imgui控制面板
		setImgui(window);


		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}


unsigned int loadCubemap(vector<std::string> faces) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrComponents;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else {
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void setImgui(GLFWwindow* window)
{
	bool Imgui;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();
	ImGui::Begin("Settings", &Imgui, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);

	ImGui::Text("SSDO Settings:");
	ImGui::SliderInt("Size of Kernel", &kernelSize, 1, 500);
	ImGui::SliderFloat("Radius", &radius, 0.0f, 3.0f, "%.1f");

	ImGui::Text("Light Settings:");
	ImGui::Text("Position:");
	ImGui::SliderFloat("Light Position.x", &lightXYZ[0], -10.0f, 10.0f, "%.3f");
	ImGui::SliderFloat("Light Position.y", &lightXYZ[1], -10.0f, 10.0f, "%.3f");
	ImGui::SliderFloat("Light Position.z", &lightXYZ[2], -10.0f, 10.0f, "%.3f");
	ImGui::Text("Color:");
	ImGui::ColorEdit3("Light Color", lightColor);

	ImGui::Text("Model Settings:");
	ImGui::SliderFloat("Model Angel", &modelAngle, -180.0f, 180.0f, "angle = %.1f");

	ImGui::BeginMenuBar();
	if (ImGui::BeginMenu("Options") == 1)
	{
		if (ImGui::MenuItem("MC Mode", NULL))
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			MC_mode = 1;
		}
		if (ImGui::MenuItem("Quit", NULL))
			exit(0);
		ImGui::EndMenu();
	}
	ImGui::EndMenuBar();

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}