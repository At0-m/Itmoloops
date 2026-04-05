#ifndef ITMOLOOPS_DSL_DIAGNOSTICS_H_
#define ITMOLOOPS_DSL_DIAGNOSTICS_H_

#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "itmoloops/dsl/source.h"

namespace itmoloops::dsl{

  enum class DiagLevel {
    kError, kWarning
  };

  struct Diagnostic{
    DiagLevel level = DiagLevel::kError;
    std::string message;
    Location where;
  };

  class Diagnostics{
  public:
    void Error(Location where, std::string message);
    void Warn(Location where, std::string message);

    bool Ok() const;
    int Errors() const;

    const std::vector<Diagnostic>& All() const;
    static void PrintTo(std::ostream& out, const Diagnostics& ds);
  private:
    std::vector<Diagnostic> diags_;
  };

} // namespace itmoloops::dsl


#endif // ITMOLOOPS_DSL_DIAGNOSTICS_H_