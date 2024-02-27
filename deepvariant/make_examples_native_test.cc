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

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "deepvariant/pileup_image_native.h"
#include "deepvariant/protos/deepvariant.pb.h"
#include "deepvariant/testing_utils.h"
#include <gmock/gmock-generated-matchers.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>

#include "tensorflow/core/platform/test.h"
#include "absl/strings/string_view.h"
#include "third_party/nucleus/io/reference.h"
#include "third_party/nucleus/protos/variants.pb.h"
#include "third_party/nucleus/util/proto_ptr.h"

namespace learning {
namespace genomics {
namespace deepvariant {

using Variant = nucleus::genomics::v1::Variant;
using Read = nucleus::genomics::v1::Read;
using Range = nucleus::genomics::v1::Range;
using VariantCall = nucleus::genomics::v1::VariantCall;
using ::testing::UnorderedElementsAreArray;
using ::testing::ValuesIn;
using ContigInfo = nucleus::genomics::v1::ContigInfo;
using ReferenceSequence = nucleus::genomics::v1::ReferenceSequence;

struct AltAlleleCombinationsTestData {
  PileupImageOptions_MultiAllelicMode mode;
  Variant variant;
  std::vector<std::vector<std::string>> expected_allele_combinations;
};

class AltAlleleCombinationsTest
    : public testing::TestWithParam<AltAlleleCombinationsTestData> {};

TEST_P(AltAlleleCombinationsTest, AltAlleleCombinationsTestCases) {
  const AltAlleleCombinationsTestData& param = GetParam();
  MakeExamplesOptions options;
  options.mutable_pic_options()->set_multi_allelic_mode(param.mode);
  options.mutable_pic_options()->set_width(21);
  options.mutable_pic_options()->set_num_channels(6);
  options.mutable_pic_options()->set_alt_aligned_pileup("none");
  ExamplesGenerator generator(options, /*test_mode=*/true);

  EXPECT_THAT(
      ExamplesGeneratorPeer::CallAltAlleleCombinations(generator,
                                                       param.variant),
      UnorderedElementsAreArray(param.expected_allele_combinations));
}

INSTANTIATE_TEST_SUITE_P(
    AltAlleleCombinationsTests, AltAlleleCombinationsTest,
    ValuesIn(std::vector<AltAlleleCombinationsTestData>({
        {.mode = deepvariant::PileupImageOptions::ADD_HET_ALT_IMAGES,
         .variant = MakeVariant("A", {"T"}),
         .expected_allele_combinations = {{"T"}}},
        {.mode = deepvariant::PileupImageOptions::ADD_HET_ALT_IMAGES,
         .variant = MakeVariant("AT", {"A"}),
         .expected_allele_combinations = {{"A"}}},
        {.mode = deepvariant::PileupImageOptions::ADD_HET_ALT_IMAGES,
         .variant = MakeVariant("A", {"ATT"}),
         .expected_allele_combinations = {{"ATT"}}},
        {.mode = deepvariant::PileupImageOptions::ADD_HET_ALT_IMAGES,
        .variant = MakeVariant("AT", {"A", "ATT"}),
         .expected_allele_combinations = {{"A"}, {"ATT"}, {"A", "ATT"}}},
        {.mode = deepvariant::PileupImageOptions::NO_HET_ALT_IMAGES,
        .variant = MakeVariant("AT", {"A", "ATT"}),
         .expected_allele_combinations = {{"A"}, {"ATT"}}},
    })));

struct CreateHaplotypeTestData {
  Variant variant;
  std::string alt;
  int64_t expeted_ref_start;
  int64_t expeted_ref_end;
  std::string expected_haplotype;
};

class CreateHaplotypeTest
    : public testing::TestWithParam<CreateHaplotypeTestData> {};

TEST_P(CreateHaplotypeTest, CreateHaplotypeTestCases) {
  const CreateHaplotypeTestData& param = GetParam();
  MakeExamplesOptions options;
  options.mutable_pic_options()->set_width(21);
  options.mutable_pic_options()->set_num_channels(6);
  options.mutable_pic_options()->set_alt_aligned_pileup("none");
  ExamplesGenerator generator(options, /*test_mode=*/true);
  // Create InMemory reference.
  int kNum = 1;
  std::vector<ContigInfo> contigs(kNum);
  std::vector<ReferenceSequence> seqs(kNum);
  CreateTestSeq("chr1", 0, 0, 21, "AGTGGGGGGGGGATGGGGGTG", &contigs, &seqs);
  ExamplesGeneratorPeer::SetRefReader(
      generator,
      std::move(
          nucleus::InMemoryFastaReader::Create(contigs, seqs).ValueOrDie()));

  int64_t ref_start_out = 0;
  int64_t ref_end_out = 0;
  std::string haplotype = ExamplesGeneratorPeer::CallCreateHaplotype(
      generator, param.variant, param.alt, &ref_start_out, &ref_end_out);
  EXPECT_EQ(haplotype, param.expected_haplotype);
  EXPECT_EQ(ref_start_out, param.expeted_ref_start);
  EXPECT_EQ(ref_end_out, param.expeted_ref_end);
};

INSTANTIATE_TEST_SUITE_P(
    CreateHaplotypeTests, CreateHaplotypeTest,
    ValuesIn(std::vector<CreateHaplotypeTestData>({
        {// Variant is in the middle of reference.
         .variant = MakeVariant("G", {"T"}, 10),
         .alt = "T",
         .expeted_ref_start = 0,
         .expeted_ref_end = 21,
         .expected_haplotype = "AGTGGGGGGGTGATGGGGGTG"},
        {// Variant is at the start of the reference.
         .variant = MakeVariant("A", {"T"}, 0),
         .alt = "T",
         .expeted_ref_start = 0,
         .expeted_ref_end = 11,
         .expected_haplotype = "TGTGGGGGGGG"},
        {// Variant is at the end of the reference.
         .variant = MakeVariant("T", {"A"}, 19),
         .alt = "A",
         .expeted_ref_start = 9,
         .expeted_ref_end = 21,
         .expected_haplotype = "GGGATGGGGGAG"},
        {// Variant exceeds half of the window.
         .variant = MakeVariant("A", {"ATATATATATAT"}, 10),
         .alt = "ATATATATATAT",
         .expeted_ref_start = 0,
         .expeted_ref_end = 21,
         .expected_haplotype = "AGTGGGGGGGATATATATATATGATGGGGGTG"},
        {// Variant is DEL.
         .variant = MakeVariant("GAT", {"G"}, 10),
         .alt = "G",
         .expeted_ref_start = 0,
         .expeted_ref_end = 21,
         .expected_haplotype = "AGTGGGGGGGGTGGGGGTG"},
    })));

TEST(GetExamplesFilename, MultiSample) {
  MakeExamplesOptions options;
  options.set_examples_filename("/some/path/to/examples.tfrecord.gz");
  auto make_examples_sample_options = options.mutable_sample_options();
  auto child_sample_options = make_examples_sample_options->Add();
  child_sample_options->set_role("child");
  auto sample_options_parent = make_examples_sample_options->Add();
  sample_options_parent->set_role("parent");
  Sample sample;
  SampleOptions sample_options;
  sample_options.set_role("child");
  sample.sample_options = sample_options;
  // Training mode.
  options.set_mode(MakeExamplesOptions::TRAINING);
  EXPECT_EQ(GetExamplesFilename(options, sample),
            "/some/path/to/examples.tfrecord.gz");
  // Calling mode.
  options.set_mode(MakeExamplesOptions::CALLING);
  EXPECT_EQ(GetExamplesFilename(options, sample),
            "/some/path/to/examples_child.tfrecord.gz");
}

TEST(GetExamplesFilename, SingleSample) {
  MakeExamplesOptions options;
  options.set_examples_filename("/some/path/to/examples.tfrecord.gz");
  auto make_examples_sample_options = options.mutable_sample_options();
  auto main_sample_options = make_examples_sample_options->Add();
  main_sample_options->set_role("main_sample");
  Sample sample;
  SampleOptions sample_options;
  sample_options.set_role("main_sample");
  sample.sample_options = sample_options;
  // Training mode.
  options.set_mode(MakeExamplesOptions::TRAINING);
  EXPECT_EQ(GetExamplesFilename(options, sample),
            "/some/path/to/examples.tfrecord.gz");
  // Calling mode.
  options.set_mode(MakeExamplesOptions::CALLING);
  EXPECT_EQ(GetExamplesFilename(options, sample),
            "/some/path/to/examples.tfrecord.gz");
}

TEST(InMemoryReader, Sanity) {
  std::vector<nucleus::ConstProtoPtr<Read>> input_read_ptrs({{
      nucleus::ConstProtoPtr<Read>(
          new Read(MakeRead("chr1", 100, "AACCTTGGAACCTTGG", {"16M"},
                            "read_1"))),
      nucleus::ConstProtoPtr<Read>(
          new Read(MakeRead("chr1", 110, "AACCTTGGAACCTTGG", {"16M"},
                            "read_2"))),
      nucleus::ConstProtoPtr<Read>(
          new Read(MakeRead("chr1", 120, "AACCTTGGAACCTTGG", {"16M"},
                            "read_3"))),
  }});

  InMemoryReader reader(input_read_ptrs);
  Range range;
  range.set_start(110);
  range.set_end(115);
  range.set_reference_name("chr1");
  std::vector<const Read*> output_reads = reader.Query(range);
  EXPECT_THAT(output_reads, UnorderedElementsAreArray({input_read_ptrs[0].p_,
                                                       input_read_ptrs[1].p_}));

  range.set_start(99);
  range.set_end(100);
  output_reads = reader.Query(range);
  EXPECT_THAT(output_reads,
              UnorderedElementsAreArray(std::vector<const Read*>()));
  for (nucleus::ConstProtoPtr<Read> read : input_read_ptrs) {
    delete read.p_;
  }
}

TEST(ExamplesGenerator, NeedAlignmentAltAlignedIndels) {
  MakeExamplesOptions options;
  options.mutable_pic_options()->set_width(21);
  options.mutable_pic_options()->set_num_channels(6);
  options.mutable_pic_options()->set_alt_aligned_pileup("diff_channels");
  options.mutable_pic_options()->set_types_to_alt_align("indels");
  ExamplesGenerator generator(options, /*test_mode=*/true);

  // SNP - not alt alignment.
  EXPECT_FALSE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("A", {"T"}, 10)));
  // INS - need alt alignement.
  EXPECT_TRUE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("A", {"AT"}, 10)));
  // DEL - need alt alignement.
  EXPECT_TRUE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("AC", {"A"}, 10)));
  // SNP and INS - need alt alignment.
  EXPECT_TRUE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("A", {"C", "AT"}, 10)));
}

