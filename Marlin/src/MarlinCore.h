/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "inc/MarlinConfig.h"

#ifdef DEBUG_GCODE_PARSER
  #include "gcode/parser.h"
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void stop();

// Pass true to keep steppers from timing out
void idle(TERN_(ADVANCED_PAUSE_FEATURE, bool no_stepper_sleep=false));
inline void idle_no_sleep() { idle(TERN_(ADVANCED_PAUSE_FEATURE, true)); }

#if ENABLED(G38_PROBE_TARGET)
  extern uint8_t G38_move;          // Flag to tell the ISR that G38 is in progress, and the type
  extern bool G38_did_trigger;      // Flag from the ISR to indicate the endstop changed
#endif

/**
 * The axis order in all axis related arrays is X, Y, Z, E
 */
void enable_e_steppers();
void enable_all_steppers();
void disable_e_stepper(const uint8_t e);
void disable_e_steppers();
void disable_all_steppers();

void kill(PGM_P const lcd_error=nullptr, PGM_P const lcd_component=nullptr, const bool steppers_off=false);
void minkill(const bool steppers_off=false);

// Global State of the firmware
enum MarlinState : uint8_t {
  MF_INITIALIZING = 0,
  MF_STOPPED,
  MF_KILLED,
  MF_RUNNING,
  MF_SD_COMPLETE,
  MF_PAUSED,
  MF_WAITING,
};

extern MarlinState marlin_state;
inline bool IsRunning() { return marlin_state >= MF_RUNNING; }
inline bool IsStopped() { return marlin_state == MF_STOPPED; }

bool printingIsActive();
bool printJobOngoing();
bool printingIsPaused();
void startOrResumeJob();

extern bool wait_for_heatup;
extern uint8_t language_change_font;
// extern uint8_t wifi_enable_flag;

#if HAS_RESUME_CONTINUE
  extern bool wait_for_user;
  void wait_for_user_response(millis_t ms=0, const bool no_sleep=false);
#endif

#if ENABLED(PSU_CONTROL)
  extern bool powersupply_on;
  #define PSU_PIN_ON()  do{ OUT_WRITE(PS_ON_PIN,  PSU_ACTIVE_STATE); powersupply_on = true;  }while(0)
  #define PSU_PIN_OFF() do{ OUT_WRITE(PS_ON_PIN, !PSU_ACTIVE_STATE); powersupply_on = false; }while(0)
  #if ENABLED(AUTO_POWER_CONTROL)
    #define PSU_ON()       powerManager.power_on()
    #define PSU_OFF()      powerManager.power_off()
    #define PSU_OFF_SOON() powerManager.power_off_soon()
  #else
    #define PSU_ON()     PSU_PIN_ON()
    #define PSU_OFF()    PSU_PIN_OFF()
    #define PSU_OFF_SOON PSU_OFF
  #endif
#endif

bool pin_is_protected(const pin_t pin);

#if HAS_SUICIDE
  inline void suicide() { OUT_WRITE(SUICIDE_PIN, SUICIDE_PIN_INVERTING); }
#endif

#if HAS_KILL
  #ifndef KILL_PIN_STATE
    #define KILL_PIN_STATE LOW
  #endif
  inline bool kill_state() { return READ(KILL_PIN) == KILL_PIN_STATE; }
#endif

extern const char M112_KILL_STR[];
