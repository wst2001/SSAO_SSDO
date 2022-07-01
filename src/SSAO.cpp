#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
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
unsigned int loadTexture(const char* path);
void setImgui(GLFWwindow* window);
void renderQuad();
void renderCube();
unsigned int loadCubemap(vector<std::string> faces);
static const std::vector<std::string> SKYBOX_TEXTURE = {
	"../resources/textures/skybox/right.jpg",
	"../resources/textures/skybox/left.jpg",
	"../resources/textures/skybox/top.jpg",
	"../resources/textures/skybox/bottom.jpg",
	"../resources/textures/skybox/back.jpg",
	"../resources/textures/skybox/front.jpg",
};
const float M_PI = glm::acos(-1);


// 设置屏幕参数
const unsigned int SCR_WIDTH = 1333;
const unsigned int SCR_HEIGHT = 800;

// 相机参数
Camera camera(glm::vec3(0.0f, 0.0f, 7.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// 时间参数
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//模式参数
bool MC_mode = 0;

//模型参数
int modelType = 0;
int previosType = 0;
float modelXYZ[3] = { 0.0f, 0.2f, 0.0f };
float modelAngle1 = 0, modelAngle2 = 0, modelAngle3 = 0;
float modelScale[13] = { 1.0,1.2,1.0,0.05,10.0,0.02,0.008,1.0,0.01,1.5 };
float modelPosition[13] = { 0.2 ,0.7,0.5,0.3,-0.85,0.6,0,0.35,-0.45,0 };

//光照参数
float lightXYZ[3] = { 2.0f, 4.0f, 2.0f };
float lightColor[3] = { 1.0f, 1.0f, 1.0f };

//SSAO参数
int kernelSize = 64;
float radius = 0.5f;
float bias = 0.025f;

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SSAO", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);


	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui::StyleColorsClassic();
	ImGui_ImplOpenGL3_Init("#version 330");

	Shader shaderGeometryPass("../GLSL/geometry.vs", "../GLSL/geometry.fs");
	Shader shaderLightingPass("../GLSL/ssao.vs", "../GLSL/ssao_lighting.fs");
	Shader shaderSSAO("../GLSL/ssao.vs", "../GLSL/ssao.fs");
	Shader shaderSSAOBlur("../GLSL/ssao.vs", "../GLSL/ssao_blur.fs");
	Shader SkyboxShader("../GLSL/skybox.vs", "../GLSL/skybox.fs");

	Model models("../resources/objects/dragon.obj");


	// 初始化一个帧缓冲g-buffer对象----------------------------------------------------------------------
	unsigned int gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	unsigned int gPosition, gNormal, gAlbedo;

	// - 位置缓冲
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
	// - 法线缓冲
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
	// - 颜色 + 镜面缓冲
	glGenTextures(1, &gAlbedo);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
	// - 告诉OpenGL我们将要使用(帧缓冲的)哪种颜色附件来进行渲染
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	// 添加渲染缓冲对象为深度缓冲
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	// 检查完整性
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//------------------------------------------------------------------------------------------------------


	// 创建SSAO帧缓冲对象-----------------------------------------------------------------------------------
	unsigned int 
		ssaoFBO, ssaoBlurFBO, skyboxFBO,
		ssaoTex, ssaoBlurTex, skyboxTex;

	unsigned int* FBOs[] = {
		&ssaoFBO,
		&ssaoBlurFBO,
		&skyboxFBO,
	};
	unsigned int* Texs[] = {
		&ssaoTex,
		&ssaoBlurTex,
		&skyboxTex,
	};
	for (int i = 0; i < 3; i++) {
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


	// 生成采样核心-----------------------------------------------------------------------------------------
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); 
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, 
			randomFloats(generator) * 2.0 - 1.0, 
			randomFloats(generator) );
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0f;
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	// 创建随机旋转向量数组--------------------------------------------------------------------------------
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, 
			randomFloats(generator) * 2.0 - 1.0, 
			0.0f);
		ssaoNoise.push_back(noise);
	}
	unsigned int noiseTexture; glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


	unsigned int skyboxMap = loadCubemap(SKYBOX_TEXTURE);

	shaderLightingPass.use();
	shaderLightingPass.setInt("gPosition", 0);
	shaderLightingPass.setInt("gNormal", 1);
	shaderLightingPass.setInt("gAlbedo", 2);
	shaderLightingPass.setInt("ssao", 3);
	shaderLightingPass.setInt("texSkybox", 4);
	shaderSSAO.use();
	shaderSSAO.setInt("gPosition", 0);
	shaderSSAO.setInt("gNormal", 1);
	shaderSSAO.setInt("texNoise", 2);
	
	shaderSSAOBlur.use();
	shaderSSAOBlur.setInt("ssaoInput", 0);

	SkyboxShader.use();
	SkyboxShader.setInt("skybox", 0);


	while (!glfwWindowShouldClose(window))
	{
		if (MC_mode)
			glfwSetCursorPosCallback(window, mouse_callback);
		else
			glfwSetCursorPosCallback(window, NULL);

		int nowHeight = SCR_HEIGHT, nowWidth = SCR_WIDTH;
		glfwGetWindowSize(window, &nowWidth, &nowHeight);
		shaderSSAO.setFloat("width", nowWidth);
		shaderSSAO.setFloat("height", nowHeight);


		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);
		if (modelType == 0 && previosType != modelType)
			models = Model("../resources/objects/Dragon.obj");
		if (modelType == 1 && previosType != modelType)
			models = Model("../resources/objects/Buddha.obj");
		if (modelType == 2 && previosType != modelType)
		{
			models = Model("../resources/objects/Arma.obj");
			modelAngle2 = 180;
		}
		if (modelType == 3 && previosType != modelType)
			models = Model("../resources/objects/Block.obj");
		if (modelType == 4 && previosType != modelType)
			models = Model("../resources/objects/Bunny.obj");
		if (modelType == 5 && previosType != modelType)
		{
			models = Model("../resources/objects/dinosaur.obj");
			modelAngle1 = -66.8;
		}
		if (modelType == 6 && previosType != modelType)
			models = Model("../resources/objects/feisar.obj");
		if (modelType == 7 && previosType != modelType)
		{
			models = Model("../resources/objects/horse.obj");
			modelAngle1 = -89.1;
			modelAngle3 = 146;
		}
		if (modelType == 8 && previosType != modelType)
			models = Model("../resources/objects/kitten.obj");
		if (modelType == 9 && previosType != modelType)
			models = Model("../resources/objects/rocker.obj");

		if ((modelType != 7 && modelType != 5 && modelType != 2)
			&& previosType != modelType)
			modelAngle1 = modelAngle2 = modelAngle3 = 0;

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shaderSSAO.use();
		shaderSSAO.setInt("kernelSize", kernelSize);
		shaderSSAO.setFloat("radius", radius);
		shaderSSAO.setFloat("bias", bias);

		// 1. 几何处理阶段
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 50.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
		shaderGeometryPass.use();
		shaderGeometryPass.setMat4("projection", projection);
		shaderGeometryPass.setMat4("view", view);
		// 场景
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0, -0.5f, 0.0f));
		model = glm::scale(model, glm::vec3(2.0f));
		model = glm::rotate(model, M_PI / 2.0f , glm::vec3(1.0f, 0.0f, 0.0f));
		shaderGeometryPass.setMat4("model", model);
		
		renderQuad();
		// 模型
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, modelPosition[modelType], 0.0));
		model = glm::rotate(model, glm::radians(modelAngle1), glm::vec3(1.0, 0.0, 0.0));
		model = glm::rotate(model, glm::radians(modelAngle2), glm::vec3(0.0, 1.0, 0.0));
		model = glm::rotate(model, glm::radians(modelAngle3), glm::vec3(0.0, 0.0, 1.0));
		model = glm::scale(model, glm::vec3(modelScale[modelType]));
		shaderGeometryPass.setMat4("model", model);
		models.Draw(shaderGeometryPass);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		// 2. 生成SSAO纹理
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		shaderSSAO.use();
		for (unsigned int i = 0; i < 64; ++i)
			shaderSSAO.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
		shaderSSAO.setMat4("projection", projection);
		//shaderSSAO.setMat4("iview", glm::inverse(view));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		// 3. 模糊化
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		shaderSSAOBlur.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssaoTex);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 4.天空盒
		glBindFramebuffer(GL_FRAMEBUFFER, skyboxFBO);
		glClear(GL_COLOR_BUFFER_BIT);
		glDepthFunc(GL_LEQUAL);
		SkyboxShader.use();
		auto viewM2 = glm::mat4(glm::mat3(view)); 
		SkyboxShader.setMat4("view", viewM2);
		SkyboxShader.setMat4("projection", projection);
		// 天空盒
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxMap);
		renderCube();
		glDepthFunc(GL_LESS);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		// 4. 光照明处理阶段
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shaderLightingPass.use();
		glm::vec3 lightPosView = glm::vec3(camera.GetViewMatrix() * glm::vec4(lightXYZ[0], lightXYZ[1], lightXYZ[2], 1.0));
		shaderLightingPass.setVec3("light.Position", lightPosView);
		shaderLightingPass.setVec3("light.Color", glm::fvec3(lightColor[0], lightColor[1], lightColor[2]));
		const float linear = 0.09f;
		const float quadratic = 0.032f;
		shaderLightingPass.setFloat("light.Linear", linear);
		shaderLightingPass.setFloat("light.Quadratic", quadratic);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, ssaoBlurTex);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, skyboxTex);
		renderQuad();

		setImgui(window);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}


unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = 0;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
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

	ImGui::Text("SSAO Settings:");
	ImGui::SliderInt("Size of Kernel", &kernelSize, 1, 500);
	ImGui::SliderFloat("Radius", &radius, 0.0f, 3.0f, "%.1f");
	ImGui::SliderFloat("Bias", &bias, 0.0f, 1.0f, "%.3f");

	ImGui::Text("Light Settings:");
	ImGui::Text("Position:");
	ImGui::SliderFloat("Light Position.x", &lightXYZ[0], -10.0f, 10.0f, "%.3f");
	ImGui::SliderFloat("Light Position.y", &lightXYZ[1], -10.0f, 10.0f, "%.3f");
	ImGui::SliderFloat("Light Position.z", &lightXYZ[2], -10.0f, 10.0f, "%.3f");
	ImGui::Text("Color:");
	ImGui::ColorEdit3("Light Color", lightColor);

	ImGui::Text("Model Settings:");
	const char* items[] = { "Dragon", "Buddha","Arma","Block","Bunny","Dinosaur"
,"Feisar","Horse","Kitten","Rocker" };
	ImGui::Combo("Model", &modelType, items, IM_ARRAYSIZE(items));
	ImGui::SliderFloat("Model Angel-x", &modelAngle1, -180.0f, 180.0f, "angle = %.1f");
	ImGui::SliderFloat("Model Angel-y", &modelAngle2, -180.0f, 180.0f, "angle = %.1f");
	ImGui::SliderFloat("Model Angel-z", &modelAngle3, -180.0f, 180.0f, "angle = %.1f");
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