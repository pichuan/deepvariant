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

#include "third_party/nucleus/util/math.h"
#include "third_party/pybind11/include/pybind11/stl.h"

namespace py = pybind11;

PYBIND11_MODULE(math, m) {
  using namespace ::nucleus;
  m.def("log10_ptrue_to_phred", &Log10PTrueToPhred, py::arg("log10_ptrue"),
        py::arg("value_if_not_finite"));
  m.def("phred_to_perror", &PhredToPError, py::arg("phred"));
  m.def("phred_to_log10_perror", &PhredToLog10PError, py::arg("phred"));
  m.def("perror_to_log10_perror", &PErrorToLog10PError, py::arg("perror"));
  m.def("perror_to_phred", &PErrorToPhred, py::arg("perror"));
  m.def("log10_perror_to_phred", &Log10PErrorToPhred, py::arg("log10_perror"));
  m.def("perror_to_rounded_phred", &PErrorToRoundedPhred, py::arg("perror"));
  m.def("log10_perror_to_rounded_phred", &Log10PErrorToRoundedPhred,
        py::arg("log10_perror"));
  m.def("log10_perror_to_perror", &Log10ToReal, py::arg("log10_perror"));
  m.def("zero_shift_log10_probs", &ZeroShiftLikelihoods,
        py::arg("log10_probs"));
}