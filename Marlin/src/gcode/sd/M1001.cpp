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

#include "../../inc/MarlinConfig.h"

#if ENABLED(SDSUPPORT)

#include "../gcode.h"
#include "../../module/planner.h"
#include "../../module/printcounter.h"
#include "../../sd/cardreader.h"

#ifdef SD_FINISHED_RELEASECOMMAND
  #include "../queue.h"
#endif

#if EITHER(LCD_SET_PROGRESS_MANUALLY, SD_REPRINT_LAST_SELECTED_FILE)
  #include "../../lcd/marlinui.h"
#endif

#if ENABLED(POWER_LOSS_RECOVERY)
  #include "../../feature/powerloss.h"
#elif ENABLED(CREALITY_POWER_LOSS)
  #include "../../feature/PRE01_Power_loss/PRE01_Power_loss.h"
#endif

#if HAS_LEDS_OFF_FLAG
  #include "../../MarlinCore.h" // for wait_for_user_response()
  #include "../../feature/leds/printer_event_leds.h"
#endif

#if ENABLED(EXTENSIBLE_UI)
  #include "../../lcd/extui/ui_api.h"
#endif

#if ENABLED(HOST_ACTION_COMMANDS)
  #include "../../feature/host_actions.h"
#endif

#if HAS_CUTTER
  #include "../../feature/spindle_laser.h"
#endif

#if ENABLED(RTS_AVAILABLE)
  #include "../../lcd/dwin/lcd_rts.h"
#endif

#ifndef PE_LEDS_COMPLETED_TIME
  #define PE_LEDS_COMPLETED_TIME (30*60)
#endif

/**
 * M1001: Execute actions for SD print completion
 */
void GcodeSuite::M1001() {
  planner.synchronize();

  // SD Printing is finished when the queue reaches M1001
  card.flag.sdprinting = card.flag.sdprintdone = false;

  // If there's another auto#.g file to run...
  if (TERN(NO_SD_AUTOSTART, false, card.autofile_check())) return;

  // Purge the recovery file...
  TERN_(POWER_LOSS_RECOVERY, recovery.purge());
  TERN_(CREALITY_POWER_LOSS, pre01_power_loss.purge());

  // Report total print time
  const bool long_print = print_job_timer.duration() > 60;
  if (long_print) gcode.process_subcommands_now_P(PSTR("M31"));
  // Stop the print job timer
  gcode.process_subcommands_now_P(PSTR("M77"));

  // Set the progress bar "done" state
  TERN_(LCD_SET_PROGRESS_MANUALLY, ui.set_progress_done());

  // Announce SD file completion
  {
    PORT_REDIRECT(SerialMask::All);
    SERIAL_ECHOLNPGM(STR_FILE_PRINTED);
  }

  // Update the status LED color
  #if HAS_LEDS_OFF_FLAG
    if (long_print) {
      printerEventLEDs.onPrintCompleted();
      TERN_(EXTENSIBLE_UI, ExtUI::onUserConfirmRequired_P(GET_TEXT(MSG_PRINT_DONE)));
      TERN_(HOST_PROMPT_SUPPORT, host_prompt_do(PROMPT_USER_CONTINUE, GET_TEXT(MSG_PRINT_DONE), CONTINUE_STR));
      wait_for_user_response(SEC_TO_MS(TERN(HAS_LCD_MENU, PE_LEDS_COMPLETED_TIME, 30)));
      printerEventLEDs.onResumeAfterWait();
    }
  #endif

  // Inject SD_FINISHED_RELEASECOMMAND, if any
  #ifdef SD_FINISHED_RELEASECOMMAND
    #if HAS_CUTTER
      if(laser_device.is_laser_device())
      {  
      }else
    #endif
    {
      gcode.process_subcommands_now_P(PSTR(SD_FINISHED_RELEASECOMMAND));
    }
  #endif

  TERN_(EXTENSIBLE_UI, ExtUI::onPrintFinished());

  // Re-select the last printed file in the UI
  TERN_(SD_REPRINT_LAST_SELECTED_FILE, ui.reselect_last_file());

  // 激光雕刻结束
  #if HAS_CUTTER
    if(laser_device.is_laser_device()){ 
      rtscheck.RTS_SndData(ExchangePageBase + 60, ExchangepageAddr);
      change_page_font = 60;
    }
  #endif

}

#endif // SDSUPPORT
