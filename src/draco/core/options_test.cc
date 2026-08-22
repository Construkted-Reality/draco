// Copyright 2026 The Draco Authors.
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
//
#include "draco/core/options.h"

#include "draco/core/draco_test_base.h"

namespace {

// The quantization origin that ExpertEncoder stores for a tile whose bounding
// box starts at 4.369 m on a grid of 2^-8 m. Its unit in the last place is
// 4.77e-7, which is smaller than the 1e-6 that six decimal places can hold.
TEST(OptionsTest, FloatRoundTripsExactly) {
  draco::Options options;
  const float value = 4.3671875f;
  options.SetFloat("origin", value);
  ASSERT_EQ(options.GetFloat("origin"), value);
}

TEST(OptionsTest, FloatVectorRoundTripsExactly) {
  draco::Options options;
  const float origin[3] = {4.3671875f, 169.6171875f, -92.15234375f};
  options.SetVector("origin", origin, 3);
  float read_back[3] = {0.f, 0.f, 0.f};
  ASSERT_TRUE(options.GetVector("origin", 3, read_back));
  for (int i = 0; i < 3; ++i) {
    ASSERT_EQ(read_back[i], origin[i]) << "component " << i;
  }
}

// Values whose unit in the last place is above 1e-6 always round-tripped.
// Guard them so the fix does not regress the easy case.
TEST(OptionsTest, LargeFloatRoundTripsExactly) {
  draco::Options options;
  const float value = -49.99609375f;
  options.SetFloat("range", value);
  ASSERT_EQ(options.GetFloat("range"), value);
}

TEST(OptionsTest, IntAndBoolAreUnaffected) {
  draco::Options options;
  options.SetInt("bits", 14);
  options.SetBool("enabled", true);
  ASSERT_EQ(options.GetInt("bits"), 14);
  ASSERT_TRUE(options.GetBool("enabled"));
}

}  // namespace
