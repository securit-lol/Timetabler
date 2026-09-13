#pragma once

// Platform-agnostic backend interface for ImGui initialization and rendering.
// Implementation selected at build time (backend_gl3.cpp or backend_dx11.cpp).

namespace Backend {

  // Initialize backend, create window, and set up ImGui context.
  // Returns false on failure.
  bool Init(const char* window_title, int width, int height);

  // Shutdown backend and destroy window.
  void Shutdown();

  // Check if the window should close.
  bool ShouldClose();

  // Begin a new frame (poll events, prepare ImGui for rendering).
  void NewFrame();

  // Render ImGui draw data and present the frame.
  void Render();

} // namespace Backend
