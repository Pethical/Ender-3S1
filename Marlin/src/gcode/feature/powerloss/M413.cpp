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

#include "../../../inc/MarlinConfig.h"

#if ENABLED(POWER_LOSS_RECOVERY)

#include "../../gcode.h"
#include "../../../feature/powerloss.h"
#include "../../../module/motion.h"
#include "../../../lcd/marlinui.h"

/**
 * M413: Enable / Disable power-loss recovery
 *
 * Parameters
 *   S[bool] - Flag to enable / disable.
 *             If omitted, report current state.
 */
void GcodeSuite::M413() {

  if (parser.seen('S'))
    recovery.enable(parser.value_bool());
  else {
    SERIAL_ECHO_START();
    SERIAL_ECHOPGM("Power-loss recovery ");
    serialprintln_onoff(recovery.enabled);
  }

  #if ENABLED(DEBUG_POWER_LOSS_RECOVERY)
    if (parser.seen("RL")) recovery.load();
    if (parser.seen_test('W')) recovery.save(true);
    if (parser.seen_test('P')) recovery.purge();
    if (parser.seen_test('D')) recovery.debug(PSTR("M413"));
    #if PIN_EXISTS(POWER_LOSS)
      if (parser.seen_test('O')) recovery._outage();
    #endif
    if (parser.seen_test('E')) SERIAL_ECHOPGM_P(recovery.exists() ? PSTR("PLR Exists\n") : PSTR("No PLR\n"));
    if (parser.seen_test('V')) SERIAL_ECHOPGM_P(recovery.valid() ? PSTR("Valid\n") : PSTR("Invalid\n"));
  #endif
}

#elif ENABLED(CREALITY_POWER_LOSS)

  #include "../../gcode.h"
  #include "../../../feature/PRE01_Power_loss/PRE01_Power_loss.h"
  #include "../../../module/motion.h"
  #include "../../../lcd/marlinui.h"

/**
 * M413: Enable / Disable power-loss recovery
 *
 * Parameters
 *   S[bool] - Flag to enable / disable.
 *             If omitted, report current state.
 */
void GcodeSuite::M413() {

  if (parser.seen('S'))
    pre01_power_loss.enable(parser.value_bool());
  else {
    SERIAL_ECHO_START();
    SERIAL_ECHOPGM("Power-loss recovery ");
    serialprintln_onoff(pre01_power_loss.enabled);
  }

  #if ENABLED(DEBUG_POWER_LOSS_RECOVERY)
    if (parser.seen("RL")) pre01_power_loss.load();
    if (parser.seen_test('W')) pre01_power_loss.save(true);
    if (parser.seen_test('P')) pre01_power_loss.purge();
    if (parser.seen_test('D')) pre01_power_loss.debug(PSTR("M413"));
    #if PIN_EXISTS(POWER_LOSS)
      if (parser.seen_test('O')) pre01_power_loss._outage();
    #endif
    if (parser.seen_test('E')) SERIAL_ECHOPGM_P(pre01_power_loss.exists() ? PSTR("PLR Exists\n") : PSTR("No PLR\n"));
    if (parser.seen_test('V')) SERIAL_ECHOPGM_P(pre01_power_loss.valid() ? PSTR("Valid\n") : PSTR("Invalid\n"));
  #endif
}

#endif // POWER_LOSS_RECOVERY
