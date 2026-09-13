#include "model/model.h"

namespace tt {

  static const char* kDayShort[kDayCount] = {
    "Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"
  };

  static const char* kDayFull[kDayCount] = {
    "Понедельник", "Вторник", "Среда", "Четверг",
    "Пятница", "Суббота", "Воскресенье"
  };

  const char* DayName(Day d)
  {
    int i = static_cast<int>(d);
    if (i < 0 || i >= kDayCount)
      return "?";
    return kDayShort[i];
  }

  const char* DayNameFull(Day d)
  {
    int i = static_cast<int>(d);
    if (i < 0 || i >= kDayCount)
      return "?";
    return kDayFull[i];
  }

  const char* StatusName(LessonStatus s)
  {
    switch (s) {
    case LessonStatus::Planned:
      return "Запланирован";
    case LessonStatus::Done:
      return "Проведён";
    case LessonStatus::Cancelled:
      return "Отменён";
    case LessonStatus::Conflict:
      return "Конфликт";
    }
    return "?";
  }

  Teacher* ScheduleState::FindTeacher(Id id)
  {
    for (auto& t : teachers)
      if (t.id == id)
        return &t;
    return nullptr;
  }

  SchoolClass* ScheduleState::FindClass(Id id)
  {
    for (auto& c : classes)
      if (c.id == id)
        return &c;
    return nullptr;
  }

  const Teacher* ScheduleState::FindTeacher(Id id) const
  {
    for (auto& t : teachers)
      if (t.id == id)
        return &t;
    return nullptr;
  }

  const SchoolClass* ScheduleState::FindClass(Id id) const
  {
    for (auto& c : classes)
      if (c.id == id)
        return &c;
    return nullptr;
  }

} // namespace tt
