#pragma once

#include "logic/scheduler.h"
#include "model/model.h"
#include <vector>

namespace tt {

  class AppUI {
public:
    AppUI();

    void Draw();

private:
    void DrawToolbar();
    void DrawLeftPanel();
    void DrawTeachersPane();
    void DrawClassesPane();
    void DrawRightPanel();
    void DrawConflictsPane();

    void Rebuild();
    void SeedSampleData();

    const char* ClassName(Id id) const;
    const char* TeacherName(Id id) const;

    enum class ViewFilter { All,
      Teacher,
      Class };
    ViewFilter view_ = ViewFilter::All;
    bool LessonVisible(const Lesson& l) const;

    ScheduleState state_;

    char t_name_[64] = { };
    int t_selected_ = -1;

    char sa_subject_[64] = { };
    int sa_class_idx_ = 0;
    int sa_hours_ = 1;

    int ds_day_ = 0;
    int ds_from_ = 8;
    int ds_to_ = 14;
    int ds_editing_ = -1;

    char c_name_[64] = { };
    int c_selected_ = -1;

    float left_split_ = 0.5f;
    float main_split_ = 340.0f;

    std::vector<Conflict> conflicts_;

    Id dragging_lesson_ = kInvalidId;

    std::vector<int> row_times_min_;
    int rt_start_ = -1, rt_rows_ = -1, rt_dur_ = -1;
    void EnsureRowTimes();

    char status_msg_[128] = { };
  };

} // namespace tt
