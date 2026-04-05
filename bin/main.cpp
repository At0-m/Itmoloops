#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstddef>

#include "itmoloops/cli/argparser.h"

#include "itmoloops/core/renderer.h"
#include "itmoloops/core/scheduler.h"

#include "itmoloops/dsl/diagnostic.h"
#include "itmoloops/dsl/parser.h"
#include "itmoloops/dsl/validator.h"
#include "itmoloops/dsl/ast.h"

#include "itmoloops/dsp/builtin_register.h"
#include "itmoloops/dsp/registries.h"

#include "itmoloops/io/wav_writer.h"
#include "itmoloops/core/note_event.h"

namespace {

    bool ReadFile(const std::string& path, std::string* out) {
        if (!out) return false;
        std::ifstream in(path.c_str(), std::ios::binary);
        if (!in) return false;

        in.seekg(0, std::ios::end);
        std::streamoff sz = in.tellg();
        if (sz < 0) sz = 0;
        in.seekg(0, std::ios::beg);

        out->assign(static_cast<std::size_t>(sz), '\0');
        if (sz > 0) in.read(&(*out)[0], sz);
        return in.good() || in.eof();
        }

        std::string DirName(const std::string& path) {
        std::size_t pos = path.find_last_of("/\\");
        return (pos == std::string::npos) ? "." : path.substr(0, pos);
        }

        std::string DefaultOutPath(const std::string& in) {
        std::size_t slash = in.find_last_of("/\\");
        std::size_t dot   = in.find_last_of('.');
        if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
            return in + ".wav";
        return in.substr(0, dot) + ".wav";
        }


        int PrintDiags(const itmoloops::dsl::Diagnostics& d, int code) {
        itmoloops::dsl::Diagnostics::PrintTo(std::cerr, d);
        return code;
    }

}  // namespace

int main(int argc, const char* const argv[]) {
  itmoloops::cli::Parser cli_parser((argc > 0) ? argv[0] : "itmoloops");
  itmoloops::cli::ParseResult pr = cli_parser.Parse(argc, argv);

  if (pr.show_help) {
    cli_parser.PrintHelp(std::cout);
    return 0;
  }
  if (!pr.Ok()) {
    for (std::size_t i = 0; i < pr.errors.size(); ++i)
      std::cerr << "arg error: " << pr.errors[i] << "\n";
    std::cerr << "\n";
    cli_parser.PrintHelp(std::cerr);
    return 1;
  }
  const itmoloops::cli::Parsed& pars = pr.parsed;

  std::string score_txt;
  if (!ReadFile(pars.score_path, &score_txt)) {
    std::cerr << "cannot read: " << pars.score_path << "\n";
    return 2;
  }

  itmoloops::dsl::Diagnostics diags;
  itmoloops::dsl::Module mod = itmoloops::dsl::Parser::Parse(score_txt, &diags);
  if (!diags.Ok()) return PrintDiags(diags, 3);

  itmoloops::dsl::Validate(&mod, &diags, DirName(pars.score_path));
  if (!diags.Ok()) return PrintDiags(diags, 4);

  itmoloops::core::Scheduler sched;
  itmoloops::core::RenderPlan plan = sched.BuildPlan(mod, &diags);
  if (!diags.Ok()) return PrintDiags(diags, 5);

  if (pars.check_only) {
    itmoloops::dsl::Diagnostics::PrintTo(std::cout, diags);
    std::cout << "OK\n";
    return 0;
  }

  itmoloops::dsp::InstrumentRegistry inst_reg;
  itmoloops::dsp::EffectRegistry     eff_reg;
  itmoloops::dsp::RegisterBuiltinDsp(&inst_reg, &eff_reg);

  itmoloops::core::Renderer renderer(inst_reg, eff_reg);
  itmoloops::core::RenderOptions ropt;
  ropt.sample_rate = static_cast<double>(pars.sample_rate);
  ropt.normalize   = pars.normalize;

  std::vector<float> master;
  if (!renderer.RenderMaster(mod, plan, ropt, &master, &diags))
    return PrintDiags(diags, 6);
  if (!diags.Ok()) return PrintDiags(diags, 7);

  std::string out_path = pars.output_wav.empty() ? DefaultOutPath(pars.score_path)
                                              : pars.output_wav;
  if (!itmoloops::io::WriteWavPcm16Mono(out_path, pars.sample_rate, master)) {
    std::cerr << "cannot write wav: " << out_path << "\n";
    return 8;
  }

  if (pars.verbose) std::cout << "wrote " << out_path << "\n";
  return 0;
}