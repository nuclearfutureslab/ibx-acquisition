/*
 * acquisition - RedPitaya Data Acquisition
 *
 *
 * Copyright (C) 2016, 2017 Moritz Kütt, Malte Göttsche, Alexander Glaser
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: moritz@nuclearfreesoftware.org
 */

#ifndef FPGAINTERFACE_H
#define FPGAINTERFACE_H

#include <stdint.h>

#define OSCBASE         0x40100000
#define OSCBASESIZE     0x30000
#define OSCCHAOFFSET    0x10000
#define OSCCHBOFFSET    0x20000

#define ADCBITS 14

#define TRIGGERARMBIT   1
#define OSCRESETBIT     2

struct housekeeping_mem {
  uint32_t designid;
  uint32_t dna1;
  uint32_t dna2;
  uint32_t digitalloop;
  uint32_t connector_direction_P;
  uint32_t connector_direction_N;
  uint32_t connector_output_P;
  uint32_t connector_output_N;
  uint32_t connector_input_P;
  uint32_t connector_input_N;
  uint32_t ledcontrol;
};

struct oscilloscope_mem {
  uint32_t configuration;
  uint32_t trigger;
  uint32_t threshold_A;
  uint32_t threshold_B;
  uint32_t posttriggertracelength;
  uint32_t decimation;
  uint32_t writepointer;
  uint32_t triggerpointer;
  uint32_t hysteresis_A;
  uint32_t hysteresis_B;
  uint32_t decimationaverage;
  uint32_t pretriggercounter;
  uint32_t filter_aa_A;
  uint32_t filter_bb_A;
  uint32_t filter_kk_A;
  uint32_t filter_pp_A;
  uint32_t filter_aa_B;
  uint32_t filter_bb_B;
  uint32_t filter_kk_B;
  uint32_t filter_pp_B;
};

class FPGAInterface
{
public:
  FPGAInterface();
  virtual ~FPGAInterface();

  int initHousekeeping();
  int stopHousekeeping();
  housekeeping_mem * GetHousekeepingMemory() {return hmem; };
  
  int initOscilloscope();
  int stopOscilloscope();
  oscilloscope_mem * GetOscilloscopeMemory();
  uint32_t * GetOscilloscopeChannelA();
  uint32_t * GetOscilloscopeChannelB();

private:
  bool hinit;
  housekeeping_mem * hmem;
  bool oinit;
  oscilloscope_mem * omem;
  int omem_fd;
  uint32_t *ochA;
  uint32_t *ochB;
  
};


#endif /* FPGAINTERFACE_H */
