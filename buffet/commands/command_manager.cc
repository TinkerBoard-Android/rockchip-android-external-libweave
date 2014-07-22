// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "buffet/commands/command_manager.h"

#include <base/at_exit.h>
#include <base/bind.h>
#include <base/files/file_enumerator.h>
#include <base/file_util.h>
#include <base/values.h>
#include <base/json/json_reader.h>

#include "buffet/commands/schema_constants.h"
#include "buffet/error_codes.h"

namespace buffet {

CommandManager* CommandManager::instance_ = nullptr;

const CommandDictionary& CommandManager::GetCommandDictionary() const {
  return dictionary_;
}

bool CommandManager::LoadBaseCommands(const base::DictionaryValue& json,
                                      ErrorPtr* error) {
  return base_dictionary_.LoadCommands(json, "", nullptr, error);
}

bool CommandManager::LoadBaseCommands(const base::FilePath& json_file_path,
                                      ErrorPtr* error) {
  std::unique_ptr<const base::DictionaryValue> json = LoadJsonDict(
      json_file_path, error);
  if (!json)
    return false;
  return LoadBaseCommands(*json, error);
}

bool CommandManager::LoadCommands(const base::DictionaryValue& json,
                                  const std::string& category,
                                  ErrorPtr* error) {
  return dictionary_.LoadCommands(json, category, &base_dictionary_, error);
}

bool CommandManager::LoadCommands(const base::FilePath& json_file_path,
                                  ErrorPtr* error) {
  std::unique_ptr<const base::DictionaryValue> json = LoadJsonDict(
      json_file_path, error);
  if (!json)
    return false;
  std::string category = json_file_path.BaseName().RemoveExtension().value();
  return LoadCommands(*json, category, error);
}

CommandManager* CommandManager::GetInstance() {
  CHECK(instance_) << "CommandManager instance not initialized.";
  return instance_;
}

void CommandManager::Startup() {
  CHECK(!instance_) << "CommandManager instance already initialized.";
  LOG(INFO) << "Initializing CommandManager.";
  std::unique_ptr<CommandManager> inst(new CommandManager);

  // Load global standard GCD command dictionary.
  base::FilePath base_command_file("/etc/buffet/gcd.json");
  LOG(INFO) << "Loading standard commands from " << base_command_file.value();
  CHECK(inst->LoadBaseCommands(base_command_file, nullptr))
      << "Failed to load the standard command definitions.";

  // Load static device command definitions.
  base::FilePath device_command_dir("/etc/buffet/commands");
  base::FileEnumerator enumerator(device_command_dir, false,
                                  base::FileEnumerator::FILES,
                                  FILE_PATH_LITERAL("*.json"));
  base::FilePath json_file_path = enumerator.Next();
  while (!json_file_path.empty()) {
    LOG(INFO) << "Loading command schema from " << json_file_path.value();
    CHECK(inst->LoadCommands(json_file_path, nullptr))
        << "Failed to load the command definition file.";
    json_file_path = enumerator.Next();
  }

  // Register a cleanup callback to be executed at shut-down.
  base::AtExitManager::RegisterTask(base::Bind(&CommandManager::Shutdown));
  // Transfer the object instance pointer from the smart pointer to
  // the global pointer.
  instance_ = inst.release();
}

void CommandManager::Shutdown() {
  CHECK(instance_) << "CommandManager instance not initialized.";
  LOG(INFO) << "Shutting down CommandManager.";
  delete instance_;
  instance_ = nullptr;
}

std::unique_ptr<const base::DictionaryValue> CommandManager::LoadJsonDict(
    const base::FilePath& json_file_path, ErrorPtr* error) {
  std::string json_string;
  if (!base::ReadFileToString(json_file_path, &json_string)) {
    Error::AddToPrintf(error, errors::file_system::kDomain,
                       errors::file_system::kFileReadError,
                       "Failed to read file '%s'",
                       json_file_path.value().c_str());
    return std::unique_ptr<const base::DictionaryValue>();
  }
  std::string error_message;
  base::Value* value = base::JSONReader::ReadAndReturnError(
      json_string, base::JSON_PARSE_RFC, nullptr, &error_message);
  if (!value) {
    Error::AddToPrintf(error, errors::json::kDomain, errors::json::kParseError,
                       "Error parsing content of JSON file '%s': %s",
                       json_file_path.value().c_str(), error_message.c_str());
    return std::unique_ptr<const base::DictionaryValue>();
  }
  const base::DictionaryValue* dict_value = nullptr;
  if (!value->GetAsDictionary(&dict_value)) {
    delete value;
    Error::AddToPrintf(error, errors::json::kDomain,
                       errors::json::kObjectExpected,
                       "Content of file '%s' is not a JSON object",
                       json_file_path.value().c_str());
    return std::unique_ptr<const base::DictionaryValue>();
  }
  return std::unique_ptr<const base::DictionaryValue>(dict_value);
}

}  // namespace buffet