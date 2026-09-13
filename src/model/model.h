#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace tt {

  enum class Day : int {
    Mon = 0,
    Tue,
    Wed,
    Thu,
    Fri,
    Sat,
    Sun,
    Count
  };

  constexpr int kDayCount = static_cast<int>(Day::Count);

  const char* DayName(Day d);
  const char* DayNameFull(Day d);

  enum class LessonStatus {
    Planned,
    Done,
    Cancelled,
    Conflict
  };

  const char* StatusName(LessonStatus s);

  using Id = int;
  constexpr Id kInvalidId = -1;

  struct TimeRange {
    Day day = Day::Mon;
    int hour_from = 8;
    int hour_to = 14;
  };

  struct SubjectAssignment {
    std::string subject_name;
    Id class_id = kInvalidId;
    int hours_per_week = 1;
  };

  struct Teacher {
    Id id = kInvalidId;
    std::string name;
    int max_lessons_per_day = 6;
    std::vector<SubjectAssignment> subjects;
    std::vector<TimeRange> desired_slots;
  };

  struct SchoolClass {
    Id id = kInvalidId;
    std::string name;
    int max_lessons_per_day = 7;
  };

  struct Lesson {
    Id id = kInvalidId;
    Id teacher_id = kInvalidId;
    Id class_id = kInvalidId;
    std::string subject_name;
    Day day = Day::Mon;
    int hour = 8;
    bool locked = false;
    LessonStatus status = LessonStatus::Planned;
  };

  struct ScheduleState {

    int day_hour_start = 8;
    int day_hour_end = 15;
    int lesson_duration_min = 45;

    Day current_day = Day::Mon;
    std::set<Day> disabled_days;

    std::vector<Teacher> teachers;
    std::vector<SchoolClass> classes;
    std::vector<Lesson> lessons;

    Id next_id = 1;
    Id NewId() { return next_id++; }

    Teacher* FindTeacher(Id id);
    SchoolClass* FindClass(Id id);
    const Teacher* FindTeacher(Id id) const;
    const SchoolClass* FindClass(Id id) const;

    bool DayEnabled(Day d) const { return disabled_days.find(d) == disabled_days.end(); }
    bool DayInPast(Day d) const { return static_cast<int>(d) < static_cast<int>(current_day); }
  };

} // namespace tt
