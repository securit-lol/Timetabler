#include "ui/app_ui.h"
#include "logic/scheduler.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <imgui.h>

namespace tt {

  static ImU32 StatusColor(LessonStatus s)
  {
    switch (s) {
    case LessonStatus::Planned:
      return IM_COL32(60, 100, 160, 255);
    case LessonStatus::Done:
      return IM_COL32(70, 110, 70, 255);
    case LessonStatus::Cancelled:
      return IM_COL32(110, 70, 70, 255);
    case LessonStatus::Conflict:
      return IM_COL32(160, 120, 40, 255);
    }
    return IM_COL32(80, 80, 80, 255);
  }

  AppUI::AppUI()
  {
    SeedSampleData();
  }

  const char* AppUI::ClassName(Id id) const
  {
    const SchoolClass* c = state_.FindClass(id);
    return c ? c->name.c_str() : "?";
  }

  const char* AppUI::TeacherName(Id id) const
  {
    const Teacher* t = state_.FindTeacher(id);
    return t ? t->name.c_str() : "?";
  }

  bool AppUI::LessonVisible(const Lesson& l) const
  {
    switch (view_) {
    case ViewFilter::Teacher:
      if (t_selected_ < 0 || t_selected_ >= (int)state_.teachers.size())
        return true;
      return l.teacher_id == state_.teachers[t_selected_].id;
    case ViewFilter::Class:
      if (c_selected_ < 0 || c_selected_ >= (int)state_.classes.size())
        return true;
      return l.class_id == state_.classes[c_selected_].id;
    case ViewFilter::All:
    default:
      return true;
    }
  }

  void AppUI::SeedSampleData()
  {
    SchoolClass a;
    a.id = state_.NewId();
    a.name = "11А";
    a.max_lessons_per_day = 7;
    SchoolClass b;
    b.id = state_.NewId();
    b.name = "11Б";
    b.max_lessons_per_day = 7;
    state_.classes.push_back(a);
    state_.classes.push_back(b);

    Teacher t;
    t.id = state_.NewId();
    t.name = "Иванова А.П.";
    t.max_lessons_per_day = 5;
    t.subjects.push_back({ "Математика", a.id, 3 });
    t.subjects.push_back({ "Математика", b.id, 2 });
    t.desired_slots.push_back({ Day::Mon, 9, 13 });
    t.desired_slots.push_back({ Day::Wed, 9, 13 });
    t.desired_slots.push_back({ Day::Fri, 9, 12 });
    state_.teachers.push_back(t);

    Teacher p;
    p.id = state_.NewId();
    p.name = "Петров В.С.";
    p.max_lessons_per_day = 4;
    p.subjects.push_back({ "Физика", a.id, 2 });
    p.subjects.push_back({ "Физика", b.id, 2 });
    p.desired_slots.push_back({ Day::Mon, 9, 13 });
    p.desired_slots.push_back({ Day::Tue, 10, 14 });
    p.desired_slots.push_back({ Day::Thu, 10, 14 });
    state_.teachers.push_back(p);

    t_selected_ = 0;
    Rebuild();
  }

  void AppUI::Rebuild()
  {
    ScheduleResult r = BuildSchedule(state_);
    conflicts_ = r.conflicts;
    std::snprintf(status_msg_, sizeof(status_msg_),
        "Составлено: %d уроков размещено, %d сохранено, %d конфликтов",
        r.placed, r.kept, (int)r.conflicts.size());
  }

  void AppUI::EnsureRowTimes()
  {
    int rows = std::max(1, state_.day_hour_end - state_.day_hour_start);
    if (rt_start_ == state_.day_hour_start && rt_rows_ == rows && rt_dur_ == state_.lesson_duration_min && (int)row_times_min_.size() == rows)
      return;

    row_times_min_.resize(rows);
    int base = state_.day_hour_start * 60;
    for (int i = 0; i < rows; ++i)
      row_times_min_[i] = base + i * state_.lesson_duration_min;

    rt_start_ = state_.day_hour_start;
    rt_rows_ = rows;
    rt_dur_ = state_.lesson_duration_min;
  }

