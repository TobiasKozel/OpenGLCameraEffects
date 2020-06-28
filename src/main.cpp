#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <random>

#include "util/Event.h"
#include "util/Scene.h"
#include "DemoScenePost.h"

/**
 * Callbacks
 */
void resizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

GLFWwindow* window = nullptr;
int width = 1280;
int height = 720;
float lastX = width / 2.0f;
float lastY = height / 2.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Scene* scene = nullptr;

std::vector<Event> queue;

struct EventValue {
    Event::Type type;
    float value = 0;
};

std::map<int, EventValue> keyMap = {
    { GLFW_KEY_W, {Event::FORWARD } },
    { GLFW_KEY_S, {Event::BACKWARD } },
    { GLFW_KEY_A, {Event::LEFT } },
    { GLFW_KEY_D, {Event::RIGHT } },
};

void init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glewExperimental = GL_TRUE;
#endif

    window = glfwCreateWindow(width, height, "DOF Example", nullptr, nullptr);
	
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, resizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSwapInterval(0);

    if (!gladLoadGL()) {
        std::cout << "Error loading glad!\n";
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

int main() {
    init();
    
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    scene = new DemoScenePost(width, height);
	
    GLC(glEnable(GL_DEPTH_TEST));
    GLC(glFrontFace(GL_CCW));
    GLC(glCullFace(GL_BACK));
    GLC(glEnable(GL_CULL_FACE));
    //GLC(glEnable(GL_BLEND));
    //GLC(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
    	
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (scene != nullptr) {
            scene->draw();
        }

        glfwPollEvents();
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
        ImGui::Begin("Debug UI");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    	
        if (scene != nullptr) {
            scene->debugUi();
        	if (io.WantCaptureMouse) {
                queue.clear(); // If imgui handles the event, we'll clear the queue
        	} else {
                processInput(window);
        	}
            scene->update(queue, deltaTime);
            queue.clear();
        }
        
        ImGui::End();
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    for (auto& i : keyMap) {
        if (glfwGetKey(window, i.first) == GLFW_PRESS) {
            queue.push_back({ i.second.type, 0.0, i.second.value });
        }
    }
}

void resizeCallback(GLFWwindow* window, int w, int h) {
    width = w;
    height = h;
    GLC(glViewport(0, 0, width, height));
    queue.push_back({ Event::RESIZE, float(width), float(height) });
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE ||
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_RELEASE
    ) {
        queue.push_back({ Event::MOUSE_MOVE, float(xpos - lastX), float(lastY - ypos) });
    }
    lastX = xpos;
    lastY = ypos;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    queue.push_back({ Event::ZOOM, float(yoffset) });
}
