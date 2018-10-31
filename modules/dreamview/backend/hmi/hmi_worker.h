/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "boost/thread/locks.hpp"
#include "boost/thread/shared_mutex.hpp"

#include "cyber/cyber.h"
#include "modules/canbus/proto/chassis.pb.h"
#include "modules/common/proto/drive_event.pb.h"
#include "modules/control/proto/pad_msg.pb.h"
#include "modules/dreamview/proto/hmi_config.pb.h"
#include "modules/dreamview/proto/hmi_status.pb.h"
#include "modules/monitor/proto/system_status.pb.h"

/**
 * @namespace apollo::dreamview
 * @brief apollo::dreamview
 */
namespace apollo {
namespace dreamview {

using ChangeModeHandler = std::function<void(const std::string&)>;
using ChangeLaunchHandler = std::function<void(const std::string&)>;
using ChangeMapHandler = std::function<void(const std::string&)>;
using ChangeVehicleHandler = std::function<void(const std::string&)>;

// Singleton worker which does the actual work of HMI actions.
class HMIWorker {
 public:
  HMIWorker() : HMIWorker(cyber::CreateNode("HMI")) {}
  explicit HMIWorker(const std::shared_ptr<apollo::cyber::Node>& node);

  // HMI action trigger.
  bool Trigger(const HMIAction action);
  bool Trigger(const HMIAction action, const std::string& value);

  // Run a command on current system mode.
  void RunModeCommand(const std::string& command_name);
  // Run a command on given module.
  int RunModuleCommand(const std::string& module, const std::string& command);
  // Run a command on given hardware.
  int RunHardwareCommand(const std::string& hardware,
                         const std::string& command);
  // Run a command on given tool.
  int RunToolCommand(const std::string& tool, const std::string& command);

  // Update system status.
  void UpdateSystemStatus(const apollo::monitor::SystemStatus& system_status);

  // Register event handlers.
  void RegisterChangeModeHandler(ChangeModeHandler handler) {
    change_mode_handlers_.emplace_back(handler);
  }
  void RegisterChangeLaunchHandler(ChangeLaunchHandler handler) {
    change_launch_handlers_.emplace_back(handler);
  }
  void RegisterChangeMapHandler(ChangeMapHandler handler) {
    change_map_handlers_.emplace_back(handler);
  }
  void RegisterChangeVehicleHandler(ChangeVehicleHandler handler) {
    change_vehicle_handlers_.emplace_back(handler);
  }
  // Change current mode, launch, map, vehicle and driving mode.
  void ChangeToMode(const std::string& mode_name);
  void ChangeToLaunch(const std::string& launch_name);
  void ChangeToMap(const std::string& map_name);
  void ChangeToVehicle(const std::string& vehicle_name);
  bool ChangeToDrivingMode(const apollo::canbus::Chassis::DrivingMode mode);

  // Submit a DriveEvent.
  void SubmitDriveEvent(const uint64_t event_time_ms,
                        const std::string& event_msg,
                        const std::vector<std::string>& event_types);

  // Get current config and status.
  inline const HMIConfig& GetConfig() const { return config_; }
  HMIStatus GetStatus() const;

  // HMIStatus is updated frequently by multiple threads, including web workers
  // and ROS message callback. Please apply proper read/write lock when
  // accessing it.
  inline boost::shared_mutex& GetStatusMutex() { return status_mutex_; }

  // Load modes configuration from FLAGS_modes_config_path to HMIConfig.modes().
  //
  // E.g. Modes directory:
  // /path/to/modes/
  //     mkz_standard/
  //         close_loop.launch
  //         map_collection.launch
  //
  // In HMIConfig it will be loaded as:
  // modes {
  //   key: "Mkz Standard"
  //   value: {
  //     path: "/path/to/modes/mkz_standard"
  //     launches: {
  //       key: "Close Loop"
  //       value: "/path/to/modes/mkz_standard/close_loop.launch"
  //     }
  //     launches: {
  //       key: "Map Collection"
  //       value: "/path/to/modes/mkz_standard/map_collection.launch"
  //     }
  //   }
  // }
  static bool LoadModesConfig(
      const std::string& modes_config_path, HMIConfig* config);

 private:
  void InitReadersAndWriters(
      const std::shared_ptr<apollo::cyber::Node>& node);

  // Run command: scripts/cyber_launch.sh <command> <current_launch>
  bool CyberLaunch(const std::string& command) const;
  void SetupMode();
  void ResetMode();

  void RecordAudio(const std::string& data);

  HMIConfig config_;
  HMIStatus status_;
  mutable boost::shared_mutex status_mutex_;

  std::vector<ChangeModeHandler> change_mode_handlers_;
  std::vector<ChangeLaunchHandler> change_launch_handlers_;
  std::vector<ChangeMapHandler> change_map_handlers_;
  std::vector<ChangeVehicleHandler> change_vehicle_handlers_;

  // Cyber members.
  std::shared_ptr<cyber::Reader<apollo::canbus::Chassis>> chassis_reader_;
  std::shared_ptr<cyber::Writer<AudioCapture>> audio_capture_writer_;
  std::shared_ptr<cyber::Writer<apollo::control::PadMessage>> pad_writer_;
  std::shared_ptr<cyber::Writer<apollo::common::DriveEvent>>
      drive_event_writer_;
};

}  // namespace dreamview
}  // namespace apollo
