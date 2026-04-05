#include "itmoloops/dsp/instr/sampler.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <cstddef>
#include <string>
#include <optional>
#include <cstddef>

#include "itmoloops/dsl/pitch.h"
#include "itmoloops/dsl/ast.h"
#include "itmoloops/dsp/oscillator_util.h"
#include "itmoloops/util/parse_utils.h"
#include "itmoloops/core/note_event.h"

namespace itmoloops::dsp {
namespace {

  double NonNegative(double x) { return (x < 0.0) ? 0.0 : x; }

  double GetDoubleParam(const std::unordered_map<std::string, std::string>& p,
                        const char* key, double def) {
    auto it = p.find(key);
    if (it == p.end()) return def;
    double v = def;
    if (!util::ParseDoubleStrict(it->second, &v)) return def;
    return v;
  }

  std::string GetStringParam(const std::unordered_map<std::string, std::string>& p,
                             const char* key) {
    auto it = p.find(key);
    if (it == p.end()) return std::string();
    return it->second;
  }

  bool ReadFourCC(std::ifstream* in, char out[4]) {
    if (!in) return false;
    return static_cast<bool>(in->read(out, 4));
  }

  bool ReadU16LE(std::ifstream* in, std::uint16_t* out) {
    if (!in || !out) return false;
    unsigned char b[2];
    if (!in->read(reinterpret_cast<char*>(b), 2)) return false;
    *out = static_cast<std::uint16_t>(b[0] | (static_cast<std::uint16_t>(b[1]) << 8));
    return true;
  }

  bool ReadU32LE(std::ifstream* in, std::uint32_t* out) {
    if (!in || !out) return false;
    unsigned char b[4];
    if (!in->read(reinterpret_cast<char*>(b), 4)) return false;
    *out = static_cast<std::uint32_t>(b[0] | (static_cast<std::uint32_t>(b[1]) << 8) |
                                             (static_cast<std::uint32_t>(b[2]) << 16) |
                                             (static_cast<std::uint32_t>(b[3]) << 24));
    return true;
  }

  bool SkipBytes(std::ifstream* in, std::uint32_t n) {
    if (!in) return false;
    in->seekg(n, std::ios::cur);
    return static_cast<bool>(*in);
  }

  bool SkipPadByte(std::ifstream* in, std::uint32_t chunk_size) {
    if (!in) return false;
    if ((chunk_size & 1u) == 0u) return true;
    char pad = 0;
    return static_cast<bool>(in->read(&pad, 1));
  }

  bool ReadRiffHeader(std::ifstream* in) {
    char id[4];
    std::uint32_t sz = 0;
    if (!ReadFourCC(in, id)) return false;
    if (std::string(id, 4) != "RIFF") return false;
    if (!ReadU32LE(in, &sz)) return false;
    if (!ReadFourCC(in, id)) return false;
    return std::string(id, 4) == "WAVE";
  }

  bool ReadChunkHeader(std::ifstream* in, char id[4], std::uint32_t* size) {
    if (!ReadFourCC(in, id)) return false;
    return ReadU32LE(in, size);
  }

