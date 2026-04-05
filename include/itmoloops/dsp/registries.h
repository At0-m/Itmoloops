#ifndef ITMOLOOPS_DSP_REGISTRIES_H_
#define ITMOLOOPS_DSP_REGISTRIES_H_

#include <memory>
#include <new>
#include <string>
#include <unordered_map>

#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsp/effect.h"
#include "itmoloops/dsp/instrument.h"

namespace itmoloops::dsp {
    
  using InstCreateFn = std::unique_ptr<IInstrument>(*)(const dsl::InstrumentSpec&);
  using EffCreateFn = std::unique_ptr<IEffect>(*)(const dsl::EffectSpec&);

  class InstrumentRegistry {
  public:
    bool Register(const std::string& type, InstCreateFn fn);

    std::unique_ptr<IInstrument> Create(const std::string& type,
                                        const dsl::InstrumentSpec& spec) const;

    bool Contains(const std::string& type) const;

  private:
    std::unordered_map<std::string, InstCreateFn> map_;
  };

  class EffectRegistry {
  public:
    bool Register(const std::string& type, EffCreateFn fn);

    std::unique_ptr<IEffect> Create(const std::string& type,
                                    const dsl::EffectSpec& spec) const;

    bool Contains(const std::string& type) const;

  private:
    std::unordered_map<std::string, EffCreateFn> map_;
  };


  template <class T>
  std::unique_ptr<IInstrument> CreateInstrumentImpl(const dsl::InstrumentSpec& spec) {
    T* ptr = new (std::nothrow) T(spec);
    return std::unique_ptr<IInstrument>(ptr);
  }

  template <class T>
  std::unique_ptr<IEffect> CreateEffectImpl(const dsl::EffectSpec& spec) {
    T* ptr = new (std::nothrow)T(spec);
    return std::unique_ptr<IEffect>(ptr);
  }

  template <class T>
  bool RegisterInstrument(InstrumentRegistry* reg, const char* type) {
    if (!reg) return false;
    return reg->Register(type, &CreateInstrumentImpl<T>);
  }

  template <class T>
  bool RegisterEffect(EffectRegistry* reg, const char* type) {
    if (!reg) return false;
    return reg->Register(type, &CreateEffectImpl<T>);
  }

}  // namespace itmoloops::dsp

#endif  // ITMOLOOPS_DSP_REGISTRIES_H_