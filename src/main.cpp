#include "platform/backend.h"
#include "ui/app_ui.h"
#include <imgui.h>

int main(int, char**)
{
  if (!Backend::Init("Timetabler — составление расписания", 1400, 860))
    return 1;

  tt::AppUI app;

  while (!Backend::ShouldClose()) {
    Backend::NewFrame();
    app.Draw();
    Backend::Render();
  }

  Backend::Shutdown();
  return 0;
}
