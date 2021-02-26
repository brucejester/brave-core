/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/time/time.h"
#include "brave/components/tor/tor_file_watcher.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tor {

class TorFileWatcherTest : public testing::Test {
 public:
  TorFileWatcherTest() {}

  void SetUp() override {
    testing::Test::SetUp();
    ASSERT_TRUE(base::PathService::Get(base::DIR_SOURCE_ROOT, &test_data_dir_));
    test_data_dir_ = test_data_dir_.Append(FILE_PATH_LITERAL("brave"))
                         .Append(FILE_PATH_LITERAL("test"))
                         .Append(FILE_PATH_LITERAL("data"));
  }

  base::FilePath test_data_dir() {
    return test_data_dir_.AppendASCII("tor").AppendASCII("tor_control");
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  base::FilePath test_data_dir_;
};

TEST_F(TorFileWatcherTest, EatControlCookie) {
  std::vector<uint8_t> cookie;
  base::Time time;

  std::unique_ptr<TorFileWatcher> tor_file_watcher =
      std::make_unique<TorFileWatcher>(
          test_data_dir().AppendASCII("not_valid"));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlCookie(cookie, time));
  EXPECT_EQ(cookie.size(), 0u);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

  // control_auth_cookie is a folder
  tor_file_watcher.reset(new TorFileWatcher(test_data_dir()));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlCookie(cookie, time));
  EXPECT_EQ(cookie.size(), 0u);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

  tor_file_watcher.reset(
      new TorFileWatcher(test_data_dir().AppendASCII("empty_auth_cookies")));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlCookie(cookie, time));
  EXPECT_EQ(cookie.size(), 0u);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

  tor_file_watcher.reset(
      new TorFileWatcher(test_data_dir().AppendASCII("auth_cookies_too_long")));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlCookie(cookie, time));
  EXPECT_EQ(cookie.size(), 0u);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

  constexpr unsigned char expected_auth_cookie[] = {
      0x6c, 0x6e, 0x9d, 0x24, 0x78, 0xe6, 0x6d, 0x69, 0xd3, 0x2d, 0xc9,
      0x90, 0x9a, 0x3c, 0x39, 0x54, 0x2b, 0x37, 0xff, 0x30, 0xda, 0x5a,
      0x90, 0x94, 0x44, 0xa4, 0x3d, 0x30, 0xd5, 0xa9, 0x19, 0xef};
  unsigned int expected_auth_cookie_len = 32;

  tor_file_watcher.reset(
      new TorFileWatcher(test_data_dir().AppendASCII("normal_auth_cookies")));
  tor_file_watcher->polling_ = true;
  EXPECT_TRUE(tor_file_watcher->EatControlCookie(cookie, time));
  EXPECT_EQ(std::memcmp(cookie.data(), expected_auth_cookie,
                        expected_auth_cookie_len),
            0);
  EXPECT_EQ(cookie.size(), expected_auth_cookie_len);
  EXPECT_NE(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();
}

TEST_F(TorFileWatcherTest, EatControlPort) {
  int port = -1;
  base::Time time;

  std::unique_ptr<TorFileWatcher> tor_file_watcher =
      std::make_unique<TorFileWatcher>(
          test_data_dir().AppendASCII("not_valid"));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

  // controlport is a folder
  tor_file_watcher.reset(new TorFileWatcher(test_data_dir()));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

  tor_file_watcher.reset(
      new TorFileWatcher(test_data_dir().AppendASCII("empty_controlport")));
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

#if defined(OS_WIN)
  tor_file_watcher.reset(new TorFileWatcher(
      test_data_dir().AppendASCII("invalid_controlport_win")));
#else
  tor_file_watcher.reset(
      new TorFileWatcher(test_data_dir().AppendASCII("invalid_controlport")));
#endif
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

#if defined(OS_WIN)
  tor_file_watcher.reset(new TorFileWatcher(
      test_data_dir().AppendASCII("valid_controlport_not_localhost_win")));
#else
  tor_file_watcher.reset(new TorFileWatcher(
      test_data_dir().AppendASCII("valid_controlport_not_localhost")));
#endif
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

#if defined(OS_WIN)
  tor_file_watcher.reset(new TorFileWatcher(
      test_data_dir().AppendASCII("controlport_too_long_win")));
#else
  tor_file_watcher.reset(
      new TorFileWatcher(test_data_dir().AppendASCII("controlport_too_long")));
#endif
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, -1);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

#if defined(OS_WIN)
  tor_file_watcher.reset(new TorFileWatcher(
      test_data_dir().AppendASCII("controlport_overflow_win")));
#else
  tor_file_watcher.reset(
      new TorFileWatcher(test_data_dir().AppendASCII("controlport_overflow")));
#endif
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, 65536);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

  port = -1;
#if defined(OS_WIN)
  tor_file_watcher.reset(new TorFileWatcher(
      test_data_dir().AppendASCII("invalid_control_port_end_win")));
#else
  tor_file_watcher.reset(new TorFileWatcher(
      test_data_dir().AppendASCII("invalid_control_port_end")));
#endif
  tor_file_watcher->polling_ = true;
  EXPECT_FALSE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, 0);
  EXPECT_EQ(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();

  port = -1;
  time = base::Time();

#if defined(OS_WIN)
  tor_file_watcher.reset(new TorFileWatcher(
      test_data_dir().AppendASCII("normal_controlport_win")));
#else
  tor_file_watcher.reset(
      new TorFileWatcher(test_data_dir().AppendASCII("normal_controlport")));
#endif
  tor_file_watcher->polling_ = true;
  EXPECT_TRUE(tor_file_watcher->EatControlPort(port, time));
  EXPECT_EQ(port, 5566);
  EXPECT_NE(time.ToJsTime(), 0u);
  std::move(*tor_file_watcher.release()).DeleteSoon();
}

}  // namespace tor