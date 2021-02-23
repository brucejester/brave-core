/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <cmath>
#include <fstream>
#include <vector>

#include "bat/ads/internal/ml/data/data.h"
#include "bat/ads/internal/ml/data/text_data.h"
#include "bat/ads/internal/ml/data/vector_data.h"
#include "bat/ads/internal/ml/model/linear/linear.h"
#include "bat/ads/internal/ml/pipeline/text_processing/text_processing.h"
#include "bat/ads/internal/ml/transformation/hashed_ngrams.h"
#include "bat/ads/internal/ml/transformation/lowercase.h"
#include "bat/ads/internal/ml/transformation/transformation.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ml {

namespace {

const char kHashingModel[] = "hashing_model_min.json";

const char kInvalidModel[] = "invalid_model.json";

const char kTextCMCCrash[] = "text_cmc_crash.txt";

const char kPipelineSpam[] = "pipeline_spam.json";

}  // namespace

class BatAdsTextProcessingPipelineTest : public UnitTestBase {
 protected:
  BatAdsTextProcessingPipelineTest() = default;

  ~BatAdsTextProcessingPipelineTest() override = default;
};

TEST_F(BatAdsTextProcessingPipelineTest, BuildSimplePipeline) {
  // Arrange
  const double kTolerance = 1e-6;
  const unsigned kExpectedLen = 3;
  const std::string kTestString = "Test String";

  TransformationVector transformations;
  transformation::Lowercase lowercase;
  transformations.push_back(
      std::make_unique<transformation::Lowercase>(lowercase));
  transformation::HashedNGrams hashed_ngrams(3, std::vector<int>{1, 2, 3});
  transformations.push_back(
      std::make_unique<transformation::HashedNGrams>(hashed_ngrams));

  std::map<std::string, data::VectorData> weights = {
      {"class_1", data::VectorData(std::vector<double>{1.0, 2.0, 3.0})},
      {"class_2", data::VectorData(std::vector<double>{3.0, 2.0, 1.0})},
      {"class_3", data::VectorData(std::vector<double>{2.0, 2.0, 2.0})}};

  std::map<std::string, double> biases = {
      {"class_1", 0.0}, {"class_2", 0.0}, {"class_3", 0.0}};

  model::Linear linear_model(weights, biases);
  pipeline::TextProcessing pipeline =
      pipeline::TextProcessing(transformations, linear_model);

  data::VectorData data_point_3(std::vector<double>{1.0, 0.0, 0.0});

  // Act
  PredictionMap data_point_3_res = linear_model.Predict(data_point_3);
  PredictionMap res = pipeline.GetTopPredictions(kTestString);

  // Assert
  ASSERT_EQ(data_point_3_res.size(), kExpectedLen);
  ASSERT_TRUE(res.size() && res.size() <= kExpectedLen);
  for (auto const& pred : res) {
    EXPECT_TRUE(pred.second > -kTolerance && pred.second < 1.0 + kTolerance);
  }
}

TEST_F(BatAdsTextProcessingPipelineTest, TestLoadFromJson) {
  // Arrange
  std::vector<std::string> train_texts = {
      "This is a spam email.", "Another spam trying to sell you viagra",
      "Message from mom with no real subject",
      "Another messase from mom with no real subject", "Yadayada"};
  std::vector<std::string> train_labels = {"spam", "spam", "ham", "ham",
                                           "junk"};

  const base::Optional<std::string> opt_value =
      ReadFileFromTestPathToString(kPipelineSpam);
  pipeline::TextProcessing pipeline;

  // Act
  ASSERT_TRUE(opt_value.has_value());
  const std::string json = opt_value.value();
  bool load_success = pipeline.FromJson(json);
  ASSERT_TRUE(load_success);

  std::vector<PredictionMap> prediction_maps(train_texts.size());
  for (size_t i = 0; i < train_texts.size(); i++) {
    std::unique_ptr<data::Data> text_data =
        std::make_unique<data::TextData>(data::TextData(train_texts[i]));
    PredictionMap prediction_map = pipeline.Apply(text_data);
    prediction_maps[i] = prediction_map;
  }

  // Assert
  for (size_t i = 0; i < prediction_maps.size(); i++) {
    PredictionMap& prediction_map = prediction_maps[i];
    for (auto const& pred : prediction_map) {
      double other_prediction = pred.second;
      EXPECT_TRUE(prediction_map[train_labels[i]] >= other_prediction);
    }
  }
}

