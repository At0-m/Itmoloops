#include "itmoloops/core/scheduler.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "itmoloops/util/parse_utils.h"
#include "itmoloops/core/note_event.h"
#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsl/diagnostic.h"
#include "itmoloops/dsl/pitch.h"

#include "itmoloops/core/note_event.h"

using itmoloops::util::ParseDoubleStrict;

namespace itmoloops::core{
namespace {
  struct NoteEventLess {
    bool operator()(const NoteEvent& a, const NoteEvent& b) const {
      if (a.start_sec == b.start_sec) return a.instrument_id < b.instrument_id;
      return a.start_sec < b.start_sec;
    }
  };
} // namespace 

  double Scheduler::BeatSec(double bpm){
    return (bpm > 0.0) ? (60.0 / bpm) : 0.0;
  }

  double Scheduler::UnitsToSec(int64_t units, int pattern_res, double beat_sec){
    if(pattern_res <= 0) return 0.0;
    return static_cast<double>(units) / static_cast<double>(pattern_res) * beat_sec;
  }

  int Scheduler::FindPatternIndex(const dsl::Module& m, const std::string& name) {
    for (std::size_t i = 0; i < m.patterns.size(); ++i) {
      if (m.patterns[i].name == name) return i;
    }
    return -1;
  }

  std::unordered_map<std::string, int> Scheduler::BuildInstrumentIndex(const dsl::Module& m) {
    std::unordered_map<std::string, int> idx;
    for (std::size_t i = 0; i < m.instruments.size(); ++i) {
      idx.emplace(m.instruments[i].name, i);
    }
    return idx;
  }
 
  double Scheduler::MaxReleaseTailSec(const dsl::Module& m) {
    double tail = 0.0;
    for (const auto& inst : m.instruments) {
      auto it = inst.params.find("release");
      if (it == inst.params.end()) continue;

      double v = 0.0;
      if (ParseDoubleStrict(it->second, &v) && v > 0.0) {
        if (v > tail) tail = v;
      }
    }
    return tail;
  } 

  double Scheduler::TimelineEndSec(const std::vector<NoteEvent>& ev) {
    double end = 0.0;
    for (const auto& e : ev) {
      double t = e.start_sec + e.dur_sec;
      if (t > end) end = t;
    }
    return end;
  }


  void Scheduler::ExpandPattern(const dsl::Module& m, int pattern_idx,
                                double offset_sec, double beat_sec,
                                const std::unordered_map<std::string, int>& inst_index,
                                std::vector<NoteEvent>* out, dsl::Diagnostics* diags,
                                int depth) const {
    constexpr int kMaxDepth = 1024;
    if (pattern_idx < 0 || pattern_idx >= static_cast<int>(m.patterns.size())) return;
    if (depth > kMaxDepth) {
      diags->Error({1, 1}, "pattern expansion depth limit exceeded");
      return;
    }

    const dsl::Pattern& pat = m.patterns[pattern_idx];
    for (const auto& up : pat.body) {
      const dsl::NoteStmt* n = dynamic_cast<const dsl::NoteStmt*>(up.get());
      if (n) {
        ExpandNote(pat, *n, offset_sec, beat_sec, inst_index, out, diags);
        continue;
      }
      const dsl::CallStmt* c = dynamic_cast<const dsl::CallStmt*>(up.get());
      if (c) {
        ExpandCall(pat, *c, offset_sec, beat_sec, m, inst_index, out, diags, depth + 1);
      }
    }
  }

  void Scheduler::ExpandNote(const dsl::Pattern& pat, const dsl::NoteStmt& n,
                             double base_offset_sec, double beat_sec,
                             const std::unordered_map<std::string, int>& inst_index,
                             std::vector<NoteEvent>* out,  dsl::Diagnostics* diags) const{
    auto it = inst_index.find(n.instrument);
    if(it == inst_index.end()){
      diags->Error(n.where.begin, "unknown instrument: " + n.instrument);
      return;
    }

    auto midi_opt = dsl::ParsePitchToMidi(n.pitch);
    if(!midi_opt.has_value()){
      diags->Warn(n.where.begin, "unrecognized pitch: " + n.pitch);
      return;
    }

    NoteEvent ev;
    ev.instrument_id = it->second;
    ev.midi = *midi_opt;
    ev.start_sec = base_offset_sec + UnitsToSec(n.start_units, pat.resolution, beat_sec);
    ev.dur_sec = UnitsToSec(n.duration_units, pat.resolution, beat_sec);
    ev.velocity = n.velocity;
    out->push_back(ev);
  }

  void Scheduler::ExpandCall(const dsl::Pattern& caller, const dsl::CallStmt& c,
                             double base_offset_sec, double beat_sec,
                             const dsl::Module& m, const std::unordered_map<std::string, int>& inst_index,
                             std::vector<NoteEvent>* out, dsl::Diagnostics* diags,
                             int depth) const {
    int callee_idx = FindPatternIndex(m, c.callee);
    if (callee_idx < 0) {
      diags->Error(c.where.begin, "unknown pattern: " + c.callee);
      return;
    }
    double call_off = UnitsToSec(c.start_units, caller.resolution, beat_sec);
    ExpandPattern(m, callee_idx, base_offset_sec + call_off, beat_sec, inst_index, out, diags, depth);
  }

  RenderPlan Scheduler::BuildPlan(const dsl::Module& module, dsl::Diagnostics* diags) const {
    RenderPlan plan;
    plan.instrument_index = BuildInstrumentIndex(module);

    int main_idx = FindPatternIndex(module, "main");
    if (main_idx < 0) {
      diags->Error({1, 1}, "pattern 'main' is required");
      return plan;
    }

    const double beat_sec = BeatSec(module.bpm);
    std::vector<NoteEvent> events;
    ExpandPattern(module, main_idx, 0.0, beat_sec, plan.instrument_index, &events, diags, 0);

    std::sort(events.begin(), events.end(), NoteEventLess());

    plan.duration_sec = TimelineEndSec(events) + MaxReleaseTailSec(module);
    plan.events = events;
    return plan;
  }

} // namespace itmoloops::core