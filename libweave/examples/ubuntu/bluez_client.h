// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_UBUNTU_BLUEZ_CLIENT_H_
#define LIBWEAVE_EXAMPLES_UBUNTU_BLUEZ_CLIENT_H_

#include <weave/bluetooth.h>

namespace weave {
namespace examples {

// Example of weave::Bluetooth implemented with bluez.
class BluetoothImpl : public Bluetooth {
 public:
  BluetoothImpl();

  ~BluetoothImpl() override;
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_UBUNTU_BLUEZ_CLIENT_H_