#include "itmoloops/dsp/registries.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "itmoloops/dsp/instrument.h"
#include "itmoloops/dsp/effect.h"
#include "itmoloops/dsl/ast.h"

namespace itmoloops::dsp {

  bool InstrumentRegistry::Register(const std::string& type, InstCreateFn fn) {
    if (!fn) return false;
    auto it = map_.find(type);
    if (it == map_.end()) {
      map_.emplace(type, fn);
      return true;
    }
    it->second = fn;
    return false;
  }

  std::unique_ptr<IInstrument>  InstrumentRegistry::Create(const std::string& type, const dsl::InstrumentSpec& spec) const {
    auto it = map_.find(type);
    if (it == map_.end()) return std::unique_ptr<IInstrument>();
    return (it->second)(spec);
  }

  bool InstrumentRegistry::Contains(const std::string& type) const {
    return map_.find(type) != map_.end();
  }

  bool EffectRegistry::Register(const std::string& type, EffCreateFn fn) {
    if (!fn) return false;
    auto it = map_.find(type);
    if (it == map_.end()) {
      map_.emplace(type, fn);
      return true;
    }
    it->second = fn;
    return false;
  }

  std::unique_ptr<IEffect> EffectRegistry::Create(const std::string& type, const dsl::EffectSpec& spec) const {
    auto it = map_.find(type);
    if (it == map_.end()) return std::unique_ptr<IEffect>();
    return (it->second)(spec);
  }

  bool EffectRegistry::Contains(const std::string& type) const {
    return map_.find(type) != map_.end();
  }

} // namespace itmoloops::dsp