TEST_F(BatAdsTextProcessingPipelineTest, InitValidModelTest) {
  // Arrange
  pipeline::TextProcessing text_proc_pipeline;
  const base::Optional<std::string> opt_value =
      ReadFileFromTestPathToString(kHashingModel);

  // Act
  ASSERT_TRUE(opt_value.has_value());
  const std::string model_json = opt_value.value();
  bool loaded_successfully = text_proc_pipeline.FromJson(model_json);

  // Assert
  EXPECT_TRUE(loaded_successfully);
}

TEST_F(BatAdsTextProcessingPipelineTest, InvalidModelTest) {
  // Arrange
  pipeline::TextProcessing text_proc_pipeline;
  const base::Optional<std::string> opt_value =
      ReadFileFromTestPathToString(kInvalidModel);

  // Act
  ASSERT_TRUE(opt_value.has_value());
  const std::string model_json = opt_value.value();
  bool loaded_successfully = text_proc_pipeline.FromJson(model_json);

  // Assert
  EXPECT_FALSE(loaded_successfully);
}

TEST_F(BatAdsTextProcessingPipelineTest, EmptyModelTest) {
  // Arrange
  pipeline::TextProcessing text_proc_pipeline;
  const std::string empty_model_json = "{}";

  // Act
  bool loaded_successfully = text_proc_pipeline.FromJson(empty_model_json);

  // Assert
  EXPECT_FALSE(loaded_successfully);
}

TEST_F(BatAdsTextProcessingPipelineTest, MissingModelTest) {
  // Arrange
  pipeline::TextProcessing text_proc_pipeline;

  // Act
  const std::string missing_model_json = "";
  bool loaded_successfully = text_proc_pipeline.FromJson(missing_model_json);

  // Assert
  EXPECT_FALSE(loaded_successfully);
}

TEST_F(BatAdsTextProcessingPipelineTest, TopPredUnitTest) {
  // Arrange
  const std::string kTestPage = "ethereum bitcoin bat zcash crypto tokens!";
  pipeline::TextProcessing text_proc_pipeline;
  const base::Optional<std::string> opt_value =
      ReadFileFromTestPathToString(kHashingModel);

  // Act
  ASSERT_TRUE(opt_value.has_value());
  const std::string model_json = opt_value.value();
  ASSERT_TRUE(text_proc_pipeline.FromJson(model_json));
  PredictionMap preds = text_proc_pipeline.ClassifyPage(kTestPage);

  // Assert
  ASSERT_TRUE(preds.size());
  ASSERT_LT(preds.size(), static_cast<size_t>(100));
  ASSERT_TRUE(preds.count("crypto-crypto"));
  for (auto const& pred : preds) {
    EXPECT_TRUE(pred.second <= preds["crypto-crypto"]);
  }
}

TEST_F(BatAdsTextProcessingPipelineTest, TextCMCCrashTest) {
  // Arrange
  pipeline::TextProcessing text_proc_pipeline;

  const base::Optional<std::string> opt_value =
      ReadFileFromTestPathToString(kHashingModel);
  ASSERT_TRUE(opt_value.has_value());

  const std::string model_json = opt_value.value();
  ASSERT_TRUE(text_proc_pipeline.FromJson(model_json));

  const base::Optional<std::string> opt_text_value =
      ReadFileFromTestPathToString(kTextCMCCrash);

  // Act
  ASSERT_TRUE(opt_text_value.has_value());
  const std::string bad_text = opt_text_value.value();
  PredictionMap preds = text_proc_pipeline.ClassifyPage(bad_text);

  // Assert
  ASSERT_LT(preds.size(), static_cast<size_t>(100));
  ASSERT_GT(preds.size(), static_cast<size_t>(2));
  ASSERT_TRUE(preds.count("personal finance-personal finance"));
  for (auto const& pred : preds) {
    EXPECT_TRUE(pred.second <= preds["personal finance-personal finance"]);
  }
}

}  // namespace ml
}  // namespace ads
