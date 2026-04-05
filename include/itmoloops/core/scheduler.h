#ifndef ITMOLOOPS_CORE_SCHEDULER_H_
#define ITMOLOOPS_CORE_SCHEDULER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "itmoloops/core/note_event.h"
#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsl/diagnostic.h"
#include "itmoloops/dsl/pitch.h"

namespace itmoloops::core{

  class Scheduler {
  public:
    RenderPlan BuildPlan(const dsl::Module& module, dsl::Diagnostics* diags) const;
  private:
    static double BeatSec(double bpm);
    static double UnitsToSec(int64_t units, int pattern_res, double beat_sec);
    static int FindPatternIndex(const dsl::Module& m, const std::string& name);
    static std::unordered_map<std::string, int> BuildInstrumentIndex(const dsl::Module& m);
    static double MaxReleaseTailSec(const dsl::Module& m);
    static double TimelineEndSec(const std::vector<NoteEvent>& ev);

    void ExpandPattern(const dsl::Module& m, int pattern_idx,
                       double offset_sec, double beat_sec,
                       const std::unordered_map<std::string, int>& inst_index,
                       std::vector<NoteEvent>* out, dsl::Diagnostics* diags,
                       int depth) const;

    void ExpandNote(const dsl::Pattern& pat, const dsl::NoteStmt& n,
                    double base_offset_sec, double beat_sec,
                    const std::unordered_map<std::string, int>& inst_index,
                    std::vector<NoteEvent>* out,  dsl::Diagnostics* diags) const;

    void ExpandCall(const dsl::Pattern& caller, const dsl::CallStmt& c,
                    double base_offset_sec, double beat_sec,
                    const dsl::Module& m, const std::unordered_map<std::string, int>& inst_index,
                    std::vector<NoteEvent>* out, dsl::Diagnostics* diags,
                    int depth) const;
  };

} // namespace itmoloops::core

#endif // ITMOLOOPS_CORE_SCHEDULER_H_