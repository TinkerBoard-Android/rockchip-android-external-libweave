// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/daemon/common/daemon.h"

#include <weave/device.h>

#include <base/bind.h>
#include <base/memory/weak_ptr.h>

#include <bitset>

namespace {
// Supported LED count on this device
const size_t kLedCount = 3;
}  // namespace

// LedFlasherHandler is a complete command handler example that shows
// how to handle commands that modify device state.
class LedFlasherHandler {
 public:
  LedFlasherHandler() {}
  void Register(weave::Device* device) {
    device_ = device;

    device->AddStateDefinitionsFromJson(R"({
      "_ledflasher": {"_leds": {"items": "boolean"}}
    })");

    device->SetStatePropertiesFromJson(R"({
      "_ledflasher":{"_leds": [false, false, false]}
    })",
                                       nullptr);

    device->AddCommandDefinitionsFromJson(R"({
      "_ledflasher": {
         "_set":{
           "parameters": {
             "_led": {"minimum": 1, "maximum": 3},
             "_on": "boolean"
           }
         },
         "_toggle":{
           "parameters": {
             "_led": {"minimum": 1, "maximum": 3}
           }
        }
      }
    })");
    device->AddCommandHandler(
        "_ledflasher._toggle",
        base::Bind(&LedFlasherHandler::OnFlasherToggleCommand,
                   weak_ptr_factory_.GetWeakPtr()));
    device->AddCommandHandler(
        "_ledflasher._set", base::Bind(&LedFlasherHandler::OnFlasherSetCommand,
                                       weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  void OnFlasherSetCommand(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    int32_t led_index = 0;
    bool cmd_value = false;
    if (cmd->GetParameters()->GetInteger("_led", &led_index) &&
        cmd->GetParameters()->GetBoolean("_on", &cmd_value)) {
      // Display this command in terminal
      LOG(INFO) << cmd->GetName() << " _led: " << led_index
                << ", _on: " << (cmd_value ? "true" : "false");

      led_index--;
      int new_state = cmd_value ? 1 : 0;
      int cur_state = led_status_[led_index];
      led_status_[led_index] = new_state;

      if (cmd_value != cur_state) {
        UpdateLedState();
      }
      cmd->Complete({}, nullptr);
      return;
    }
    weave::ErrorPtr error;
    weave::Error::AddTo(&error, FROM_HERE, "example", "invalid_parameter_value",
                        "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  void OnFlasherToggleCommand(const std::weak_ptr<weave::Command>& command) {
    auto cmd = command.lock();
    if (!cmd)
      return;
    LOG(INFO) << "received command: " << cmd->GetName();
    int32_t led_index = 0;
    if (cmd->GetParameters()->GetInteger("_led", &led_index)) {
      LOG(INFO) << cmd->GetName() << " _led: " << led_index;
      led_index--;
      led_status_[led_index] = ~led_status_[led_index];

      UpdateLedState();
      cmd->Complete({}, nullptr);
      return;
    }
    weave::ErrorPtr error;
    weave::Error::AddTo(&error, FROM_HERE, "example", "invalid_parameter_value",
                        "Invalid parameters");
    cmd->Abort(error.get(), nullptr);
  }

  void UpdateLedState() {
    base::ListValue list;
    for (uint32_t i = 0; i < led_status_.size(); i++)
      list.AppendBoolean(led_status_[i] ? true : false);

    device_->SetStateProperty("_ledflasher._leds", list, nullptr);
  }

  weave::Device* device_{nullptr};

  // Simulate LED status on this device so client app could explore
  // Each bit represents one device, indexing from LSB
  std::bitset<kLedCount> led_status_{0};
  base::WeakPtrFactory<LedFlasherHandler> weak_ptr_factory_{this};
};

int main(int argc, char** argv) {
  Daemon::Options opts;
  if (!opts.Parse(argc, argv)) {
    Daemon::Options::ShowUsage(argv[0]);
    return 1;
  }
  Daemon daemon{opts};
  LedFlasherHandler handler;
  handler.Register(daemon.GetDevice());
  daemon.Run();
  return 0;
}