  bool ReadFmtChunk(std::ifstream* in, std::uint32_t size, WavInfo* fmt) {
    if (!in || !fmt) return false;
    if (size < 16) return false;

    std::uint16_t tag = 0;
    std::uint16_t channels = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t byte_rate = 0;
    std::uint16_t block_align = 0;
    std::uint16_t bits = 0;

    if (!ReadU16LE(in, &tag)) return false;
    if (!ReadU16LE(in, &channels)) return false;
    if (!ReadU32LE(in, &sample_rate)) return false;
    if (!ReadU32LE(in, &byte_rate)) return false;
    if (!ReadU16LE(in, &block_align)) return false;
    if (!ReadU16LE(in, &bits)) return false;

    fmt->format_tag = tag;
    fmt->channels = channels;
    fmt->sample_rate = sample_rate;
    fmt->bits_per_sample = bits;

    if (tag == 0xFFFE && size >= 40) {
      std::uint16_t cb_size = 0;
      std::uint16_t valid_bits = 0;
      std::uint32_t channel_mask = 0;
      unsigned char guid[16];

      if (!ReadU16LE(in, &cb_size)) return false;
      if (!ReadU16LE(in, &valid_bits)) return false;
      if (!ReadU32LE(in, &channel_mask)) return false;
      if (!in->read(reinterpret_cast<char*>(guid), 16)) return false;

      std::uint32_t sub = static_cast<std::uint32_t>(guid[0] | (static_cast<std::uint32_t>(guid[1]) << 8) |
                                                               (static_cast<std::uint32_t>(guid[2]) << 16) |
                                                               (static_cast<std::uint32_t>(guid[3]) << 24));

      if (sub == 1u || sub == 3u) fmt->format_tag = static_cast<std::uint16_t>(sub);
      if (valid_bits != 0) fmt->bits_per_sample = valid_bits;

      const std::uint32_t already = 40;
      if (size > already) {
        if (!SkipBytes(in, size - already)) return false;
      }
      return SkipPadByte(in, size);
    }

    if (size > 16) {
      if (!SkipBytes(in, size - 16)) return false;
    }
    return SkipPadByte(in, size);
  }

  bool ReadDataChunk(std::ifstream* in, std::uint32_t size, std::vector<char>* data) {
    if (!in || !data) return false;
    data->assign(size, 0);
    if (size > 0) {
      if (!in->read(data->data(), static_cast<std::streamsize>(size))) return false;
    }
    return SkipPadByte(in, size);
  }

}  // namespace


  Sampler::Sampler(const dsl::InstrumentSpec& spec) {
    ParseParams(spec);
    const std::string path = GetStringParam(spec.params, "sample");
    if (!path.empty()) LoadSample(path);
    SanitizeLoop();
  }

  void Sampler::ParseParams(const dsl::InstrumentSpec& spec) {
    attack_sec_  = NonNegative(GetDoubleParam(spec.params, "attack", 0.0));
    release_sec_ = NonNegative(GetDoubleParam(spec.params, "release", 0.0));

    const std::string root = GetStringParam(spec.params, "root");
    if (!root.empty()) {
      std::optional<int> m = dsl::ParsePitchToMidi(root);
      if (m.has_value()) root_midi_ = *m;
    }

    const std::string loop = GetStringParam(spec.params, "loop");
    if (!loop.empty()) {
      int64_t a = -1;
      int64_t b = -1;
      if (util::ParseI64Pair(loop, &a, &b) && a >= 0 && b > a) {
        loop_start_ = a;
        loop_end_ = b;
      }
    }
  }

  bool Sampler::LoadSample(const std::string& path) {
    sample_.clear();

    WavInfo info;
    std::vector<char> data;
    if (!ReadWavFile(path, &info, &data)) return false;

    std::vector<float> mono;
    if (!ConvertToMonoFloat(info, data, &mono)) return false;

    sample_.swap(mono);
    return !sample_.empty();
  }

  void Sampler::SanitizeLoop() {
    if (loop_start_ < 0 || loop_end_ <= loop_start_) {
      loop_start_ = -1;
      loop_end_ = -1;
      return;
    }
    const int64_t n = sample_.size();
    if (loop_start_ >= n || loop_end_ > n) {
      loop_start_ = -1;
      loop_end_ = -1;
    }
  }

  double Sampler::Clamp01(double x) {
    if (x < -1.0) return -1.0;
    if (x > 1.0) return 1.0;
    return x;
  }

  bool Sampler::ReadWavFile(const std::string& path, WavInfo* info, std::vector<char>* data) {
    if (!info || !data) return false;
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) return false;
    if (!ReadRiffHeader(&in)) return false;

