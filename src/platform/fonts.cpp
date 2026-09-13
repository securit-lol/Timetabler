#include "fonts.h"
#include <cstdio>
#include <imgui.h>

void LoadAppFonts(ImGuiIO& io, float size_px)
{

  const char* font_paths[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
    "C:\\Windows\\Fonts\\arial.ttf",
    "C:\\Windows\\Fonts\\segoeui.ttf",
    nullptr
  };

  const char* loaded_path = nullptr;
  for (int i = 0; font_paths[i]; ++i) {
    FILE* test = fopen(font_paths[i], "rb");
    if (test) {
      fclose(test);
      loaded_path = font_paths[i];
      break;
    }
  }

  if (!loaded_path) {

    io.Fonts->AddFontDefault();
    printf("[fonts] Не найден TTF-шрифт с кириллицей, используется встроенный (только латиница)\n");
    return;
  }

  ImFontGlyphRangesBuilder builder;
  builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
  builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());

  const ImWchar extra[] = {
    0x00D7, 0x00D7,
    0x2013, 0x2014,
    0x2022, 0x2022,
    0x2264, 0x2264,
    0x2717, 0x2717,
    0
  };
  builder.AddRanges(extra);

  ImVector<ImWchar> ranges;
  builder.BuildRanges(&ranges);

  ImFontConfig cfg;
  cfg.OversampleH = 2;
  cfg.OversampleV = 1;
  cfg.PixelSnapH = true;

  ImFont* font = io.Fonts->AddFontFromFileTTF(loaded_path, size_px, &cfg, ranges.Data);
  if (!font) {

    io.Fonts->AddFontDefault();
    printf("[fonts] Не удалось загрузить %s, используется встроенный шрифт\n", loaded_path);
  } else {
    printf("[fonts] Загружен %s с кириллицей\n", loaded_path);
  }
}
