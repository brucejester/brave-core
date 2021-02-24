/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include <map>
#include <stdexcept>

#include "bat/ads/internal/ml/data/data.h"

namespace ads {
namespace ml {
namespace data {

Data::Data(const DataType& type) : type_(type) {}

Data::~Data() = default;

DataType Data::GetType() const {
  return type_;
}

}  // namespace data
}  // namespace ml
}  // namespace ads