// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_COMMANDS_H_
#define LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_COMMANDS_H_

#include <weave/commands.h>

#include <string>

#include <gmock/gmock.h>

namespace weave {
namespace test {

class MockCommands : public Commands {
 public:
  ~MockCommands() override = default;

  MOCK_METHOD1(AddOnCommandAddedCallback, void(const OnCommandCallback&));
  MOCK_METHOD1(AddOnCommandRemovedCallback, void(const OnCommandCallback&));
  MOCK_METHOD3(AddCommand,
               bool(const base::DictionaryValue&, std::string*, ErrorPtr*));
  MOCK_METHOD1(FindCommand, Command*(const std::string&));
};

}  // namespace test
}  // namespace weave

#endif  // LIBWEAVE_INCLUDE_WEAVE_TEST_MOCK_COMMANDS_H_