    bool got_fmt = false;
    bool got_data = false;

    while (in && (!got_fmt || !got_data)) {
      char id[4];
      std::uint32_t size = 0;
      if (!ReadChunkHeader(&in, id, &size)) break;

      const std::string cid(id, 4);
      if (cid == "fmt ") {
        if (!ReadFmtChunk(&in, size, info)) return false;
        got_fmt = true;
        continue;
      }
      if (cid == "data") {
        if (!ReadDataChunk(&in, size, data)) return false;
        got_data = true;
        continue;
      }
      if (!SkipBytes(&in, size)) return false;
      if (!SkipPadByte(&in, size)) return false;
    }

    if (!got_fmt || !got_data) return false;
    if (info->sample_rate == 0) return false;
    if (info->channels == 0) return false;
    return true;
  }

  double ReadPcmNorm(const unsigned char* p, int bits) {
    if (bits == 8) {
      int v = static_cast<int>(p[0]) - 128;
      return static_cast<double>(v) / 128.0;
    }
    if (bits == 16) {
      std::int16_t v = static_cast<std::int16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8));
      return static_cast<double>(v) / 32768.0;
    }
    if (bits == 24) {
      std::int32_t v = static_cast<std::int32_t>(p[0] | (p[1] << 8) | (p[2] << 16));
      if (v & 0x00800000) v |= 0xFF000000;
      return static_cast<double>(v) / 8388608.0;
    }
    if (bits == 32) {
      std::int32_t v = static_cast<std::int32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
      return static_cast<double>(v) / 2147483648.0;
    }
    return 0.0;
  }

  double ReadFloat32Norm(const unsigned char* p) {
    std::uint32_t u = static_cast<std::uint32_t>(p[0] | (static_cast<std::uint32_t>(p[1]) << 8) |
                                                        (static_cast<std::uint32_t>(p[2]) << 16) |
                                                        (static_cast<std::uint32_t>(p[3]) << 24));
    float f = 0.0f;
    std::memcpy(&f, &u, 4);
    return static_cast<double>(f);
  }

  bool Sampler::ConvertToMonoFloat(const WavInfo& info, const std::vector<char>& data,
                                   std::vector<float>* out) {
    if (!out) return false;

    const int fmt = info.format_tag;
    const int ch = info.channels;
    const int bits = info.bits_per_sample;
    if (ch <= 0) return false;

    int bytes_per_sample = 0;
    if (fmt == 1) {
      if (bits == 8) bytes_per_sample = 1;
      else if (bits == 16) bytes_per_sample = 2;
      else if (bits == 24) bytes_per_sample = 3;
      else if (bits == 32) bytes_per_sample = 4;
      else return false;
    } else if (fmt == 3) { 
      if (bits != 32) return false;
      bytes_per_sample = 4;
    } else {
      return false;
    }

    const std::size_t frame_bytes = static_cast<std::size_t>(bytes_per_sample * ch);
    if (frame_bytes == 0) return false;

    const std::size_t frames = data.size() / frame_bytes;
    out->assign(frames, 0.0f);

    for (std::size_t i = 0; i < frames; ++i) {
      const unsigned char* base = reinterpret_cast<const unsigned char*>(data.data() + i * frame_bytes);
      double sum = 0.0;
      for (int c = 0; c < ch; ++c) {
        const unsigned char* p = base + static_cast<std::size_t>(c * bytes_per_sample);
        double x = 0.0;
        if (fmt == 1) x = ReadPcmNorm(p, bits);
        else x = ReadFloat32Norm(p);
        sum += x;
      }
      double mono = sum / static_cast<double>(ch);
      (*out)[i] = static_cast<float>(Clamp01(mono));
    }

    return !out->empty();
  }

  double Sampler::WrapLoop(double pos) const {
    if (loop_start_ < 0 || loop_end_ <= loop_start_) return pos;
    const double a = static_cast<double>(loop_start_);
    const double b = static_cast<double>(loop_end_);
    const double len = b - a;
    if (len <= 1.0) return a;
    if (pos < b) return pos;
    double x = std::fmod(pos - a, len);
    if (x < 0.0) x += len;
    return a + x;
  }

  float Sampler::SampleAt(double pos) const {
    if (sample_.empty()) return 0.0f;
    if (pos < 0.0) pos = 0.0;

    if (loop_start_ >= 0) pos = WrapLoop(pos);

    const int64_t n = static_cast<int64_t>(sample_.size());
    if (n == 1) return sample_[0];

    int64_t i0 = static_cast<int64_t>(pos);
    if (i0 < 0) i0 = 0;

    if (loop_start_ < 0) {
      if (i0 >= n) return 0.0f;
      int64_t i1 = i0 + 1;
      if (i1 >= n) i1 = n - 1;
      const double frac = pos - static_cast<double>(i0);
      const float s0 = sample_[i0];
      const float s1 = sample_[i1];
      return static_cast<float>(static_cast<double>(s0) + (static_cast<double>(s1) - static_cast<double>(s0)) * frac);
    }

    if (i0 >= n) return 0.0f;
    int64_t i1 = i0 + 1;
    if (i1 >= loop_end_) i1 = loop_start_;
    if (i1 < 0 || i1 >= n) return 0.0f;

    const double frac = pos - static_cast<double>(i0);
    const float s0 = sample_[i0];
    const float s1 = sample_[i1];
    return static_cast<float>(static_cast<double>(s0) + (static_cast<double>(s1) - static_cast<double>(s0)) * frac);
  }

  void Sampler::Render(const std::vector<core::NoteEvent>& notes, double out_sample_rate,
                      std::vector<float>* out) {
    if (!out || out->empty()) return;
    if (sample_.empty()) return;
    if (out_sample_rate <= 0.0) return;

    for (std::size_t i = 0; i < notes.size(); ++i) {
      RenderOneNote(notes[i], out_sample_rate, out);
    }
  }

  void Sampler::RenderOneNote(const core::NoteEvent& e, double out_sample_rate,
                              std::vector<float>* out) const {
    if (!out || out->empty()) return;
    if (sample_.empty()) return;

    const int64_t dst_n = out->size();
    int64_t dst_start = static_cast<int64_t>(e.start_sec * out_sample_rate + 0.5);
    if (dst_start < 0) dst_start = 0;
    if (dst_start >= dst_n) return;

    const double total_sec = e.dur_sec + release_sec_;
    if (total_sec <= 0.0) return;

    int64_t dst_count = static_cast<int64_t>(total_sec * out_sample_rate + 0.5);
    if (dst_count <= 0) return;

    int64_t dst_end = dst_start + dst_count;
    if (dst_end > dst_n) dst_end = dst_n;
    dst_count = dst_end - dst_start;
    if (dst_count <= 0) return;

    const double vel = VelocityToGain(e.velocity);
    const double semis = static_cast<double>(e.midi - root_midi_) / 12.0;
    const double pitch_ratio = std::pow(2.0, semis);

    const double sr_ratio = static_cast<double>(sample_rate_) / out_sample_rate;
    const double step = pitch_ratio * sr_ratio;
    if (step <= 0.0) return;

    double src = 0.0;
    const double src_limit = static_cast<double>(sample_.size() - 1);

    for (int64_t i = 0; i < dst_count; ++i) {
      const double t = static_cast<double>(i) / out_sample_rate;
      const double env = EnvelopeAR(t, e.dur_sec, attack_sec_, release_sec_);
      const float s = SampleAt(src);
      (*out)[dst_start + i] += static_cast<float>(static_cast<double>(s) * env * vel);

      src += step; 
      if (loop_start_ < 0 && src >= src_limit) break;
    }
  }

}  // namespace itmoloops::dsp