#include "itmoloops/dsl/validator.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string_view>

#include "itmoloops/util/parse_utils.h"
#include "itmoloops/dsl/pitch.h"
#include "itmoloops/dsl/source.h"
#include "itmoloops/dsl/diagnostic.h"
#include "itmoloops/dsl/ast.h"


using namespace itmoloops::util;

namespace itmoloops::dsl{
namespace {

  void ValidateCommonAR(const std::unordered_map<std::string, std::string>& params,
                        const Location& where, Diagnostics* diags) {
    auto it = params.find("attack");
    if (it != params.end()) {
      double v = 0.0;
      if (!ParseDoubleStrict(it->second, &v) || v < 0.0)
        diags->Error(where, "attack must be non-negative");
    }
    it = params.find("release");
    if (it != params.end()) {
      double v = 0.0;
      if (!ParseDoubleStrict(it->second, &v) || v < 0.0)
        diags->Error(where, "release must be non-negative");
    }
  }

  void ValidateSampler(InstrumentSpec* inst, std::string_view score_dir,
                       Diagnostics* diags) {
    auto& p = inst->params;

    auto sample_it = p.find("sample");
    if (sample_it == p.end()) {
      diags->Error(inst->where.begin, "sampler: sample is required");
    } else {
      std::string full = ResolveSamplePath(score_dir, sample_it->second);
      if (!FileExists(full)) {
        diags->Error(inst->where.begin, "sampler: sample file does not exist: " + full);
      } else {
        p["sample"] = full;
      }
    }

    auto root_it = p.find("root");
    if (root_it == p.end()) {
      diags->Error(inst->where.begin, "sampler: root note is required");
    } else if (!ParsePitchToMidi(root_it->second).has_value()) {
      diags->Error(inst->where.begin, "sampler: invalid root pitch: " + root_it->second);
    }

    auto loop_it = p.find("loop");
    if (loop_it != p.end()) {
      int64_t a = 0;
      int64_t b = 0;
      if (!ParseI64Pair(loop_it->second, &a, &b) || a < 0 || b <= a) {
        diags->Error(inst->where.begin, "sampler: loop must be 'start, end' with 0 <= start < end");
      }
    }

    ValidateCommonAR(p, inst->where.begin, diags);
  }

  void ValidateSquare(const InstrumentSpec& inst, Diagnostics* diags) {
    const auto& p = inst.params;
    auto it = p.find("duty");
    if (it != p.end()) {
      double duty = 0.0;
      if (!ParseDoubleStrict(it->second, &duty) || duty < 0.0 || duty > 100.0)
        diags->Error(inst.where.begin, "square: duty must be in [0..100]");
    }
    ValidateCommonAR(p, inst.where.begin, diags);
  }

  void ValidateSineTriangle(const InstrumentSpec& inst, Diagnostics* diags) {
    ValidateCommonAR(inst.params, inst.where.begin, diags);
  }

  const std::string* RequireEffectParam(const EffectSpec& e, const char* key,
                                        Diagnostics* diags) {
    auto it = e.params.find(key);
    if (it == e.params.end()) {
      diags->Error(e.where.begin, std::string("effect ") + e.type + ": " + key + " is required");
      return nullptr;
    }
    return &it->second;
  }

  void ValidateEffect(const EffectSpec& e, Diagnostics* diags) {
    if (e.type == "gain") {
      const std::string* g = RequireEffectParam(e, "gain", diags);
      if (!g) return;
      double v = 0.0;
      if (!ParseDoubleStrict(*g, &v)) diags->Error(e.where.begin, "gain: invalid 'gain' value");
      return;
    }

    if (e.type == "echo") {
      const std::string* d = RequireEffectParam(e, "delay", diags);
      const std::string* c = RequireEffectParam(e, "decay", diags);
      if (!d || !c) return;

      double dv = 0.0, cv = 0.0;
      if (!ParseDoubleStrict(*d, &dv) || dv < 0.0) {
        diags->Error(e.where.begin, "echo: 'delay' must be non-negative seconds");
      }
      if (!ParseDoubleStrict(*c, &cv)) {
        diags->Error(e.where.begin, "echo: 'decay' must be a number");
      }
      return;
    }

    if (e.type == "tremolo") {
      const std::string* f   = RequireEffectParam(e, "freq", diags);
      const std::string* dep = RequireEffectParam(e, "depth", diags);
      if (!f || !dep) return;

      double fv = 0.0, dv = 0.0;
      if (!ParseDoubleStrict(*f, &fv) || fv <= 0.0){
        diags->Error(e.where.begin, "tremolo: freq must be > 0");
      }
      if (!ParseDoubleStrict(*dep, &dv) || dv < 0.0 || dv > 1.0) {
        diags->Error(e.where.begin, "tremolo: depth must be in [0..1]");
      }
      return;
    }

    diags->Warn(e.where.begin, "unknown effect type: " + e.type);
  }

