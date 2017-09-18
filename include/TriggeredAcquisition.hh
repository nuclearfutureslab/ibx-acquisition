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

#ifndef TRIGGEREDACQUISTION
#define TRIGGEREDACQUISTION

#include <iostream>
#include <string>
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <cmath>

#include "FPGAInterface.hh"

/** enum definitions for possible settings */
enum MeasurementLengthType {
  LENGTH_IS_TIME,
  LENGTH_IS_TRACENO
};

enum TriggerSetting {
  TRIG_NO_ACQUISITION = 0,
  TRIG_IMMEDIATE = 1,
  TRIG_A_POS_EDGE = 2,
  TRIG_A_NEG_EDGE = 3,
  TRIG_B_POS_EDGE = 4,
  TRIG_B_NEG_EDGE = 5,
  TRIG_EXTERNAL_0 = 6,
  TRIG_EXTERNAL_1 = 7
};

enum WriteOffSetting {
  WRITE_OFF_ASCII_SINGLE,
  WRITE_OFF_BINARY_SINGLE,
  WRITE_OFF_BINARY_TRACE,
  WRITE_OFF_BINARY_MUL,
  WRITE_OFF_ASCII_INTEGRAL,
  WRITE_OFF_JUST_CHECK
};

const int BUF = 16*1024;
const int MULBUF = 64;

class TriggeredAcquisition
{
public:
  TriggeredAcquisition();
  virtual ~TriggeredAcquisition();
  bool Init();
  void Measure(float length = 10, MeasurementLengthType mlt = LENGTH_IS_TIME);
  void Geiger(float length = 10, MeasurementLengthType mlt = LENGTH_IS_TIME);
  int MeasureCalibrationA();
  int MeasureCalibrationB();

  void SetRejectionParameters(float rmin, float rmax, int cstart, int cend);
  void SetRejectionParameters(float rmin, float rmax, int cstart, int cend, float bend);
  
  void SetDecimation(int dec);
  int GetDecimation() { return decimation; }
  
  void SetTracelength(int n);
  int GetTracelength() { return tracelength; }

  void SetPretriggerlength(int n);
  int GetPretriggerlength() { return pretriggerlength; }

  void SetTriggervalue(int tv);
  float GetTriggervalue() { return triggervalue; }

  void SetTriggervoltage(float vol);
  float GetTriggervoltage() { return triggervoltage; }

  void SetTrigger(TriggerSetting ts);
  TriggerSetting GetTrigger() { return trigger; }
  
  void SetVerboseLevel(int vl);
  int GetVerboseLevel() { return verboseLevel; }

  void SetWriteOff(WriteOffSetting ws);
  WriteOffSetting GetWriteOff() { return writeoff; }

  void SetFilename(std::string filen);
  std::string GetFilename() { return filename; }
  

  inline void WriteOffBinarySingle();
  inline void WriteOffAsciiSingle();
  inline bool WriteOffAsciiIntegral();
  inline void WriteOffJustCheck();
  
  void DumpSettings();
  std::string triggerString(TriggerSetting ts);

private:
  int decimation;
  int tracelength;
  int pretriggerlength;
  float triggervoltage;
  int triggervalue;
  TriggerSetting trigger;
  WriteOffSetting writeoff;

  float ratiomin;
  float ratiomax;
  int channelstart;
  int channelend;
  float curvebend;

  double avgintegpeak;
  int peakpos [BUF];
  
  std::string filename;
  
  bool initialized;
  FPGAInterface * iface;
  
  int verboseLevel;

  // write off variables
  int data [BUF];
  int* datam;
  int* datamb;
  FILE * fh;

  //int * signal_start_ptr;
  uint32_t * signal_start_ptr;
  int trig_ptr;
  int mulcount;
};


#endif /* TRIGGEREDACQUISTION */
