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

#include "../../sd/cardreader.h"
#include "../../gcode/gcode.h"
#include "../../module/temperature.h"

#include "../../inc/MarlinConfig.h"


#if ENABLED(CREALITY_POWER_LOSS)

#if ENABLED(GCODE_REPEAT_MARKERS)
  #include "../../feature/repeat.h"
#endif

#if ENABLED(MIXING_EXTRUDER)
  #include "../../feature/mixing.h"
#endif

#if !defined(POWER_LOSS_STATE) && PIN_EXISTS(POWER_LOSS)
  #define POWER_LOSS_STATE HIGH
#endif

#ifndef POWER_LOSS_ZRAISE
  #define POWER_LOSS_ZRAISE 2
#endif

typedef temp_info_t Power_ADC_Info_t;

typedef struct {
  uint8_t valid_head;

  // Machine state
  xyze_pos_t current_position;
  uint16_t feedrate;

  float zraise;

  // Repeat information
  #if ENABLED(GCODE_REPEAT_MARKERS)
    Repeat stored_repeat;
  #endif

  #if ENABLED(HAS_HOME_OFFSET)
    xyz_pos_t home_offset;
  #endif
  #if ENABLED(HAS_POSITION_SHIFT)
    xyz_pos_t position_shift;
  #endif
  #if ENABLED(HAS_MULTI_EXTRUDER)
    uint8_t active_extruder;
  #endif

  #if DISABLED(NO_VOLUMETRICS)
    float filament_size[EXTRUDERS];
  #endif

  #if ENABLED(HAS_HOTEND)
    celsius_t target_temperature[HOTENDS];
  #endif
  #if ENABLED(HAS_HEATED_BED)
    celsius_t target_temperature_bed;
  #endif
  #if ENABLED(HAS_FAN)
    uint8_t fan_speed[FAN_COUNT];
  #endif

  #if ENABLED(HAS_LEVELING)
    float fade;
  #endif

  #if ENABLED(FWRETRACT)
    float retract[EXTRUDERS], retract_hop;
  #endif

  // Mixing extruder and gradient
  #if ENABLED(MIXING_EXTRUDER)
    //uint_fast8_t selected_vtool;
    //mixer_comp_t color[NR_MIXING_VIRTUAL_TOOLS][MIXING_STEPPERS];
    #if ENABLED(GRADIENT_MIX)
      gradient_t gradient;
    #endif
  #endif

  #if ENABLED(RTS_AVAILABLE)
  //recovery flag 
   uint8_t recovery_flag;
  #endif

  // SD Filename and position
  char sd_filename[MAXPATHNAMELENGTH];
  volatile uint32_t sdpos;

  // Job elapsed time
  millis_t print_job_elapsed;

  // Relative axis modes
  uint8_t axis_relative;

  // Misc. Marlin flags
  struct {
    bool raised:1;                // Raised before saved
    bool dryrun:1;                // M111 S8
    bool allow_cold_extrusion:1;  // M302 P1
    #if ENABLED(HAS_LEVELING)
      bool leveling:1;            // M420 S
    #endif
    #if DISABLED(NO_VOLUMETRICS)
      bool volumetric_enabled:1;  // M200 S D
    #endif
  } flag;

  uint8_t valid_foot;

  bool valid() { return valid_head && valid_head == valid_foot; }

} creality_job_recovery_info_t;



class PRE01PowerLoss
{
private:

public:

    static Power_ADC_Info_t POWER_ADC_VALUE;
    /**************************************
     * ADC初始化相关的配置
     * 
     * 
     * 
    */
    static void ADC_GPIO_init(){HAL_ANALOG_SELECT(POWER_DETECTION_PIN);};  //设置GPIO为输入模式，用于ADC采样

    static void init();

    static void check_current_voltage();


    /**********************************************************
     * 断电续打恢复相关的操作
     * 
     * 
     * 
    */
    static const char filename[5];  //当前打印的文件名

    static SdFile file;  //文件句柄
    static creality_job_recovery_info_t info;  //断电续打需要保存的数据

    static uint8_t queue_index_r;     //!< Queue index of the active command
    static uint32_t cmd_sdpos,        //!< SD position of the next command
                    sdpos[BUFSIZE];   //!< SD positions of queued commands

    #if ENABLED(DWIN_CREALITY_LCD)
      static bool dwin_flag;
    #endif


    static void data_init(); //清除断电续打数据
    static void prepare();
    static inline void setup() {
      #if PIN_EXISTS(POWER_LOSS)
        #if ENABLED(POWER_LOSS_PULLUP)
          SET_INPUT_PULLUP(POWER_LOSS_PIN);
        #elif ENABLED(POWER_LOSS_PULLDOWN)
          SET_INPUT_PULLDOWN(POWER_LOSS_PIN);
        #else
          SET_INPUT(POWER_LOSS_PIN);
        #endif
      #endif
    }

    // Track each command's file offsets
    static inline uint32_t command_sdpos() { return sdpos[queue_index_r]; }
    static inline void commit_sdpos(const uint8_t index_w) { sdpos[index_w] = cmd_sdpos; }

    static bool enabled;
    static void enable(const bool onoff);
    static void changed();

    // static inline bool exists() { return card.jobRecoverFileExists(); }
    // static inline void open(const bool read) { card.openJobRecoveryFile(read); }
    // static inline void close() { file.close(); }

    static void check();
    static void resume();
    static void purge();

    static inline void cancel() { purge(); IF_DISABLED(NO_SD_AUTOSTART, card.autofile_begin()); }

    static void load();
    static void save(const bool force=ENABLED(SAVE_EACH_CMD_MODE), const float zraise=POWER_LOSS_ZRAISE, const bool raised=false);

    #if PIN_EXISTS(POWER_LOSS)
      static inline void outage() {
        static constexpr uint8_t OUTAGE_THRESHOLD = 3;
        static uint8_t outage_counter = 0;
        if (enabled && READ(POWER_LOSS_PIN) == POWER_LOSS_STATE) {
          outage_counter++;
          if (outage_counter >= OUTAGE_THRESHOLD) _outage();
        }
        else
          outage_counter = 0;
      }
    #endif

    // The referenced file exists
    static inline bool interrupted_file_exists() { return card.fileExists(info.sd_filename); }

    static inline bool valid() { return info.valid() && interrupted_file_exists(); }

    #if ENABLED(DEBUG_POWER_LOSS_RECOVERY)
      static void debug(PGM_P const prefix);
    #else
      static inline void debug(PGM_P const) {}
    #endif

  private:
    static void write();

    #if ENABLED(BACKUP_POWER_SUPPLY)
      static void retract_and_lift(const_float_t zraise);
    #endif

    #if PIN_EXISTS(POWER_LOSS)
      friend class GcodeSuite;
      static void _outage();
    #endif

};


extern PRE01PowerLoss pre01_power_loss;

#endif


