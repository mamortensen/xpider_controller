/*
 * Xpider controller software, running on Atmel MEGA328p
 * Copyright (C) 2015-2017  Roboeve, MakerCollider
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "xpider_inside_protocol.h"

// #define TEST_MODE

#ifdef TEST_MODE
#include <Arduino.h>
#endif

XpiderInsideProtocol::XpiderInsideProtocol() {
}

XpiderInsideProtocol::~XpiderInsideProtocol() {
}

void XpiderInsideProtocol::Initialize(SendType send, CallbackListStruct callback_list) {
  Send = send;
  callback_list_ = callback_list;
}

XpiderInsideProtocol::MessageType XpiderInsideProtocol::Decode(const uint8_t *buffer, uint16_t length) {
  MessageType index = static_cast<MessageType>(buffer[0]);
  switch (index) {
    case kMove: {
      if(callback_list_.move != NULL) {
        callback_list_.move(buffer[1]);
      }
      break;
    }
    case kMoveStep: {
      if(callback_list_.step != NULL) {
        callback_list_.step(buffer[1], buffer[2]);
      }
      break;
    }
    case kAutoMove: {
      if(callback_list_.auto_move != NULL) {
        uint8_t rotate_speed = buffer[1];
        float rotate_rad;
        memcpy(&rotate_rad, &buffer[2], 4);
        uint8_t walk_speed = buffer[6];
        uint8_t walk_step = buffer[7];
        callback_list_.auto_move(rotate_speed, rotate_rad, walk_speed, walk_step);
      }
      break;
    }
    case kRotate: {
      if(callback_list_.rotate != NULL) {
        callback_list_.rotate(buffer[1]);
      }
      break;
    }
    case kSetEye: {
      if(callback_list_.set_eye != NULL) {
        callback_list_.set_eye(static_cast<int8_t>(buffer[1]));
      }
      break;
    }
    case kSetFrontLeds: {
      if(callback_list_.set_front_leds != NULL) {
        callback_list_.set_front_leds(buffer+1);
      }
      break;
    }
    case kHeartBeat: {
      HeartBeatStruct heartbeat;
      int16_t temp_voltage, temp_imu;

      memcpy(&heartbeat.step_counter, &buffer[1], 2);
      memcpy(&heartbeat.obstacle_distance, &buffer[3], 2);
      memcpy(&temp_voltage, &buffer[5], 2);
      heartbeat.battery_voltage = static_cast<float>(temp_voltage) / 100.0f;

      for(int i=0; i<3; i++) {
        memcpy(&temp_imu, &buffer[7+i*2], 2);
        heartbeat.yaw_pitch_roll[i] = static_cast<float>(temp_imu) / 100.0f;
      }

      if(callback_list_.heartbeat != NULL) {
        callback_list_.heartbeat(heartbeat);
      }
      break;
    }
    case kGetRegister: {
      callback_list_.get_register(static_cast<RegisterIndex>(buffer[1]));
      break;
    }
    case kRegisterResponse: {
      callback_list_.register_response(static_cast<RegisterIndex>(buffer[1]), &buffer[3], buffer[2]);
      break;
    }
    case kUnknown:
    default: {
    }
  }

  return index;
}

void XpiderInsideProtocol::SetMove(int8_t speed) {
  uint8_t buffer[2];
  buffer[0] = kMove;
  buffer[1] = speed;
  Send(buffer, 2);
}

void XpiderInsideProtocol::SetStep(int8_t step_speed, uint8_t step_count) {
  uint8_t buffer[3];
  buffer[0] = kMoveStep;
  buffer[1] = step_speed;
  buffer[2] = step_count;
  Send(buffer, 3);
}

void XpiderInsideProtocol::SetAutoMove(uint8_t rotate_speed, float rotate_rad, uint8_t walk_speed, int8_t walk_step) {
  uint8_t buffer[8];
  buffer[0] = kAutoMove;
  buffer[1] = rotate_speed;
  memcpy(&buffer[2], &rotate_rad, 4);
  buffer[6] = walk_speed;
  buffer[7] = walk_step;
  Send(buffer, 8);
}

void XpiderInsideProtocol::SetRotate(int8_t speed) {
  uint8_t buffer[2];
  buffer[0] = kRotate;
  buffer[1] = speed;
  Send(buffer, 2);
}

void XpiderInsideProtocol::SetEye(uint8_t angle) {
  uint8_t buffer[2];
  buffer[0] = kSetEye;
  buffer[1] = angle;
  Send(buffer, 2);
}

void XpiderInsideProtocol::SetFrontLeds(const uint8_t leds[6]) {
  uint8_t buffer[7];
  buffer[0] = kSetFrontLeds;
  memcpy(buffer+1, leds, 6);
  Send(buffer, 7);
}

void XpiderInsideProtocol::SetHeartBeat(HeartBeatStruct heartbeat) {
  uint8_t buffer[13];

  buffer[0] = kHeartBeat;
  memcpy(&buffer[1], &heartbeat.step_counter, 2);
  memcpy(&buffer[3], &heartbeat.obstacle_distance, 2);

  int16_t temp_voltage = static_cast<int16_t>(heartbeat.battery_voltage*100.0f);
  memcpy(&buffer[5], &temp_voltage, 2);

  int16_t temp_imu[3];
  for(int i=0; i<3; i++) {
    temp_imu[i] = static_cast<int16_t>(heartbeat.yaw_pitch_roll[i]*100.0f);
  }
  memcpy(&buffer[7], &temp_imu, 6);
  Send(buffer, 13);
}

void XpiderInsideProtocol::GetRegister(RegisterIndex register_index) {
  uint8_t buffer[2];

  buffer[0] = kGetRegister;
  buffer[1] = static_cast<uint8_t>(register_index);

  Send(buffer, 2);
}

void XpiderInsideProtocol::RegisterResponse(RegisterIndex register_index, const uint8_t *value, uint8_t length) {
  uint8_t buffer[3+length];
  
  buffer[0] = kRegisterResponse;
  buffer[1] = static_cast<uint8_t>(register_index);
  buffer[2] = length;
  memcpy(buffer+3, value, length);

  Send(buffer, 3+length);
}