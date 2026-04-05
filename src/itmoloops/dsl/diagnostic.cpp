#include "itmoloops/dsl/diagnostic.h"
#include "itmoloops/dsl/source.h"
#include <string>
#include <utility>
#include <vector>
#include <ostream>

namespace itmoloops::dsl{
  void Diagnostics::Error(Location where, std::string message){
    diags_.push_back({DiagLevel::kError, std::move(message), where});
  }

  void Diagnostics::Warn(Location where, std::string message){
    diags_.push_back({DiagLevel::kWarning, std::move(message), where});
  }

  bool Diagnostics::Ok() const{
    return Errors() == 0;
  }

  int Diagnostics::Errors() const{
    int num_of_errors = 0;
    for(auto& i : diags_){
      num_of_errors += i.level == DiagLevel::kError;
    }
    return num_of_errors;
  }

  const std::vector<Diagnostic>& Diagnostics::All() const {
    return diags_;
  }

  void Diagnostics::PrintTo(std::ostream& out, const Diagnostics& ds){
    for(auto& i : ds.diags_){
      out << (i.level == DiagLevel::kError ? "error" : "warning") <<
      " (" << i.where.col << ':' << i.where.line << ") " << i.message << '\n';
    }
  }

} // namespace itmoloops::dsl