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

#include "../gcode.h"
#include "../../module/motion.h"
#include "../../module/stepper.h"

#if ENABLED(I2C_POSITION_ENCODERS)
  #include "../../feature/encoder_i2c.h"
#endif

/**
 * G92: Set the Current Position to the given X Y Z E values.
 *
 * Behind the scenes the G92 command may modify the Current Position
 * or the Position Shift depending on settings and sub-commands.
 *
 * Since E has no Workspace Offset, it is always set directly.
 *
 * Without Workspace Offsets (e.g., with NO_WORKSPACE_OFFSETS):
 *   G92   : Set NATIVE Current Position to the given X Y Z E.
 *
 * Using Workspace Offsets (default Marlin behavior):
 *   G92   : Modify Workspace Offsets so the reported position shows the given X Y Z E.
 *   G92.1 : Zero XYZ Workspace Offsets (so the reported position = the native position).
 *
 * With POWER_LOSS_RECOVERY:
 *   G92.9 : Set NATIVE Current Position to the given X Y Z E.
 */
void GcodeSuite::G92() {

  bool sync_E = false, sync_XYZE = false;

  #if USE_GCODE_SUBCODES
    const uint8_t subcode_G92 = parser.subcode;
  #else
    constexpr uint8_t subcode_G92 = 0;
  #endif

  switch (subcode_G92) {
    default: return;                                                  // Ignore unknown G92.x

    #if ENABLED(CNC_COORDINATE_SYSTEMS) && !IS_SCARA
      case 1:                                                         // G92.1 - Zero the Workspace Offset
        LOOP_LINEAR_AXES(i) if (position_shift[i]) {
          position_shift[i] = 0;
          update_workspace_offset((AxisEnum)i);
        }
        break;
    #endif

    #if ENABLED(POWER_LOSS_RECOVERY)
      case 9:                                                         // G92.9 - Set Current Position directly (like Marlin 1.0)
        LOOP_LOGICAL_AXES(i) {
          if (parser.seenval(axis_codes[i])) {
            if (i == E_AXIS) sync_E = true; else sync_XYZE = true;
            current_position[i] = parser.value_axis_units((AxisEnum)i);
          }
        }
        break;
    #elif ENABLED(CREALITY_POWER_LOSS)
      case 9:                                                         // G92.9 - Set Current Position directly (like Marlin 1.0)
        LOOP_LOGICAL_AXES(i) {
          if (parser.seenval(axis_codes[i])) {
            if (i == E_AXIS) sync_E = true; else sync_XYZE = true;
            current_position[i] = parser.value_axis_units((AxisEnum)i);
          }
        }
        break;
    #endif

    case 0:
      LOOP_LOGICAL_AXES(i) {
        if (parser.seenval(axis_codes[i])) {
          const float l = parser.value_axis_units((AxisEnum)i),       // Given axis coordinate value, converted to millimeters
                      v = i == E_AXIS ? l : LOGICAL_TO_NATIVE(l, i),  // Axis position in NATIVE space (applying the existing offset)
                      d = v - current_position[i];                    // How much is the current axis position altered by?
          if (!NEAR_ZERO(d)) {
            #if HAS_POSITION_SHIFT && !IS_SCARA                       // When using workspaces...
              if (i == E_AXIS) {
                sync_E = true;
                current_position.e = v;                               // ...E is still set directly
              }
              else {
                position_shift[i] += d;                               // ...but other axes offset the workspace.
                update_workspace_offset((AxisEnum)i);
              }
            #else                                                     // Without workspaces...
              if (i == E_AXIS) sync_E = true; else sync_XYZE = true;
              current_position[i] = v;                                // ...set Current Position directly (like Marlin 1.0)
            #endif
          }
        }
      }
      break;
  }

  #if ENABLED(CNC_COORDINATE_SYSTEMS)
    // Apply Workspace Offset to the active coordinate system
    if (WITHIN(active_coordinate_system, 0, MAX_COORDINATE_SYSTEMS - 1))
      coordinate_system[active_coordinate_system] = position_shift;
  #endif

  if   (sync_XYZE) sync_plan_position();
  else if (sync_E) sync_plan_position_e();

  IF_DISABLED(DIRECT_STEPPING, report_current_position());
}
