/*
 * Copyright 2023 Google LLC.
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
 */

#include "deepvariant/make_examples_native.h"

#include <string>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "third_party/nucleus/io/reference.h"


namespace learning {
namespace genomics {
namespace deepvariant {

using Variant = nucleus::genomics::v1::Variant;

ExamplesGenerator::ExamplesGenerator(const MakeExamplesOptions& options,
                                     bool test_mode)
    : options_(options) {
  if (test_mode) {
    return;
  }
  // Initialize reference reader.
  // We should always have reference name set up, except for some unit tests.
  // This code will fail if reference is not set up or cannot be loaded.
  nucleus::genomics::v1::FastaReaderOptions fasta_reader_options;
  std::string fasta_path = options_.reference_filename();
  fasta_reader_options.set_keep_true_case(false);
  std::string fai_path = absl::StrCat(fasta_path, ".fai");
  ref_reader_ = std::move(
      nucleus::IndexedFastaReader::FromFile(fasta_path, fai_path).ValueOrDie());
}

std::vector<std::vector<std::string>> ExamplesGenerator::AltAlleleCombinations(
    const Variant& variant) const {
  std::vector<std::vector<std::string>> alt_combinations;
  switch (options_.pic_options().multi_allelic_mode()) {
    case deepvariant::PileupImageOptions::UNSPECIFIED:
      LOG(FATAL) << "multi_allelic_mode cannot be UNSPECIFIED";
      break;
    case deepvariant::PileupImageOptions::NO_HET_ALT_IMAGES:
      for (const std::string& alt : variant.alternate_bases()) {
        alt_combinations.push_back({alt});
      }
      break;
    case deepvariant::PileupImageOptions::ADD_HET_ALT_IMAGES:
      {
        std::vector<std::string> alts = {variant.reference_bases()};
        alts.insert(alts.end(), variant.alternate_bases().begin(),
                    variant.alternate_bases().end());
        for (int i = 0; i < alts.size(); ++i) {
          for (int j = i + 1; j < alts.size(); ++j) {
            std::vector<std::string> one_combination;
            // Ref allele is not used in combinations.
            if (i > 0) {
              one_combination.push_back(alts[i]);
            }
            one_combination.push_back(alts[j]);
            alt_combinations.push_back(std::move(one_combination));
          }
        }
      }
      break;
    default:
      LOG(FATAL) << "Unknown value is specified for PileupImageOptions";
  }
  return alt_combinations;
}

}  // namespace deepvariant
}  // namespace genomics
}  // namespace learning
