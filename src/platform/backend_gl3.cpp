// OpenGL3 + GLFW backend for Linux/macOS
#include "backend.h"
#include "fonts.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace Backend {

  static GLFWwindow* g_Window = nullptr;

  static void glfw_error_callback(int error, const char* description)
  {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
  }

  bool Init(const char* window_title, int width, int height)
  {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
      return false;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    g_Window = glfwCreateWindow(width, height, window_title, nullptr, nullptr);
    if (!g_Window)
      return false;
    glfwMakeContextCurrent(g_Window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Load a font that supports Cyrillic (see fonts.cpp).
    LoadAppFonts(io, 16.0f);

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(g_Window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    return true;
  }

  void Shutdown()
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (g_Window) {
      glfwDestroyWindow(g_Window);
      g_Window = nullptr;
    }
    glfwTerminate();
  }

  bool ShouldClose()
  {
    return glfwWindowShouldClose(g_Window);
  }

  void NewFrame()
  {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
  }

  void Render()
  {
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(g_Window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(g_Window);
  }

} // namespace Backend