TEST(ExamplesGenerator, NeedAlignmentAltAlignedAll) {
  MakeExamplesOptions options;
  options.mutable_pic_options()->set_width(21);
  options.mutable_pic_options()->set_num_channels(6);
  options.mutable_pic_options()->set_alt_aligned_pileup("diff_channels");
  options.mutable_pic_options()->set_types_to_alt_align("all");
  ExamplesGenerator generator(options, /*test_mode=*/true);

  // SNP - need alt alignment.
  EXPECT_TRUE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("A", {"T"}, 10)));
  // INS - need alt alignement.
  EXPECT_TRUE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("A", {"AT"}, 10)));
  // DEL - need alt alignement.
  EXPECT_TRUE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("AC", {"A"}, 10)));
  // SNP and INS - need alt alignment.
  EXPECT_TRUE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("A", {"C", "AT"}, 10)));
}

TEST(ExamplesGenerator, NeedAlignmentAltAlignedNone) {
  MakeExamplesOptions options;
  options.mutable_pic_options()->set_width(21);
  options.mutable_pic_options()->set_num_channels(6);
  options.mutable_pic_options()->set_alt_aligned_pileup("none");
  ExamplesGenerator generator(options, /*test_mode=*/true);

  // SNP - need alt alignment.
  EXPECT_FALSE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("A", {"T"}, 10)));
  // INS - need alt alignement.
  EXPECT_FALSE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("A", {"AT"}, 10)));
  // DEL - need alt alignement.
  EXPECT_FALSE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("AC", {"A"}, 10)));
  // SNP and INS - need alt alignment.
  EXPECT_FALSE(ExamplesGeneratorPeer::NeedAltAlignment(
      generator, MakeVariant("A", {"C", "AT"}, 10)));
}

