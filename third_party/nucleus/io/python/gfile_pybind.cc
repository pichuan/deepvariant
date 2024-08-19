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

#include "third_party/nucleus/io/gfile.h"
#include "third_party/pybind11/include/pybind11/stl.h"

namespace py = pybind11;

PYBIND11_MODULE(gfile, m) {
  using namespace ::nucleus;
  py::class_<WritableFile>(m, "WritableFile")
      .def_static("New", &WritableFile::New, py::arg("filename"))
      .def("write", &WritableFile::Write, py::arg("s"))
      .def("close", &WritableFile::Close)
      .def("__enter__", &WritableFile::PythonEnter)
      .def("__exit__", &WritableFile::Close);

  py::class_<ReadableFile>(m, "ReadableFile")
      .def_static("New", &ReadableFile::New, py::arg("filename"))
      // This one doesn't pass the test, and I don't know why yet.
      .def("Readline", &ReadableFile::Readline)
      .def("close", &ReadableFile::Close)
      .def("__enter__", &ReadableFile::PythonEnter)
      .def("__exit__", &ReadableFile::Close);
  m.def("Exists", &Exists, py::arg("filename"));
  m.def("Glob", &Glob, py::arg("pattern"));
}