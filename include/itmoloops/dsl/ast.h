#ifndef ITMOLOOPS_DSL_AST_H_
#define ITMOLOOPS_DSL_AST_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "itmoloops/dsl/source.h"

namespace itmoloops::dsl {
  struct EffectSpec{
    std::string type;
    std::unordered_map<std::string, std::string> params;
    Range where;
  };

  struct InstrumentSpec{
    std::string name;
    std::string type;
    std::unordered_map<std::string, std::string> params;
    std::vector<EffectSpec> effects;
    Range where;
  };

  struct Stmt{
    virtual ~Stmt() = default;
    Range where;
  };

  struct NoteStmt : public Stmt {
    int64_t start_units = 0;
    std::string instrument;
    std::string pitch;
    int64_t duration_units = 0;
    int velocity = 0;
  };

  struct CallStmt : public Stmt{
    int64_t start_units = 0;
    std::string callee;
  };

  struct Pattern {
    std::string name;
    int resolution = 1;
    std::vector<std::unique_ptr<Stmt>> body;
    Range where;
  };

  struct Module{
    bool bpm_set = false;
    double bpm = 120.0;
    std::vector<InstrumentSpec> instruments;
    std::vector<Pattern> patterns;
  };
}  // namespace itmoloops::dsl


#endif // ITMOLOOPS_DSL_LEXER_H_
