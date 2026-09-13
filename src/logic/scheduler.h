#pragma once

#include "model/model.h"
#include <string>
#include <vector>

namespace tt {

  struct Conflict {
    Id teacher_id = kInvalidId;
    Id class_id = kInvalidId;
    std::string subject_name;
    std::string reason;
  };

  struct ScheduleResult {
    int placed = 0;
    int kept = 0;
    std::vector<Conflict> conflicts;
  };

  ScheduleResult BuildSchedule(ScheduleState& state);

  bool RescheduleLesson(ScheduleState& state, Id lesson_id, std::string& reason);

  bool CanPlaceAt(const ScheduleState& state, const Lesson& lesson,
      Day day, int hour, std::string& reason);

} // namespace tt
