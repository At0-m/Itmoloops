#ifndef ITMOLOOPS_CORE_NOTE_EVENT_H_
#define ITMOLOOPS_CORE_NOTE_EVENT_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace itmoloops::core{

  struct NoteEvent{
    int instrument_id = -1;
    int midi = 60;
    double start_sec = 0.0;
    double dur_sec = 0.0;
    int velocity = 0;
  };

  struct RenderPlan{
    std::vector<NoteEvent> events;
    double duration_sec = 0.0;
    std::unordered_map<std::string, int> instrument_index;
  };

} // namespace itmoloops::core

#endif // ITMOLOOPS_CORE_NOTE_EVENT_H_