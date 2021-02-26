/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "bat/ledger/internal/endpoint/promotion/post_suggestions_claim/post_suggestions_claim.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/request_util.h"
#include "bat/ledger/internal/common/security_util.h"
#include "bat/ledger/internal/credentials/credentials_util.h"
#include "bat/ledger/internal/endpoint/promotion/promotions_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace ledger {
namespace endpoint {
namespace promotion {

PostSuggestionsClaim::PostSuggestionsClaim(LedgerImpl* ledger)
    : ledger_(ledger) {
  DCHECK(ledger_);
}

PostSuggestionsClaim::~PostSuggestionsClaim() = default;

std::string PostSuggestionsClaim::GetUrl() {
  return GetServerUrl("/v1/suggestions/claim");
}

std::string PostSuggestionsClaim::GeneratePayload(
    const credential::CredentialsRedeem& redeem) {
  const auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    return "";
  }

  base::Value credentials(base::Value::Type::LIST);
  credential::GenerateCredentials(redeem.token_list, wallet->payment_id,
                                  &credentials);

  base::Value body(base::Value::Type::DICTIONARY);
  body.SetStringKey("paymentId", wallet->payment_id);
  body.SetKey("credentials", std::move(credentials));

  std::string json;
  base::JSONWriter::Write(body, &json);
  return json;
}

type::Result PostSuggestionsClaim::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid request");
    return type::Result::LEDGER_ERROR;
  }

  if (status_code == net::HTTP_SERVICE_UNAVAILABLE) {
    BLOG(0, "No conversion rate yet in ratios service");
    return type::Result::BAD_REGISTRATION_RESPONSE;
  }

  if (status_code != net::HTTP_OK) {
    return type::Result::LEDGER_ERROR;
  }

  return type::Result::LEDGER_OK;
}

mojo::StructPtr<type::UrlRequest> PostSuggestionsClaim::GetSuggestionRequest(
    const type::BraveWalletPtr wallet,
    const char* path,
    const std::string& payload) {
  auto headers = util::BuildSignHeaders(path, payload, wallet->payment_id,
                                        wallet->recovery_seed);

  auto request = type::UrlRequest::New();
  request->url = GetUrl();
  request->content = payload;
  request->headers = headers;
  request->content_type = "application/json; charset=utf-8";
  request->method = type::UrlMethod::POST;
  return request;
}

void PostSuggestionsClaim::Request(const credential::CredentialsRedeem& redeem,
                                   PostSuggestionsClaimCallback callback) {
  auto url_callback =
      std::bind(&PostSuggestionsClaim::OnRequest, this, _1, callback);

  const std::string payload = GeneratePayload(redeem);

  auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto request = GetSuggestionRequest(std::move(wallet),
                                      "post /v1/suggestions/claim", payload);
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostSuggestionsClaim::RequestV2(
    const credential::CredentialsRedeem& redeem,
    PostSuggestionsClaimCallbackV2 callback) {
  auto url_callback =
      std::bind(&PostSuggestionsClaim::OnRequestV2, this, _1, callback);

  const std::string payload = GeneratePayload(redeem);

  auto wallet = ledger_->wallet()->GetWallet();
  if (!wallet) {
    BLOG(0, "Wallet is null");
    callback(type::Result::LEDGER_ERROR, "");
    return;
  }

  auto request = GetSuggestionRequest(std::move(wallet),
                                      "post /v2/suggestions/claim", payload);
  ledger_->LoadURL(std::move(request), url_callback);
}

void PostSuggestionsClaim::OnRequest(const type::UrlResponse& response,
                                     PostSuggestionsClaimCallback callback) {
  ledger::LogUrlResponse(__func__, response);
  callback(CheckStatusCode(response.status_code));
}

void PostSuggestionsClaim::OnRequestV2(
    const type::UrlResponse& response,
    PostSuggestionsClaimCallbackV2 callback) {
  ledger::LogUrlResponse(__func__, response);
  auto result = CheckStatusCode(response.status_code);
  if (result != type::Result::LEDGER_OK) {
    callback(result, "");
    return;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(response.body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    callback(type::Result::RETRY, "");
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    BLOG(0, "Invalid JSON");
    callback(type::Result::RETRY, "");
    return;
  }

  auto* drain_id = dictionary->FindStringKey("drain_id");
  if (!drain_id) {
    BLOG(0, "Missing drain id");
    callback(type::Result::RETRY, "");
    return;
  }
  callback(result, *drain_id);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
