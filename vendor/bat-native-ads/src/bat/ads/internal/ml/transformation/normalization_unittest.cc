/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <vector>

#include "bat/ads/internal/ml/data/text_data.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/data/vector_data_aliases.h"
#include "bat/ads/internal/ml/ml_aliases.h"
#include "bat/ads/internal/ml/ml_static_values.h"
#include "bat/ads/internal/ml/transformation/hashed_ngrams.h"
#include "bat/ads/internal/ml/transformation/lowercase.h"
#include "bat/ads/internal/ml/transformation/normalization.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ml {

class BatAdsNormalizationTest : public UnitTestBase {
 protected:
  BatAdsNormalizationTest() = default;

  ~BatAdsNormalizationTest() override = default;
};

TEST_F(BatAdsNormalizationTest, NormalizationTest) {
  // Arrange
  const double kTolerance = 1e-7;

  std::string kTestString = "quite a small test string";
  data::TextData text_data(kTestString);
  std::unique_ptr<data::Data> data =
      std::make_unique<data::TextData>(text_data);

  transformation::HashedNGrams hashed_ngrams(10, std::vector<int>{3, 4});
  transformation::Normalization normalization;

  // Act
  data = hashed_ngrams.Apply(data);

  data = normalization.Apply(data);

  ASSERT_EQ(data->GetType(), data::DataType::VECTOR_DATA);

  data::VectorData* norm_data = static_cast<data::VectorData*>(data.release());

  std::vector<double> components;
  double s = 0.0;
  for (data::SparseVectorElement const& x : norm_data->GetRawData()) {
    components.push_back(x.second);
    s += x.second * x.second;
  }

  // Assert
  for (double const& x : components) {
    ASSERT_TRUE(x >= 0.0);
    ASSERT_TRUE(x <= 1.0);
  }
  EXPECT_TRUE(std::fabs(s - 1.0) < kTolerance);
}

TEST_F(BatAdsNormalizationTest, ChainingTest) {
  // Arrange
  const size_t kExpectedElementCount = 10;
  const std::string kTestString = "TINY";

  TransformationVector chain;

  transformation::Lowercase lowercase;
  chain.push_back(std::make_unique<transformation::Lowercase>(lowercase));

  transformation::HashedNGrams hashed_ngrams;
  chain.push_back(
      std::make_unique<transformation::HashedNGrams>(hashed_ngrams));

  transformation::Normalization normalization;
  chain.push_back(
      std::make_unique<transformation::Normalization>(normalization));

  data::TextData text_data(kTestString);
  std::unique_ptr<data::Data> data =
      std::make_unique<data::TextData>(text_data);

  // Act
  for (size_t i = 0; i < chain.size(); ++i) {
    data = chain[i]->Apply(data);
  }

  ASSERT_EQ(data->GetType(), data::DataType::VECTOR_DATA);
  data::VectorData* vect_data = static_cast<data::VectorData*>(data.get());

  // Assert
  EXPECT_EQ(vect_data->GetDimensionCount(), kNumBuckets);

  // Hashes for [t, i, n, y, ti, in, ny, tin, iny, tiny] -- 10 in total
  EXPECT_EQ(vect_data->GetRawData().size(), kExpectedElementCount);
}

}  // namespace ml
}  // namespace ads