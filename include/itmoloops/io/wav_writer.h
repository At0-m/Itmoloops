#ifndef ITMOLOOPS_IO_WAV_WRITER_H_
#define ITMOLOOPS_IO_WAV_WRITER_H_

#include <string>
#include <vector>

namespace itmoloops::io {
  bool WriteWavPcm16Mono(const std::string& path, int sample_rate,
                         const std::vector<float>& mono);
} // namespace itmoloops::io

#endif // ITMOLOOPS_IO_WAV_WRITER_H_