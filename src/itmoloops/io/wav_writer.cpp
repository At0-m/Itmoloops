#include "itmoloops/io/wav_writer.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <cstddef>

namespace itmoloops::io{
namespace {

  bool WriteFourcc(std::ofstream* out, const char* s4) {
    if (!out || !out->good()) return false;
    out->write(s4, 4);
    return out->good();
  }

  bool WriteLE16(std::ofstream* out, std::uint16_t v) {
    if (!out || !out->good()) return false;
    char b[2];
    b[0] = static_cast<char>(v & 0xFF);
    b[1] = static_cast<char>((v >> 8) & 0xFF);
    out->write(b, 2);
    return out->good();
  }

  bool WriteLE32(std::ofstream* out, std::uint32_t v) {
    if (!out || !out->good()) return false;
    char b[4];
    b[0] = static_cast<char>(v & 0xFF);
    b[1] = static_cast<char>((v >> 8) & 0xFF);
    b[2] = static_cast<char>((v >> 16) & 0xFF);
    b[3] = static_cast<char>((v >> 24) & 0xFF);
    out->write(b, 4);
    return out->good();
  }

  std::int16_t FloatToPcm16(float x) {
    if (x >= 1.0f) return 32767;
    if (x <= -1.0f) return -32768;
    float scaled = x * 32767.0f;
    int v = (scaled >= 0.0f) ? static_cast<int>(scaled + 0.5f)
                             : static_cast<int>(scaled - 0.5f);
    if (v > 32767) v = 32767;
    if (v < -32768) v = -32768;
    return static_cast<std::int16_t>(v);
  }

  bool WriteWavHeader(std::ofstream* out, int sample_rate, std::uint32_t data_bytes) {
    const std::uint16_t audio_format_pcm = 1;
    const std::uint16_t num_channels = 1;
    const std::uint16_t bits_per_sample = 16;
    const std::uint16_t block_align = num_channels * (bits_per_sample / 8);
    const std::uint32_t byte_rate = static_cast<std::uint32_t>(sample_rate) * block_align;

    const std::uint32_t fmt_size = 16;
    const std::uint32_t riff_size = 4 + (8 + fmt_size) + (8 + data_bytes);

    if (!WriteFourcc(out, "RIFF")) return false;
    if (!WriteLE32(out, riff_size)) return false;
    if (!WriteFourcc(out, "WAVE")) return false;

    if (!WriteFourcc(out, "fmt ")) return false;
    if (!WriteLE32(out, fmt_size)) return false;
    if (!WriteLE16(out, audio_format_pcm)) return false;
    if (!WriteLE16(out, num_channels)) return false;
    if (!WriteLE32(out, static_cast<std::uint32_t>(sample_rate))) return false;
    if (!WriteLE32(out, byte_rate)) return false;
    if (!WriteLE16(out, block_align)) return false;
    if (!WriteLE16(out, bits_per_sample)) return false;

    if (!WriteFourcc(out, "data")) return false;
    if (!WriteLE32(out, data_bytes)) return false;
    return out->good();
  }
} // namespace 

  bool WriteWavPcm16Mono(const std::string& path, int sample_rate,
                         const std::vector<float>& mono) {
    if (sample_rate <= 0) return false;

    std::ofstream out(path.c_str(), std::ios::binary);
    if (!out) return false;

    const std::uint32_t data_bytes = static_cast<std::uint32_t>(mono.size() * sizeof(std::int16_t));
    if (!WriteWavHeader(&out, sample_rate, data_bytes)) return false;

    for (std::size_t i = 0; i < mono.size(); ++i) {
      const std::int16_t s = FloatToPcm16(mono[i]);
      char b[2];
      b[0] = static_cast<char>(s & 0xFF);
      b[1] = static_cast<char>((s >> 8) & 0xFF);
      out.write(b, 2);
      if (!out.good()) return false;
    }
    return out.good();
  }
} // namespace itmoloops::io