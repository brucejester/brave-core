/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_CRYPTO_DOT_COM_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_CRYPTO_DOT_COM_API_H_

#include <map>
#include <string>

#include "extensions/browser/extension_function.h"

class Profile;

namespace extensions {
namespace api {

class CryptoDotComGetTickerInfoFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getTickerInfo", UNKNOWN)

 protected:
  ~CryptoDotComGetTickerInfoFunction() override {}
  void OnInfoResult(const std::map<std::string, std::string>& info);

  ResponseAction Run() override;
};

class CryptoDotComGetChartDataFunction :
    public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("cryptoDotCom.getChartData", UNKNOWN)

 protected:
  ~CryptoDotComGetChartDataFunction() override {}
  void OnChartDataResult(const std::map<std::string, std::string>& data);

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_CRYPTO_DOT_COM_API_H_
