#include "logic/scheduler.h"

#include <algorithm>

namespace tt {

  namespace {

    struct Occupancy {

      std::vector<Lesson>* lessons = nullptr;

      static int Key(Day day, int hour)
      {
        return static_cast<int>(day) * 64 + hour;
      }

      bool TeacherBusy(Id teacher_id, Day day, int hour) const
      {
        for (const auto& l : *lessons) {
          if (l.status == LessonStatus::Cancelled)
            continue;
          if (l.teacher_id == teacher_id && l.day == day && l.hour == hour)
            return true;
        }
        return false;
      }

      bool ClassBusy(Id class_id, Day day, int hour) const
      {
        for (const auto& l : *lessons) {
          if (l.status == LessonStatus::Cancelled)
            continue;
          if (l.class_id == class_id && l.day == day && l.hour == hour)
            return true;
        }
        return false;
      }

      int TeacherLessonsOnDay(Id teacher_id, Day day) const
      {
        int n = 0;
        for (const auto& l : *lessons) {
          if (l.status == LessonStatus::Cancelled)
            continue;
          if (l.teacher_id == teacher_id && l.day == day)
            ++n;
        }
        return n;
      }

      int ClassLessonsOnDay(Id class_id, Day day) const
      {
        int n = 0;
        for (const auto& l : *lessons) {
          if (l.status == LessonStatus::Cancelled)
            continue;
          if (l.class_id == class_id && l.day == day)
            ++n;
        }
        return n;
      }
    };

    bool CheckSlot(const ScheduleState& state, const Occupancy& occ,
        Id teacher_id, Id class_id, Day day, int hour,
        Id ignore_lesson_id, std::string& reason)
    {
      if (!state.DayEnabled(day)) {
        reason = std::string("день отключён (") + DayName(day) + ")";
        return false;
      }
      if (state.DayInPast(day)) {
        reason = std::string("день уже прошёл (") + DayName(day) + ")";
        return false;
      }
      if (hour < state.day_hour_start || hour >= state.day_hour_end) {
        reason = "час вне учебного дня";
        return false;
      }

      const Teacher* t = state.FindTeacher(teacher_id);
      const SchoolClass* c = state.FindClass(class_id);
      if (!t || !c) {
        reason = "нет преподавателя или класса";
        return false;
      }

      for (const auto& l : state.lessons) {
        if (l.id == ignore_lesson_id)
          continue;
        if (l.status == LessonStatus::Cancelled)
          continue;
        if (l.day == day && l.hour == hour) {
          if (l.teacher_id == teacher_id) {
            reason = "преподаватель занят в это время";
            return false;
          }
          if (l.class_id == class_id) {
            reason = "класс занят в это время";
            return false;
          }
        }
      }

      int t_on_day = 0, c_on_day = 0;
      for (const auto& l : state.lessons) {
        if (l.id == ignore_lesson_id)
          continue;
        if (l.status == LessonStatus::Cancelled)
          continue;
        if (l.day != day)
          continue;
        if (l.teacher_id == teacher_id)
          ++t_on_day;
        if (l.class_id == class_id)
          ++c_on_day;
      }
      if (t_on_day >= t->max_lessons_per_day) {
        reason = "превышен лимит уроков в день у преподавателя";
        return false;
      }
      if (c_on_day >= c->max_lessons_per_day) {
        reason = "превышен лимит уроков в день у класса";
        return false;
      }

      (void)occ;
      return true;
    }

  } // namespace

  bool CanPlaceAt(const ScheduleState& state, const Lesson& lesson,
      Day day, int hour, std::string& reason)
  {
    Occupancy occ;
    return CheckSlot(state, occ, lesson.teacher_id, lesson.class_id,
        day, hour, lesson.id, reason);
  }

  ScheduleResult BuildSchedule(ScheduleState& state)
  {
    ScheduleResult result;

    std::vector<Lesson> kept;
    kept.reserve(state.lessons.size());
    for (const auto& l : state.lessons) {
      if (l.locked || l.status == LessonStatus::Done) {
        kept.push_back(l);
      }
    }
    result.kept = static_cast<int>(kept.size());

    auto keptCount = [&](Id tid, Id cid, const std::string& subj) {
      int n = 0;
      for (const auto& l : kept)
        if (l.teacher_id == tid && l.class_id == cid && l.subject_name == subj)
          ++n;
      return n;
    };

    state.lessons = kept;

    Occupancy occ;
    occ.lessons = &state.lessons;

    for (const auto& teacher : state.teachers) {
      for (const auto& sub : teacher.subjects) {
        int already = keptCount(teacher.id, sub.class_id, sub.subject_name);
        int need = sub.hours_per_week - already;

        for (int i = 0; i < need; ++i) {
          std::string last_reason = "нет подходящего слота в желаемом времени";
          bool placed = false;

          for (const auto& slot : teacher.desired_slots) {
            if (placed)
              break;
            for (int hour = slot.hour_from; hour < slot.hour_to; ++hour) {
              std::string reason;
              if (CheckSlot(state, occ, teacher.id, sub.class_id,
                      slot.day, hour, kInvalidId, reason)) {
                Lesson l;
                l.id = state.NewId();
                l.teacher_id = teacher.id;
                l.class_id = sub.class_id;
                l.subject_name = sub.subject_name;
                l.day = slot.day;
                l.hour = hour;
                l.locked = false;
                l.status = state.DayInPast(slot.day)
                    ? LessonStatus::Done
                    : LessonStatus::Planned;
                state.lessons.push_back(l);
                ++result.placed;
                placed = true;
                break;
              } else {
                last_reason = reason;
              }
            }
          }

          if (!placed) {
            Conflict c;
            c.teacher_id = teacher.id;
            c.class_id = sub.class_id;
            c.subject_name = sub.subject_name;
            c.reason = teacher.desired_slots.empty()
                ? "у преподавателя не заданы желаемые слоты"
                : last_reason;
            result.conflicts.push_back(c);
          }
        }
      }
    }

    return result;
  }

  bool RescheduleLesson(ScheduleState& state, Id lesson_id, std::string& reason)
  {
    Lesson* target = nullptr;
    for (auto& l : state.lessons)
      if (l.id == lesson_id) {
        target = &l;
        break;
      }
    if (!target) {
      reason = "урок не найден";
      return false;
    }

    const Teacher* teacher = state.FindTeacher(target->teacher_id);
    if (!teacher) {
      reason = "преподаватель не найден";
      return false;
    }

    Occupancy occ;
    occ.lessons = &state.lessons;

    for (const auto& slot : teacher->desired_slots) {
      for (int hour = slot.hour_from; hour < slot.hour_to; ++hour) {
        std::string r;
        if (CheckSlot(state, occ, target->teacher_id, target->class_id,
                slot.day, hour, target->id, r)) {
          target->day = slot.day;
          target->hour = hour;
          target->status = state.DayInPast(slot.day)
              ? LessonStatus::Done
              : LessonStatus::Planned;
          return true;
        }
      }
    }

    reason = "нет свободного слота для переноса";
    return false;
  }

} // namespace tt
