#ifndef ITMOLOOPS_DSL_VALIDATOR_H_
#define ITMOLOOPS_DSL_VALIDATOR_H_

#include <string_view>

#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsl/diagnostic.h"

namespace itmoloops::dsl{
  bool Validate(Module* m, Diagnostics* daigs,
                std::string_view score_dit);
} // namespace itmoloops::dsl

#endif