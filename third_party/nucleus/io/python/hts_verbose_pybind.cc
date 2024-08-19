/*
 * Copyright 2024 Google LLC.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#if true  // Trick to stop tooling from moving the #include around.
// MUST appear before any standard headers are included.
#include <pybind11/pybind11.h>
#endif

#include "third_party/nucleus/io/hts_verbose.h"

namespace py = pybind11;

PYBIND11_MODULE(hts_verbose, m) {
  using namespace ::nucleus;
  py::enum_<htsLogLevel>(m, "htsLogLevel")
      .value("HTS_LOG_DEBUG", htsLogLevel::HTS_LOG_DEBUG)
      .value("HTS_LOG_ERROR", htsLogLevel::HTS_LOG_ERROR)
      .value("HTS_LOG_INFO", htsLogLevel::HTS_LOG_INFO)
      .value("HTS_LOG_OFF", htsLogLevel::HTS_LOG_OFF)
      .value("HTS_LOG_TRACE", htsLogLevel::HTS_LOG_TRACE)
      .value("HTS_LOG_WARNING", htsLogLevel::HTS_LOG_WARNING);
  m.def("get", &HtsGetLogLevel);
  m.def("set", &HtsSetLogLevel, py::arg("level"));
}