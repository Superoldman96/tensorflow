// Copyright 2024 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tensorflow/lite/experimental/litert/cc/litert_compiled_model.h"

#include <cstring>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_log.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "tensorflow/lite/experimental/litert/cc/litert_model.h"
#include "tensorflow/lite/experimental/litert/test/common.h"

constexpr const float kTestInput0Tensor[] = {1, 2};
constexpr const size_t kTestInput0Size =
    sizeof(kTestInput0Tensor) / sizeof(kTestInput0Tensor[0]);
constexpr const float kTestInput1Tensor[] = {10, 20};
constexpr const size_t kTestInput1Size =
    sizeof(kTestInput1Tensor) / sizeof(kTestInput1Tensor[0]);
constexpr const float kTestOutputTensor[] = {11, 22};
constexpr const size_t kTestOutputSize =
    sizeof(kTestOutputTensor) / sizeof(kTestOutputTensor[0]);

namespace litert {
namespace {

using ::testing::FloatNear;
using ::testing::Pointwise;

static constexpr absl::string_view kTfliteFile = "simple_model.tflite";

TEST(CompiledModelTest, Basic) {
  auto model = testing::LoadTestFileModel(kTfliteFile);
  ASSERT_TRUE(model);
  auto res_compiled_model = CompiledModel::Create(model);
  ASSERT_TRUE(res_compiled_model) << "Failed to initialize CompiledModel";
  auto& compiled_model = *res_compiled_model;
  auto signatures = model.GetSignatures().Value();
  EXPECT_EQ(signatures.size(), 1);
  auto signature_key = signatures[0].Key();
  EXPECT_EQ(signature_key, Model::DefaultSignatureKey());
  size_t signature_index = 0;

  auto input_buffers_res = compiled_model.CreateInputBuffers(signature_index);
  EXPECT_TRUE(input_buffers_res);
  auto& input_buffers = *input_buffers_res;

  auto output_buffers_res = compiled_model.CreateOutputBuffers(signature_index);
  EXPECT_TRUE(output_buffers_res);
  auto& output_buffers = *output_buffers_res;

  // Fill model inputs.
  auto input_names = signatures[0].InputNames();
  EXPECT_EQ(input_names.size(), 2);
  EXPECT_EQ(input_names.at(0), "arg0");
  EXPECT_EQ(input_names.at(1), "arg1");
  ASSERT_TRUE(input_buffers[0].Write<float>(
      absl::MakeConstSpan(kTestInput0Tensor, kTestInput0Size)));
  ASSERT_TRUE(input_buffers[1].Write<float>(
      absl::MakeConstSpan(kTestInput1Tensor, kTestInput1Size)));

  // Execute model.
  compiled_model.Run(signature_index, input_buffers, output_buffers);

  // Check model output.
  auto output_names = signatures[0].OutputNames();
  EXPECT_EQ(output_names.size(), 1);
  EXPECT_EQ(output_names.at(0), "tfl.add");
  float output_buffer_data[kTestOutputSize];
  auto output_span = absl::MakeSpan(output_buffer_data, kTestOutputSize);
  ASSERT_TRUE(output_buffers[0].Read(output_span));
  for (auto i = 0; i < kTestOutputSize; ++i) {
    ABSL_LOG(INFO) << "Result: " << output_span.at(i) << "\t"
                   << kTestOutputTensor[i];
  }
  EXPECT_THAT(output_span, Pointwise(FloatNear(1e-5), kTestOutputTensor));
}

}  // namespace
}  // namespace litert