  void AppUI::Draw()
  {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::SetNextWindowViewport(vp->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##Root", nullptr, flags);
    ImGui::PopStyleVar(2);

    DrawToolbar();
    ImGui::Separator();

    float avail_h = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("##left", ImVec2(main_split_, avail_h), true);
    DrawLeftPanel();
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::InvisibleButton("##vsplit", ImVec2(6.0f, avail_h));
    if (ImGui::IsItemActive())
      main_split_ += ImGui::GetIO().MouseDelta.x;
    main_split_ = std::max(240.0f, std::min(main_split_, 640.0f));
    if (ImGui::IsItemHovered())
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    ImGui::SameLine();

    ImGui::BeginChild("##right", ImVec2(0, avail_h), true);
    DrawRightPanel();
    ImGui::EndChild();

    ImGui::End();
  }

  void AppUI::DrawToolbar()
  {
    ImGui::Dummy(ImVec2(4, 0));
    ImGui::SameLine();
    if (ImGui::Button("Составить расписание")) {
      Rebuild();
    }
    ImGui::SameLine();

    ImGui::Text("Сегодня:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    int cur = static_cast<int>(state_.current_day);
    const char* days[] = { "Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс" };
    if (ImGui::Combo("##today", &cur, days, kDayCount))
      state_.current_day = static_cast<Day>(cur);

    ImGui::SameLine();
    ImGui::Text("|  Учебный день: с");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::InputInt("##hstart", &state_.day_hour_start, 0, 0);
    ImGui::SameLine();
    ImGui::Text("по");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::InputInt("##hend", &state_.day_hour_end, 0, 0);

    if (state_.day_hour_start < 0)
      state_.day_hour_start = 0;
    if (state_.day_hour_start > 22)
      state_.day_hour_start = 22;
    if (state_.day_hour_end <= state_.day_hour_start)
      state_.day_hour_end = state_.day_hour_start + 1;
    if (state_.day_hour_end > 23)
      state_.day_hour_end = 23;

    ImGui::SameLine();
    ImGui::Text("| Урок:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(55);
    ImGui::InputInt("##dur", &state_.lesson_duration_min, 0, 0);
    ImGui::SameLine();
    ImGui::Text("мин");
    if (state_.lesson_duration_min < 5)
      state_.lesson_duration_min = 5;
    if (state_.lesson_duration_min > 180)
      state_.lesson_duration_min = 180;

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", status_msg_);
  }

  void AppUI::DrawLeftPanel()
  {
    float total = ImGui::GetContentRegionAvail().y;
    float top_h = total * left_split_;

    ImGui::BeginChild("##teachers", ImVec2(0, top_h), false);
    DrawTeachersPane();
    ImGui::EndChild();

    ImGui::InvisibleButton("##hsplit", ImVec2(-1, 6.0f));
    if (ImGui::IsItemActive())
      left_split_ += ImGui::GetIO().MouseDelta.y / total;
    left_split_ = std::max(0.2f, std::min(left_split_, 0.8f));
    if (ImGui::IsItemHovered())
      ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);

    ImGui::BeginChild("##classes", ImVec2(0, 0), false);
    DrawClassesPane();
    ImGui::EndChild();
  }

  void AppUI::DrawTeachersPane()
  {
    ImGui::SeparatorText("Преподаватели");

    ImGui::SetNextItemWidth(-30);
    ImGui::InputTextWithHint("##tname", "Имя преподавателя", t_name_, sizeof(t_name_));
    ImGui::SameLine();
    if (ImGui::Button("+##addt") && t_name_[0]) {
      Teacher t;
      t.id = state_.NewId();
      t.name = t_name_;
      state_.teachers.push_back(t);
      t_selected_ = (int)state_.teachers.size() - 1;
      t_name_[0] = '\0';
    }

    ImGui::BeginChild("##tlist", ImVec2(0, 110), true);
    for (int i = 0; i < (int)state_.teachers.size(); ++i) {
      Teacher& t = state_.teachers[i];
      char label[128];
      std::snprintf(label, sizeof(label), "%s (≤%d уроков/д)##t%d",
          t.name.c_str(), t.max_lessons_per_day, t.id);
      if (ImGui::Selectable(label, t_selected_ == i)) {
        t_selected_ = i;
        ds_editing_ = -1;
        view_ = ViewFilter::Teacher;
        c_selected_ = -1;
      }
      ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
      char rm[16];
      std::snprintf(rm, sizeof(rm), "×##rt%d", t.id);
      if (ImGui::SmallButton(rm)) {
        state_.teachers.erase(state_.teachers.begin() + i);
        if (t_selected_ >= (int)state_.teachers.size())
          t_selected_ = (int)state_.teachers.size() - 1;
        --i;
      }
    }
    ImGui::EndChild();

    if (t_selected_ >= 0 && t_selected_ < (int)state_.teachers.size()) {
      Teacher& t = state_.teachers[t_selected_];
      ImGui::SeparatorText("Предметы и нагрузка");

      for (int j = 0; j < (int)t.subjects.size(); ++j) {
        SubjectAssignment& s = t.subjects[j];
        ImGui::PushID(j);
        ImGui::BulletText("%s — %s — %d ч/нед",
            s.subject_name.c_str(), ClassName(s.class_id), s.hours_per_week);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
        if (ImGui::SmallButton("×")) {
          t.subjects.erase(t.subjects.begin() + j);
          --j;
        }
        ImGui::PopID();
      }

      ImGui::SetNextItemWidth(110);
      ImGui::InputTextWithHint("##sasubj", "Предмет", sa_subject_, sizeof(sa_subject_));
      ImGui::SameLine();
      ImGui::SetNextItemWidth(80);

      std::vector<const char*> cnames;
      for (auto& c : state_.classes)
        cnames.push_back(c.name.c_str());
      if (sa_class_idx_ >= (int)cnames.size())
        sa_class_idx_ = 0;
      if (!cnames.empty())
        ImGui::Combo("##saclass", &sa_class_idx_, cnames.data(), (int)cnames.size());
      ImGui::SameLine();
      ImGui::SetNextItemWidth(60);
      ImGui::DragInt("##sahours", &sa_hours_, 0.1f, 1, 20, "%dч");
      ImGui::SameLine();
      if (ImGui::Button("+##adds") && sa_subject_[0] && !state_.classes.empty()) {
        SubjectAssignment s;
        s.subject_name = sa_subject_;
        s.class_id = state_.classes[sa_class_idx_].id;
        s.hours_per_week = sa_hours_;
        t.subjects.push_back(s);
        sa_subject_[0] = '\0';
      }

      ImGui::SeparatorText("Желаемое время");
      for (int j = 0; j < (int)t.desired_slots.size(); ++j) {
        TimeRange& r = t.desired_slots[j];
        ImGui::PushID(1000 + j);
        ImGui::BulletText("%s %d:00–%d:00", DayName(r.day), r.hour_from, r.hour_to);

        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 62);
        if (ImGui::SmallButton("ред##ed")) {
          ds_editing_ = j;
          ds_day_ = static_cast<int>(r.day);
          ds_from_ = r.hour_from;
          ds_to_ = r.hour_to;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("×")) {
          t.desired_slots.erase(t.desired_slots.begin() + j);
          if (ds_editing_ == j)
            ds_editing_ = -1;
          else if (ds_editing_ > j)
            --ds_editing_;
          --j;
        }
        ImGui::PopID();
      }
      ImGui::SetNextItemWidth(70);
      const char* days[] = { "Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс" };
      ImGui::Combo("##dsday", &ds_day_, days, kDayCount);
      ImGui::SameLine();
      ImGui::Text("с");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(50);
      ImGui::InputInt("##dsfrom", &ds_from_, 0, 0);
      ImGui::SameLine();
      ImGui::Text("по");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(50);
      ImGui::InputInt("##dsto", &ds_to_, 0, 0);
      ImGui::SameLine();

      auto overlaps = [&](int day, int from, int to, int ignore_idx) {
        for (int k = 0; k < (int)t.desired_slots.size(); ++k) {
          if (k == ignore_idx)
            continue;
          const TimeRange& o = t.desired_slots[k];
          if (static_cast<int>(o.day) != day)
            continue;
          if (from < o.hour_to && o.hour_from < to)
            return true;
        }
        return false;
      };

      const char* apply_lbl = (ds_editing_ >= 0) ? "OK##adds2" : "+##adds2";
      if (ImGui::Button(apply_lbl)) {
        bool editing = (ds_editing_ >= 0 && ds_editing_ < (int)t.desired_slots.size());
        if (ds_from_ >= ds_to_) {
          std::snprintf(status_msg_, sizeof(status_msg_),
              "Ошибка: начало диапазона (%d) >= конца (%d)", ds_from_, ds_to_);
        } else if (overlaps(ds_day_, ds_from_, ds_to_, editing ? ds_editing_ : -1)) {
          std::snprintf(status_msg_, sizeof(status_msg_),
              "Ошибка: диапазон %s %d–%d пересекается с уже заданным",
              days[ds_day_], ds_from_, ds_to_);
        } else if (editing) {
          TimeRange& r = t.desired_slots[ds_editing_];
          r.day = static_cast<Day>(ds_day_);
          r.hour_from = ds_from_;
          r.hour_to = ds_to_;
          std::snprintf(status_msg_, sizeof(status_msg_), "Желаемый слот изменён");
          ds_editing_ = -1;
        } else {
          TimeRange r;
          r.day = static_cast<Day>(ds_day_);
          r.hour_from = ds_from_;
          r.hour_to = ds_to_;
          t.desired_slots.push_back(r);
          std::snprintf(status_msg_, sizeof(status_msg_), "Желаемый слот добавлен");
        }
      }
      if (ds_editing_ >= 0) {
        ImGui::SameLine();
        if (ImGui::Button("Отмена##dscancel"))
          ds_editing_ = -1;
      }
    }
  }

  void AppUI::DrawClassesPane()
  {
    ImGui::SeparatorText("Классы");

    ImGui::SetNextItemWidth(-30);
    ImGui::InputTextWithHint("##cname", "Название класса", c_name_, sizeof(c_name_));
    ImGui::SameLine();
    if (ImGui::Button("+##addc") && c_name_[0]) {
      SchoolClass c;
      c.id = state_.NewId();
      c.name = c_name_;
      state_.classes.push_back(c);
      c_name_[0] = '\0';
    }

    ImGui::BeginChild("##clist", ImVec2(0, 0), true);
    for (int i = 0; i < (int)state_.classes.size(); ++i) {
      SchoolClass& c = state_.classes[i];

      int nteach = 0;
      for (auto& t : state_.teachers)
        for (auto& s : t.subjects)
          if (s.class_id == c.id) {
            ++nteach;
            break;
          }

      char label[128];
      std::snprintf(label, sizeof(label), "%s  (≤%dуроков/д, преподавателей: %d)##c%d",
          c.name.c_str(), c.max_lessons_per_day, nteach, c.id);
      if (ImGui::Selectable(label, c_selected_ == i)) {
        c_selected_ = i;
        view_ = ViewFilter::Class;
      }
      ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
      char rm[16];
      std::snprintf(rm, sizeof(rm), "×##rc%d", c.id);
      if (ImGui::SmallButton(rm)) {
        state_.classes.erase(state_.classes.begin() + i);
        if (c_selected_ >= (int)state_.classes.size())
          c_selected_ = (int)state_.classes.size() - 1;
        --i;
      }
    }
    ImGui::EndChild();
  }

  void AppUI::DrawRightPanel()
  {

    const char* view_label = "Всё расписание";
    if (view_ == ViewFilter::Teacher && t_selected_ >= 0 && t_selected_ < (int)state_.teachers.size())
      view_label = state_.teachers[t_selected_].name.c_str();
    else if (view_ == ViewFilter::Class && c_selected_ >= 0 && c_selected_ < (int)state_.classes.size())
      view_label = state_.classes[c_selected_].name.c_str();

    ImGui::Text("Расписание:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.55f, 0.75f, 0.95f, 1.0f), "%s", view_label);
    if (view_ != ViewFilter::All) {
      ImGui::SameLine();
      if (ImGui::SmallButton("Показать всё"))
        view_ = ViewFilter::All;
    }
    ImGui::Separator();

    int hstart = state_.day_hour_start;
    int hend = state_.day_hour_end;
    int nrows = std::max(1, hend - hstart);

    EnsureRowTimes();

    const float time_col_w = 48.0f;
    const float right_pad = 12.0f;
    const float header_h = 52.0f;
    const float row_h = 46.0f;
    const float conflicts_h = 140.0f;

    float avail_w = ImGui::GetContentRegionAvail().x - time_col_w - right_pad;
    float col_w = std::max(90.0f, avail_w / kDayCount);

    float grid_h = ImGui::GetContentRegionAvail().y - conflicts_h;
    if (grid_h < 200.0f)
      grid_h = 200.0f;

    ImGui::BeginChild("##grid", ImVec2(0, grid_h), false,
        ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    auto colX = [&](int d) { return origin.x + time_col_w + d * col_w; };

    for (int d = 0; d < kDayCount; ++d) {
      Day day = static_cast<Day>(d);
      bool is_today = (day == state_.current_day);
      bool disabled = !state_.DayEnabled(day);
      float x = colX(d);

      ImGui::PushID(d);

      ImGui::SetCursorScreenPos(ImVec2(x, origin.y));
      ImVec4 hdr = is_today ? ImVec4(0.25f, 0.45f, 0.65f, 1.0f)
                            : ImVec4(0.20f, 0.20f, 0.22f, 1.0f);
      ImGui::PushStyleColor(ImGuiCol_Button, hdr);
      char lbl[32];
      std::snprintf(lbl, sizeof(lbl), "%s%s", DayNameFull(day), is_today ? " •" : "");
      ImGui::Button(lbl, ImVec2(col_w - 4, 22));
      ImGui::PopStyleColor();

      ImGui::SetCursorScreenPos(ImVec2(x, origin.y + 24));
      ImGui::PushStyleColor(ImGuiCol_Button,
          disabled ? ImVec4(0.5f, 0.2f, 0.2f, 1.0f) : ImVec4(0.2f, 0.35f, 0.2f, 1.0f));
      const char* toggle = disabled ? "не проводить" : "проводить";
      if (ImGui::Button(toggle, ImVec2(col_w - 4, 22))) {
        if (disabled) {
          state_.disabled_days.erase(day);
          std::snprintf(status_msg_, sizeof(status_msg_),
              "%s снова учебный (нажмите «Составить расписание»)", DayName(day));
        } else {
          state_.disabled_days.insert(day);
          int n = 0;
          for (auto& l : state_.lessons)
            if (l.day == day && l.status == LessonStatus::Planned && !l.locked) {
              l.status = LessonStatus::Cancelled;
              ++n;
            }
          std::snprintf(status_msg_, sizeof(status_msg_),
              "%s отключён: отменено %d уроков", DayName(day), n);
        }
      }
      ImGui::PopStyleColor();
      ImGui::PopID();
    }

    ImVec2 body = ImVec2(origin.x, origin.y + header_h);

    ImGuiIO& io = ImGui::GetIO();
    for (int r = 0; r < nrows; ++r) {
      int mins = (r < (int)row_times_min_.size()) ? row_times_min_[r]
                                                  : (hstart + r) * 60;
      ImVec2 lp(body.x + 2, body.y + r * row_h + 4);
      char hbuf[16];
      std::snprintf(hbuf, sizeof(hbuf), "%d:%02d", mins / 60, mins % 60);

      ImGui::SetCursorScreenPos(ImVec2(body.x, body.y + r * row_h));
      ImGui::PushID(10000 + r);
      ImGui::InvisibleButton("##time", ImVec2(time_col_w, row_h));
      bool hovered = ImGui::IsItemHovered();
      ImGui::PopID();

      if (hovered && io.MouseWheel != 0.0f && !row_times_min_.empty()) {
        int delta = (io.MouseWheel > 0.0f ? 5 : -5);
        for (int k = r; k < (int)row_times_min_.size(); ++k) {
          int v = row_times_min_[k] + delta;
          if (v < 0)
            v = 0;
          if (v > 1439)
            v = 1439;
          row_times_min_[k] = v;
        }
      }

      ImU32 tcol = hovered ? IM_COL32(255, 230, 150, 255)
                           : IM_COL32(200, 200, 200, 255);
      dl->AddText(lp, tcol, hbuf);
      if (hovered)
        ImGui::SetTooltip("Колесо мыши — сдвиг времени (±5 мин, каскадом ниже)");

      for (int d = 0; d < kDayCount; ++d) {
        Day day = static_cast<Day>(d);
        ImVec2 p0(colX(d), body.y + r * row_h);
        ImVec2 p1(p0.x + col_w - 3, p0.y + row_h - 3);
        ImU32 bg = state_.DayEnabled(day) ? IM_COL32(38, 38, 42, 255)
                                          : IM_COL32(28, 22, 22, 255);
        dl->AddRectFilled(p0, p1, bg, 3.0f);
        dl->AddRect(p0, p1, IM_COL32(60, 60, 66, 255), 3.0f);
      }
    }

    auto cellAt = [&](ImVec2 pos, Day& out_day, int& out_hour) -> bool {
      float rx = pos.x - body.x - time_col_w;
      float ry = pos.y - body.y;
      if (rx < 0 || ry < 0)
        return false;
      int d = (int)(rx / col_w);
      int r = (int)(ry / row_h);
      if (d < 0 || d >= kDayCount || r < 0 || r >= nrows)
        return false;
      out_day = static_cast<Day>(d);
      out_hour = hstart + r;
      return true;
    };

    for (int r = 0; r < nrows; ++r) {
      int hour = hstart + r;
      for (int d = 0; d < kDayCount; ++d) {
        Day day = static_cast<Day>(d);
        ImVec2 p0(colX(d), body.y + r * row_h);
        ImVec2 cell_sz(col_w - 3, row_h - 3);

        std::vector<Lesson*> cell_lessons;
        for (auto& l : state_.lessons) {
          if (l.day == day && l.hour == hour && l.status != LessonStatus::Cancelled
              && LessonVisible(l))
            cell_lessons.push_back(&l);
        }
        if (cell_lessons.empty())
          continue;
        int count = (int)cell_lessons.size();

        Lesson* primary = nullptr;
        for (Lesson* lp : cell_lessons)
          if (lp->id != dragging_lesson_) {
            primary = lp;
            break;
          }

        if (!primary)
          continue;
        Lesson& l = *primary;

        ImVec2 q1(p0.x + cell_sz.x, p0.y + cell_sz.y);
        dl->AddRectFilled(p0, q1, StatusColor(l.status), 3.0f);
        if (l.locked)
          dl->AddRect(p0, q1, IM_COL32(240, 220, 120, 255), 3.0f, 0, 2.0f);
        char txt[128];
        std::snprintf(txt, sizeof(txt), "%s%s\n%s\n%s",
            l.locked ? "[закр] " : "",
            l.subject_name.c_str(),
            ClassName(l.class_id),
            TeacherName(l.teacher_id));
        dl->AddText(ImVec2(p0.x + 4, p0.y + 3), IM_COL32(240, 240, 240, 255), txt);

        if (count > 1) {
          char badge[16];
          std::snprintf(badge, sizeof(badge), "×%d", count);
          ImVec2 bs = ImGui::CalcTextSize(badge);
          ImVec2 b1(p0.x + cell_sz.x - bs.x - 8, p0.y + 2);
          ImVec2 b2(b1.x + bs.x + 6, b1.y + bs.y + 2);
          dl->AddRectFilled(b1, b2, IM_COL32(20, 20, 24, 220), 4.0f);
          dl->AddText(ImVec2(b1.x + 3, b1.y + 1), IM_COL32(255, 230, 150, 255), badge);
        }

        ImGui::SetCursorScreenPos(p0);
        char bid[32];
        std::snprintf(bid, sizeof(bid), "L%d", l.id);
        bool pressed = ImGui::InvisibleButton(bid, cell_sz);

        if (dragging_lesson_ == kInvalidId && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
          dragging_lesson_ = l.id;
        }

        char popid[32];
        std::snprintf(popid, sizeof(popid), "cellpop_%d_%d", d, hour);
        if (pressed && dragging_lesson_ == kInvalidId && count > 1)
          ImGui::OpenPopup(popid);

        if (ImGui::BeginPopup(popid)) {
          ImGui::TextDisabled("%s %d:00 — уроков: %d", DayName(day), hour, count);
          ImGui::Separator();
          for (Lesson* lp : cell_lessons) {
            ImGui::PushID(lp->id);
            ImGui::Text("%s%s — %s", lp->locked ? "[закр] " : "",
                lp->subject_name.c_str(), ClassName(lp->class_id));
            ImGui::TextDisabled("%s · %s", TeacherName(lp->teacher_id),
                StatusName(lp->status));
            if (ImGui::SmallButton(lp->locked ? "Открепить" : "Закрепить"))
              lp->locked = !lp->locked;
            ImGui::SameLine();
            if (ImGui::SmallButton("Отменить"))
              lp->status = LessonStatus::Cancelled;
            ImGui::Separator();
            ImGui::PopID();
          }
          ImGui::EndPopup();
        }

        if (ImGui::BeginPopupContextItem(bid)) {
          if (ImGui::MenuItem(l.locked ? "Открепить" : "Закрепить"))
            l.locked = !l.locked;

          if (ImGui::MenuItem("Отменить на неделе", nullptr, false, count == 1))
            l.status = LessonStatus::Cancelled;
          if (count > 1)
            ImGui::TextDisabled("Отмена — в окне (клик по ячейке)");
          ImGui::EndPopup();
        }
        if (ImGui::IsItemHovered() && dragging_lesson_ == kInvalidId) {
          if (count > 1)
            ImGui::SetTooltip("%s — %s\n%s\nСтатус: %s\nКликните, чтобы увидеть все %d уроков",
                l.subject_name.c_str(), ClassName(l.class_id),
                TeacherName(l.teacher_id), StatusName(l.status), count);
          else
            ImGui::SetTooltip("%s — %s\n%s\nСтатус: %s\nПеретащите мышью, ПКМ — меню",
                l.subject_name.c_str(), ClassName(l.class_id),
                TeacherName(l.teacher_id), StatusName(l.status));
        }
      }
    }

    if (dragging_lesson_ != kInvalidId) {
      Lesson* drag = nullptr;
      for (auto& l : state_.lessons)
        if (l.id == dragging_lesson_) {
          drag = &l;
          break;
        }

      if (drag) {
        ImVec2 mp = ImGui::GetIO().MousePos;
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        Day tgt_day;
        int tgt_hour;
        bool over_cell = cellAt(mp, tgt_day, tgt_hour);
        bool valid_target = false;
        if (over_cell) {
          std::string reason;
          valid_target = CanPlaceAt(state_, *drag, tgt_day, tgt_hour, reason);
          int rr = tgt_hour - hstart;
          int dd = static_cast<int>(tgt_day);
          ImVec2 hp0(colX(dd), body.y + rr * row_h);
          ImVec2 hp1(hp0.x + col_w - 3, hp0.y + row_h - 3);
          ImU32 hl = valid_target ? IM_COL32(90, 200, 90, 90) : IM_COL32(200, 80, 80, 90);
          dl->AddRectFilled(hp0, hp1, hl, 3.0f);
        }

        ImVec2 g0(mp.x + 10, mp.y + 8);
        ImVec2 g1(g0.x + col_w - 6, g0.y + row_h - 6);
        dl->AddRectFilled(g0, g1, StatusColor(drag->status), 3.0f);
        dl->AddRect(g0, g1, IM_COL32(255, 255, 255, 200), 3.0f, 0, 1.5f);
        char gtxt[128];
        std::snprintf(gtxt, sizeof(gtxt), "%s\n%s\n%s",
            drag->subject_name.c_str(),
            ClassName(drag->class_id),
            TeacherName(drag->teacher_id));
        dl->AddText(ImVec2(g0.x + 4, g0.y + 3), IM_COL32(255, 255, 255, 255), gtxt);

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
          if (over_cell && valid_target) {
            drag->day = tgt_day;
            drag->hour = tgt_hour;
            drag->status = state_.DayInPast(tgt_day) ? LessonStatus::Done
                                                     : LessonStatus::Planned;
            std::snprintf(status_msg_, sizeof(status_msg_), "Урок перенесён");
          } else {
            std::string reason = "ячейка занята или недоступна";
            if (over_cell)
              CanPlaceAt(state_, *drag, tgt_day, tgt_hour, reason);
            std::snprintf(status_msg_, sizeof(status_msg_),
                "Возвращён на место: %s", reason.c_str());
          }
          dragging_lesson_ = kInvalidId;
        }
      } else {
        dragging_lesson_ = kInvalidId;
      }
    }

    ImGui::SetCursorScreenPos(ImVec2(origin.x, body.y + nrows * row_h + 4));
    ImGui::Dummy(ImVec2(time_col_w + kDayCount * col_w + right_pad, 1));
    ImGui::EndChild();

    ImGui::Separator();
    DrawConflictsPane();
  }

  void AppUI::DrawConflictsPane()
  {
    ImGui::SeparatorText("Конфликты");
    ImGui::BeginChild("##conflicts", ImVec2(0, 0), false);
    if (conflicts_.empty()) {
      ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Конфликтов в расписании нет");
    } else {
      ImGui::Text("Не удалось разместить %d урок(ов):", (int)conflicts_.size());
      for (int i = 0; i < (int)conflicts_.size(); ++i) {
        const Conflict& c = conflicts_[i];
        ImGui::PushID(i);
        ImGui::TextColored(ImVec4(0.9f, 0.75f, 0.35f, 1.0f), "• %s — %s",
            c.subject_name.c_str(), ClassName(c.class_id));
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", TeacherName(c.teacher_id));
        ImGui::Indent();
        ImGui::TextWrapped("причина: %s", c.reason.c_str());
        ImGui::Unindent();
        ImGui::PopID();
      }
    }
    ImGui::EndChild();
  }

} // namespace tt