std::unique_ptr<ImageRow> MakeImageRow(
    const std::vector<std::vector<unsigned char>>& data, int width,
    int num_channels) {
  ImageRow row(width, num_channels);
  int channel_index = 0;
  for (const std::vector<unsigned char>& data_row : data) {
    std::vector<unsigned char> channel_data_row;
    channel_data_row.assign(data_row.begin(), data_row.end());
    row.channel_data[channel_index].assign(data_row.begin(), data_row.end());
    channel_index++;
  }
  return std::make_unique<ImageRow>(row);
}

TEST(FillPileupArray, TestCases) {
  int num_channels = 7;
  int width = 5;
  std::vector<std::unique_ptr<ImageRow>> input;
  input.emplace_back(MakeImageRow({{11, 11, 11, 11, 11},
                                   {21, 21, 21, 21, 21},
                                   {31, 31, 31, 31, 31},
                                   {41, 41, 41, 41, 41},
                                   {51, 51, 51, 51, 51},
                                   {61, 61, 61, 61, 61},
                                   {71, 71, 71, 71, 71}},
                                  width, num_channels));
  input.emplace_back(MakeImageRow({{12, 12, 12, 12, 12},
                                   {22, 22, 22, 22, 22},
                                   {32, 32, 32, 32, 32},
                                   {42, 42, 42, 42, 42},
                                   {52, 52, 52, 52, 52},
                                   {62, 62, 62, 62, 62},
                                   {72, 72, 72, 72, 72}},
                                  width, num_channels));
  input.emplace_back(MakeImageRow({{13, 13, 13, 13, 13},
                                   {23, 23, 23, 23, 23},
                                   {33, 33, 33, 33, 33},
                                   {43, 43, 43, 43, 43},
                                   {53, 53, 53, 53, 53},
                                   {63, 63, 63, 63, 63},
                                   {73, 73, 73, 73, 73}},
                                  width, num_channels));
  // In order to distinguish channel 8 and 9 "-5" is added to channel 8 and
  // "+5" is added to channel 9.
  std::vector<std::vector<std::unique_ptr<ImageRow>>> alt_rows(2);
  alt_rows[0].emplace_back(
      MakeImageRow({{11 - 5, 11 - 5, 11 - 5, 11 - 5, 11 - 5},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {61 - 5, 61 - 5, 61 - 5, 61 - 5, 61 - 5},
                    {1, 1, 1, 1, 1}},
                   width, num_channels));
  alt_rows[0].emplace_back(
      MakeImageRow({{12 - 5, 12 - 5, 12 - 5, 12 - 5, 12 - 5},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {62 - 5, 62 - 5, 62 - 5, 62 - 5, 62 - 5},
                    {1, 1, 1, 1, 1}},
                   width, num_channels));
  alt_rows[0].emplace_back(
      MakeImageRow({{13 - 5, 13 - 5, 13 - 5, 13 - 5, 13 - 5},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {63 - 5, 63 - 5, 63 - 5, 63 - 5, 63 - 5},
                    {1, 1, 1, 1, 1}},
                   width, num_channels));
  std::vector<std::unique_ptr<ImageRow>> alt_rows_2;
  alt_rows[1].emplace_back(
      MakeImageRow({{11 + 5, 11 + 5, 11 + 5, 11 + 5, 11 + 5},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {61 + 5, 61 + 5, 61 + 5, 61 + 5, 61 + 5},
                    {1, 1, 1, 1, 1}},
                   width, num_channels));
  alt_rows[1].emplace_back(
      MakeImageRow({{12 + 5, 12 + 5, 12 + 5, 12 + 5, 12 + 5},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {62 + 5, 62 + 5, 62 + 5, 62 + 5, 62 + 5},
                    {1, 1, 1, 1, 1}},
                   width, num_channels));
  alt_rows[1].emplace_back(
      MakeImageRow({{13 + 5, 13 + 5, 13 + 5, 13 + 5, 13 + 5},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {1, 1, 1, 1, 1},
                    {63 + 5, 63 + 5, 63 + 5, 63 + 5, 63 + 5},
                    {1, 1, 1, 1, 1}},
                   width, num_channels));

  std::vector<unsigned char> expected_alt_align_none = {
      11, 21, 31, 41, 51, 61, 71, 11, 21, 31, 41, 51, 61, 71, 11, 21, 31, 41,
      51, 61, 71, 11, 21, 31, 41, 51, 61, 71, 11, 21, 31, 41, 51, 61, 71,

      12, 22, 32, 42, 52, 62, 72, 12, 22, 32, 42, 52, 62, 72, 12, 22, 32, 42,
      52, 62, 72, 12, 22, 32, 42, 52, 62, 72, 12, 22, 32, 42, 52, 62, 72,

      13, 23, 33, 43, 53, 63, 73, 13, 23, 33, 43, 53, 63, 73, 13, 23, 33, 43,
      53, 63, 73, 13, 23, 33, 43, 53, 63, 73, 13, 23, 33, 43, 53, 63, 73,
  };
  std::vector<unsigned char> expected_alt_align_base = {
      11, 21, 31, 41, 51, 61, 71, 11 - 5, 11 + 5,
      11, 21, 31, 41, 51, 61, 71, 11 - 5, 11 + 5,
      11, 21, 31, 41, 51, 61, 71, 11 - 5, 11 + 5,
      11, 21, 31, 41, 51, 61, 71, 11 - 5, 11 + 5,
      11, 21, 31, 41, 51, 61, 71, 11 - 5, 11 + 5,

      12, 22, 32, 42, 52, 62, 72, 12 - 5, 12 + 5,
      12, 22, 32, 42, 52, 62, 72, 12 - 5, 12 + 5,
      12, 22, 32, 42, 52, 62, 72, 12 - 5, 12 + 5,
      12, 22, 32, 42, 52, 62, 72, 12 - 5, 12 + 5,
      12, 22, 32, 42, 52, 62, 72, 12 - 5, 12 + 5,

      13, 23, 33, 43, 53, 63, 73, 13 - 5, 13 + 5,
      13, 23, 33, 43, 53, 63, 73, 13 - 5, 13 + 5,
      13, 23, 33, 43, 53, 63, 73, 13 - 5, 13 + 5,
      13, 23, 33, 43, 53, 63, 73, 13 - 5, 13 + 5,
      13, 23, 33, 43, 53, 63, 73, 13 - 5, 13 + 5,
  };
  std::vector<unsigned char> expected_alt_align_diff = {
      11, 21, 31, 41, 51, 61, 71, 61 - 5, 61 + 5,
      11, 21, 31, 41, 51, 61, 71, 61 - 5, 61 + 5,
      11, 21, 31, 41, 51, 61, 71, 61 - 5, 61 + 5,
      11, 21, 31, 41, 51, 61, 71, 61 - 5, 61 + 5,
      11, 21, 31, 41, 51, 61, 71, 61 - 5, 61 + 5,

      12, 22, 32, 42, 52, 62, 72, 62 - 5, 62 + 5,
      12, 22, 32, 42, 52, 62, 72, 62 - 5, 62 + 5,
      12, 22, 32, 42, 52, 62, 72, 62 - 5, 62 + 5,
      12, 22, 32, 42, 52, 62, 72, 62 - 5, 62 + 5,
      12, 22, 32, 42, 52, 62, 72, 62 - 5, 62 + 5,

      13, 23, 33, 43, 53, 63, 73, 63 - 5, 63 + 5,
      13, 23, 33, 43, 53, 63, 73, 63 - 5, 63 + 5,
      13, 23, 33, 43, 53, 63, 73, 63 - 5, 63 + 5,
      13, 23, 33, 43, 53, 63, 73, 63 - 5, 63 + 5,
      13, 23, 33, 43, 53, 63, 73, 63 - 5, 63 + 5,
  };

  // alt_aligned_pileup = none
  std::vector<unsigned char> pileup_image;
  FillPileupArray(input, {}, AltAlignedPileup::kNone, &pileup_image);
  EXPECT_THAT(pileup_image, testing::ElementsAreArray(expected_alt_align_none));

  pileup_image.clear();
  FillPileupArray(input, alt_rows, AltAlignedPileup::kNone, &pileup_image);
  EXPECT_THAT(pileup_image, UnorderedElementsAreArray(expected_alt_align_none));

  // alt_aligned_pileup = diff_channels
  pileup_image.clear();
  FillPileupArray(input, alt_rows, AltAlignedPileup::kDiffChannels,
                  &pileup_image);
  EXPECT_THAT(pileup_image, UnorderedElementsAreArray(expected_alt_align_diff));

  // alt_aligned_pileup = base_channels
  pileup_image.clear();
  FillPileupArray(input, alt_rows, AltAlignedPileup::kBaseChannels,
                  &pileup_image);
  EXPECT_THAT(pileup_image, UnorderedElementsAreArray(expected_alt_align_base));
}

}  // namespace deepvariant
}  // namespace genomics
}  // namespace learning