  void ValidateTopLevelBpm(const Module& m, Diagnostics* diags) {
    if (!m.bpm_set) diags->Error({1, 1}, "bpm must be specified once at file top");
    if (m.bpm <= 0.0) diags->Error({1, 1}, "bpm must be > 0");
  }

  auto BuildAndValidateInstruments(Module* m, Diagnostics* diags, std::string_view score_dir) {
    std::unordered_map<std::string, int> inst_index;
    for (std::size_t i = 0; i < m->instruments.size(); ++i) {
      auto& in = m->instruments[i];

      if (inst_index.count(in.name)) diags->Error(in.where.begin, "duplicate instrument name: " + in.name);
      else inst_index[in.name] = i;

      if (in.type == "sampler") ValidateSampler(&in, score_dir, diags);
      else if (in.type == "square") ValidateSquare(in, diags);
      else if (in.type == "sine" || in.type == "triangle") ValidateSineTriangle(in, diags);
      else diags->Warn(in.where.begin, "unknown instrument type: " + in.type);

      for (const auto& fx : in.effects) ValidateEffect(fx, diags);
    }
    return inst_index;
  }

  auto BuildPatternsAndCheckMain(const Module& m, Diagnostics* diags) {
    std::unordered_map<std::string, int> pat_index;
    bool has_main = false;

    for (std::size_t i = 0; i < m.patterns.size(); ++i) {
      const auto& p = m.patterns[i];
      if (pat_index.count(p.name)) diags->Error(p.where.begin, "duplicate pattern name: " + p.name);
      else pat_index[p.name] = i;

      if (p.resolution <= 0) diags->Error(p.where.begin, "pattern resolution must be > 0");
      if (p.name == "main") has_main = true;
    }
    if (!has_main) diags->Error({1, 1}, "pattern 'main' is required");
    return pat_index;
  }

  void ValidatePatternBodies(const Module& m, const std::unordered_map<std::string, int>& inst_index,
                             const std::unordered_map<std::string, int>& pat_index, Diagnostics* diags) {
    for (std::size_t i = 0; i < m.patterns.size(); ++i) {
      const auto& p = m.patterns[i];
      for (const auto& uptr : p.body) {
        const NoteStmt* n = dynamic_cast<const NoteStmt*>(uptr.get());
        if (n) {
          if (!inst_index.count(n->instrument)) diags->Error(n->where.begin, "unknown instrument: " + n->instrument);
          if (n->velocity < 0 || n->velocity > 100) diags->Error(n->where.begin, "velocity must be in [0..100]");
          if (n->duration_units < 0) diags->Error(n->where.begin, "note duration must be >= 0");
          if (!ParsePitchToMidi(n->pitch).has_value()) diags->Warn(n->where.begin, "unrecognized pitch format: " + n->pitch);
          continue;
        }
        const CallStmt* c = dynamic_cast<const CallStmt*>(uptr.get());
        if (c) {
          auto it = pat_index.find(c->callee);
          if (it == pat_index.end()) diags->Error(c->where.begin, "unknown pattern: " + c->callee);
          else if (it->second >= static_cast<int>(i))  diags->Error(c->where.begin, "pattern can be used only after its declaration: " + c->callee);
        }
      }
    }
  }

} // namespace 


  bool Validate(Module* m, Diagnostics* diags, std::string_view score_dir) {
    ValidateTopLevelBpm(*m, diags);

    std::unordered_map<std::string, int> inst_index = BuildAndValidateInstruments(m, diags, score_dir);
    std::unordered_map<std::string, int> pat_index  = BuildPatternsAndCheckMain(*m, diags);

    ValidatePatternBodies(*m, inst_index, pat_index, diags);

    return diags->Ok();
  }

} // namespace itmoloops::dsl
