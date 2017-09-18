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


#include "TriggeredAcquisition.hh"


TriggeredAcquisition::TriggeredAcquisition() {
  decimation = 1;
  tracelength = 32;
  pretriggerlength = 0;
  
  trigger = TRIG_A_POS_EDGE;
  triggervoltage = 1.0;

  writeoff = WRITE_OFF_ASCII_SINGLE;
  
  initialized = false;
  filename = "output";

  avgintegpeak = 0;
  for(int i = 0; i < BUF; i++) {
    peakpos[i] = 0;
  }

  ratiomin = 0;
  ratiomax = 1e6;
  channelstart = 0;
  channelend = 8192;
  curvebend = 0;

  datam = (int*) malloc(BUF * sizeof(int));
  datamb = (int*) malloc(MULBUF * BUF * sizeof(int));

  verboseLevel = 0;

  iface = new FPGAInterface();
}

TriggeredAcquisition::~TriggeredAcquisition() {
  if(initialized) {
    iface->stopOscilloscope();
  }
  free(datam);

  delete iface;
}

bool TriggeredAcquisition::Init() {
  if(verboseLevel > 0) {
    std::cout << "Start OSC FPGA initialization" << std::endl;
  }

  int start = iface->initOscilloscope();
  
  if(start) {
    std::cout << "Error: OSC FPGA initialization didn't work, retval = " << start << std::endl;
    return false;
  }
  else {
    if(verboseLevel > 0) {
      std::cout << "Initialization successful." << std::endl;
    }
  }

  initialized = true;
  return true;
}

void TriggeredAcquisition::Measure(float length, MeasurementLengthType mlt) {
  int traces = (int) length;
  bool runcondition = true;
  int runcount = 0;
  int discarded = 0;

  int trig_test;
  int * cha_signal;
  int * chb_signal;
  int ptr;
  int i;

  if(writeoff == WRITE_OFF_ASCII_SINGLE) {
    std::cout << "Measure, store in ascii file, write every data point separately" << std::endl;
  }
  else if(writeoff == WRITE_OFF_BINARY_SINGLE) {
    std::cout << "Measure, store in binary file, write every data point separately" << std::endl;
  }
  else if(writeoff == WRITE_OFF_ASCII_INTEGRAL) {
    std::cout << "Measure, store in ascii file, write integral over peak, baseline substracted, simple double peak rejection" << std::endl;
  }
  else if(writeoff == WRITE_OFF_JUST_CHECK) {
    std::cout << "Measure, no storage, just calculation of values useful for adjusting settings." << std::endl;
  }

  // Set 'Trigger delay', number of data points to be acquired after trigger
  iface->GetOscilloscopeMemory()->posttriggertracelength = tracelength;
  if(verboseLevel > 0) {
    std::cout << "Set tracelength for FPGA module" << std::endl;
  }

  // Set Decimation to FPGA module
  iface->GetOscilloscopeMemory()->decimation = decimation;
  if(verboseLevel > 0) {
    std::cout << "Set decimation for FPGA module" << std::endl;
  }

  // Reset Oscilloscope?
  iface->GetOscilloscopeMemory()->configuration |= OSCRESETBIT;

  // Set Trigger Value (check for channel A / B)
  if(trigger == 2 || trigger == 3) { // Channel A
    iface->GetOscilloscopeMemory()->threshold_A = triggervalue;
  }
  else if(trigger == 4 || trigger == 5) { // Channel B
    iface->GetOscilloscopeMemory()->threshold_B = triggervalue;
  }
  if(verboseLevel > 0) {
    std::cout << "Set trigger value for FPGA module" << std::endl;
  }

  std::chrono::high_resolution_clock::time_point starttime;
  typedef std::chrono::duration<double, std::milli> millisec_t;
  millisec_t clkDuration;
  starttime = std::chrono::high_resolution_clock::now();

  //  std::ofstream intfile("data.newbin", std::ios::out | std::ios::binary);
  if (writeoff == WRITE_OFF_BINARY_SINGLE) {
    std::string fullfile = filename + ".bin";
    fh = fopen(fullfile.c_str(), "wb");
    fwrite(&decimation, sizeof(int), 1, fh);
    fwrite(&tracelength, sizeof(int), 1, fh);
    fwrite(&pretriggerlength, sizeof(int), 1, fh);
    fwrite(&triggervoltage, sizeof(float), 1, fh);
    fwrite(&trigger, sizeof(int), 1, fh);
  }
  else if (writeoff == WRITE_OFF_ASCII_SINGLE){
    std::string fullfile = filename + ".txt";
    fh = fopen(fullfile.c_str(), "w");
    if(verboseLevel > 0) {
      std::cout << "Opened output ascii file" << std::endl;
    }

    std::string triggers = triggerString(trigger);
    fprintf(fh, "Decimation:           %d\n", decimation);
    fprintf(fh, "Trace length:         %d\n", tracelength);
    fprintf(fh, "Pretrigger length:    %d\n", pretriggerlength);
    fprintf(fh, "Trigger Value:        %f\n", triggervalue);
    fprintf(fh, "Triggering on:        %s\n", triggers.c_str());
  }
  else if (writeoff == WRITE_OFF_ASCII_INTEGRAL){
    std::string fullfile = filename + ".txt";
    fh = fopen(fullfile.c_str(), "w");
    if(verboseLevel > 0) {
      std::cout << "Opened output ascii file" << std::endl;
    }

    std::string triggers = triggerString(trigger);
    fprintf(fh, "Decimation:           %d\n", decimation);
    fprintf(fh, "Trace length:         %d\n", tracelength);
    fprintf(fh, "Pretrigger length:    %d\n", pretriggerlength);
    fprintf(fh, "Trigger Value:        %f\n", triggervalue);
    fprintf(fh, "Triggering on:        %s\n", triggers.c_str());
    fprintf(fh, "Rej. Param. <min>     %f\n", ratiomin);
    fprintf(fh, "Rej. Param. <max>     %f\n", ratiomax);
    fprintf(fh, "Rej. Param. <s>       %d\n", channelstart);
    fprintf(fh, "Rej. Param. <e>       %d\n", channelend);
  }

  if(verboseLevel > 0) {
    std::cout << "Start main loop" << std::endl;
  }

  mulcount = 0;
  while(runcondition) {
    // Arm Trigger and set to Trigger method
    iface->GetOscilloscopeMemory()->configuration |= TRIGGERARMBIT;
    iface->GetOscilloscopeMemory()->trigger = trigger;

    // Test if triggered, with protection of 10s if no trigger happens
    std::chrono::high_resolution_clock::time_point triggerstarttime;
    millisec_t triggerduration;
    triggerstarttime = std::chrono::high_resolution_clock::now();
    trig_test = iface->GetOscilloscopeMemory()->trigger;
    while (trig_test!=0) {
      triggerduration = std::chrono::duration_cast<millisec_t>(std::chrono::high_resolution_clock::now() - triggerstarttime);
      if(triggerduration.count() / 1000 > 10) {
	std::cout << "Did not trigger for more than 10 s - will stop now!" << std::endl;
	std::cout << "This could be due to wrong trigger settings." << std::endl;
	runcondition = false;
	break;
      }
      trig_test = iface->GetOscilloscopeMemory()->trigger;
    }
    if(verboseLevel > 1) {
      std::cout << "Event triggered" << std::endl;
    }

    // Data is only written if runcondition still valid
    if(runcondition) {
      
      // Get Memory pointers
      trig_ptr = iface->GetOscilloscopeMemory()->triggerpointer;
      signal_start_ptr = iface->GetOscilloscopeChannelA(); // FIX depending on measure channel

      // Write Data depending on method
      if(writeoff == WRITE_OFF_BINARY_SINGLE) {
	WriteOffBinarySingle();
      }
      else if(writeoff == WRITE_OFF_ASCII_SINGLE) {
	WriteOffAsciiSingle();
      }
      else if(writeoff == WRITE_OFF_ASCII_INTEGRAL) {
	if(!WriteOffAsciiIntegral()) {
	  discarded++;
	}
      }
      else if(writeoff == WRITE_OFF_JUST_CHECK) {
	WriteOffJustCheck();
      }

      runcount++;
      if(mlt == LENGTH_IS_TIME) {
	clkDuration = std::chrono::duration_cast<millisec_t>(std::chrono::high_resolution_clock::now() - starttime);
	if(verboseLevel > 1) {
	  std::cout << clkDuration.count()  << "ms" << std::endl;
	}
	if(clkDuration.count() / 1000 > length) {
	  runcondition = false;
	}
      }
      else {
	if(runcount >= traces) {
	  runcondition = false;
	}
      }
    }

  }
  clkDuration = std::chrono::duration_cast<millisec_t>(std::chrono::high_resolution_clock::now() - starttime);
  std::cout << "Sampled " << runcount << " traces in " << clkDuration.count()  << "ms (" << runcount / clkDuration.count() * 1000 << " traces/s)."<< std::endl;
  if (writeoff == WRITE_OFF_ASCII_INTEGRAL) { 
    std::cout << "Discarded " << discarded << " traces because of rejection conditions" << std::endl;
  }
  //    intfile.close();
  if(writeoff == WRITE_OFF_JUST_CHECK) {
    int peak1 = 0;
    int peak2 = 0;
    int peak3 = 0;
    int max1 = 0;
    int max2 = 0;
    int max3 = 0;
    for(int i = 0; i < BUF; i++) {
      if(peakpos[i] > max1) {
	max3 = max2;
	peak3 = peak2;
	max2 = max1;
	peak2 = peak1;
	max1 = peakpos[i];
	peak1 = i;
      }
      else if (peakpos[i] > max2) {
	max3 = max2;
	peak3 = peak2;
	max2 = peakpos[i];
	peak2 = i;
      }
      else if (peakpos[i] > max3) {
	max3 = peakpos[i];
	peak3 = i;
      }
    }

    std::cout << "Results: " << std::endl;
    std::cout << "Average area/peak: " << 1.0 * avgintegpeak / runcount << std::endl;
    std::cout << "Most frequent peak position: " << peak1 << " (" << max1 << " times)"<< std::endl;
    std::cout << "Second most frequent peak position: " << peak2 << " (" << max2 << " times)"<< std::endl;
    std::cout << "Third most frequent peak position: " << peak3 << " (" << max3 << " times)"<< std::endl;
  }
  else {
    fclose(fh);
  }
}

void TriggeredAcquisition::Geiger(float length, MeasurementLengthType mlt) {
  int traces = (int) length;
  bool runcondition = true;
  int runcount = 0;

  int trig_test;
  int * cha_signal;
  int * chb_signal;
  int ptr;
  int i;

  // Set 'Trigger delay', number of data points to be acquired after trigger
  // to 0
  iface->GetOscilloscopeMemory()->posttriggertracelength = 0;

  // Reset Oscilloscope?
  iface->GetOscilloscopeMemory()->configuration |= OSCRESETBIT;

  // Set Trigger Value (check for channel A / B)
  if(trigger == 2 || trigger == 3) { // Channel A
    iface->GetOscilloscopeMemory()->threshold_A = triggervalue;
  }
  else if(trigger == 4 || trigger == 5) { // Channel B
    iface->GetOscilloscopeMemory()->threshold_B = triggervalue;
  }
  if(verboseLevel > 0) {
    std::cout << "Set trigger value for FPGA module" << std::endl;
  }

  std::chrono::high_resolution_clock::time_point starttime;
  typedef std::chrono::duration<double, std::milli> millisec_t;
  millisec_t clkDuration;
  starttime = std::chrono::high_resolution_clock::now();

  //  std::ofstream intfile("data.newbin", std::ios::out | std::ios::binary);

  if(verboseLevel > 0) {
    std::cout << "Start main loop" << std::endl;
  }

  mulcount = 0;
  while(runcondition) {
    // Arm Trigger and set to Trigger method
    iface->GetOscilloscopeMemory()->configuration |= TRIGGERARMBIT;
    iface->GetOscilloscopeMemory()->trigger = trigger;

    trig_test = iface->GetOscilloscopeMemory()->trigger;
    while (trig_test!=0) {
      trig_test = iface->GetOscilloscopeMemory()->trigger;
    }
    runcount++;
    if(mlt == LENGTH_IS_TIME) {
      clkDuration = std::chrono::duration_cast<millisec_t>(std::chrono::high_resolution_clock::now() - starttime);
      if(clkDuration.count() / 1000 > length) {
	runcondition = false;
      }
    }
    else {
      if(runcount >= traces) {
	runcondition = false;
      }
    }
  }
  clkDuration = std::chrono::duration_cast<millisec_t>(std::chrono::high_resolution_clock::now() - starttime);
  std::cout << "Got  " << runcount << " counts in " << clkDuration.count()  << "ms (" << runcount / clkDuration.count() * 1000 << " counts/s)."<< std::endl;
  //    intfile.close();

  fh = fopen("count.txt", "w");
  fprintf(fh, "%d", runcount);
  fclose(fh);
}

void TriggeredAcquisition::SetRejectionParameters(float rmin, float rmax, int cstart, int cend) {
  ratiomin = rmin;
  ratiomax = rmax;
  channelstart = cstart;
  channelend = cend;
  curvebend = 0;
  std::cout << "Reject if integral < peak * " << ratiomin << " or integral > peak * " << ratiomax << std::endl;
  std::cout << "Search for peak between datapoint " << channelstart << " and " << channelend <<std::endl;
}

void TriggeredAcquisition::SetRejectionParameters(float rmin, float rmax, int cstart, int cend, float bend) {
  ratiomin = rmin;
  ratiomax = rmax;
  channelstart = cstart;
  channelend = cend;
  curvebend = bend;
  std::cout << "Reject if integral < peak * " << ratiomin << " or integral > peak * " << ratiomax << std::endl;
  std::cout << "When peak < " << curvebend << ", will only use integ >= peak * " << ratiomax << std::endl;
  std::cout << "Search for peak between datapoint " << channelstart << " and " << channelend <<std::endl;
}

int TriggeredAcquisition::MeasureCalibrationA() {
  int trig_test;
  int * cha_signal;
  int * chb_signal;
  iface->GetOscilloscopeMemory()->posttriggertracelength = 16383;

  double total = 0;
  for(int runs = 0; runs < 100; runs++) {
    int totalrun = 0;
    iface->GetOscilloscopeMemory()->configuration |= TRIGGERARMBIT;
    iface->GetOscilloscopeMemory()->trigger = 1; // Immediate Trigger for Calibration
    trig_test = iface->GetOscilloscopeMemory()->trigger;
    while (trig_test!=0) {
      trig_test = iface->GetOscilloscopeMemory()->trigger;
    }
    trig_ptr = iface->GetOscilloscopeMemory()->triggerpointer;
    signal_start_ptr = iface->GetOscilloscopeChannelA(); // FIX depending on measure 
    for (int i=0; i < 16383; i++) {
      totalrun += signal_start_ptr[(trig_ptr+i)%BUF];
    }
    total += 1.0 * totalrun / 16383;
  }
  total = total / 100;
  // if(total >= 8192) {
  //   total -= 16384;
  // }
  return (int) total;
}

int TriggeredAcquisition::MeasureCalibrationB() {
  int trig_test;
  int * cha_signal;
  int * chb_signal;
  
  iface->GetOscilloscopeMemory()->posttriggertracelength = 16383;

  double total = 0;
  for(int runs = 0; runs < 100; runs++) {
    int totalrun = 0;
    iface->GetOscilloscopeMemory()->configuration |= TRIGGERARMBIT;
    iface->GetOscilloscopeMemory()->trigger = 1; // Immediate Trigger for Calibration
    trig_test = iface->GetOscilloscopeMemory()->trigger;
    while (trig_test!=0) {
      trig_test = iface->GetOscilloscopeMemory()->trigger;
    }
    trig_ptr = iface->GetOscilloscopeMemory()->triggerpointer;
    signal_start_ptr = iface->GetOscilloscopeChannelB(); // FIX depending on measure 
    for (int i=0; i < 16383; i++) {
      totalrun += signal_start_ptr[(trig_ptr+i)%BUF];
    }
    total += 1.0 * totalrun / 16383;
  }
  total = total / 100;
  // if(total >= 8192) {
  //   total -= 16384;
  // }
  return (int) total;
}

inline void TriggeredAcquisition::WriteOffBinarySingle() {
  int tracestart = trig_ptr - pretriggerlength;

  if(tracestart < 0) {
    tracestart += BUF;
  }

  for (int i=0; i < tracelength; i++) {
    datam[i] = signal_start_ptr[(tracestart+i)%BUF];
  }
  fwrite(datam, sizeof(int), tracelength, fh);
}


inline void TriggeredAcquisition::WriteOffAsciiSingle() {
  int tracestart = trig_ptr - pretriggerlength;
  
  if(tracestart < 0) {
    tracestart += BUF;
  }
  for (int i=0; i < tracelength; i++) {
    fprintf(fh, "%d ", signal_start_ptr[(tracestart+i)%BUF]);
  }
  fprintf(fh, "\n");
}

inline bool TriggeredAcquisition::WriteOffAsciiIntegral() {
  int tracestart = trig_ptr - pretriggerlength;
  
  if(tracestart < 0) {
    tracestart += BUF;
  }
  double baseline = 0;
  double total = 0;
  // Set Start / End for Peak test, based on default values or settings
  int startp = pretriggerlength;
  int endp = tracelength;
  if(channelstart != -1) {
    startp = channelstart;
    endp = channelend;
  }
  int peak = 0;
  for (int i=0; i < tracelength; i++) {
    int signal = 0;
    if(signal_start_ptr[(tracestart+i)%BUF] >= 8192) {
      signal = signal_start_ptr[(tracestart+i)%BUF] - 16384;
    }
    else {
      signal = signal_start_ptr[(tracestart+i)%BUF];
    }
    if(i < 25) {
      baseline += signal;
    }
    if(i >= startp and i <= endp and abs(signal) > peak) {
	peak = abs(signal);
    }
    total += signal;
  }
  total -= tracelength * baseline / 25.0;
  peak -= abs(baseline / 25.0);
  if(verboseLevel > 1) {
    std::cout << "Total" << total << " Peak:" << peak <<" base: " << baseline << std::endl;
  }
  if(abs(total) >= peak * ratiomin and abs(total) <= peak * ratiomax) {
    fprintf(fh, "%f\n", total);
    return true;
  }
  else if(peak <= curvebend and abs(total) <= peak * ratiomax) {
    fprintf(fh, "%f\n", total);
    return true;
  }
  return false;
}

inline void TriggeredAcquisition::WriteOffJustCheck() {
  int tracestart = trig_ptr - pretriggerlength;
  
  if(tracestart < 0) {
    tracestart += BUF;
  }
  double baseline = 0;
  double total = 0;
  // Set Start / End for Peak test, based on default values or settings
  int startp = pretriggerlength;
  int endp = tracelength;
  int peak = 0;
  int peakposition = 0;
  for (int i=0; i < tracelength; i++) {
    int signal = 0;
    if(signal_start_ptr[(tracestart+i)%BUF] >= 8192) {
      signal = signal_start_ptr[(tracestart+i)%BUF] - 16384;
    }
    else {
      signal = signal_start_ptr[(tracestart+i)%BUF];
    }
    if(i < 25) {
      baseline += signal;
    }
    if(abs(signal) > peak) {
	peak = abs(signal);
	peakposition = i;
    }
    total += signal;
  }
  total -= tracelength * baseline / 25.0;
  peak -= abs(baseline / 25.0);
  avgintegpeak += 1.0 * total / peak;
  peakpos[peakposition] += 1;
}

void TriggeredAcquisition::SetDecimation(int dec) {
  if(dec != 1 && dec != 8 && dec != 64 && dec != 1024 && dec != 8192 && dec != 65536) {
    std::cout << "Error: Trying to set wrong decimation value. Possible values are 1, 8, 64, 1024, 8192, 65536." << std::endl;
    exit(-2);
  }
  else {
    decimation = dec;
  }
}

void TriggeredAcquisition::SetTracelength(int n) {
  if(n > 0 && n <= 16383) {
    tracelength = n;
  }
  else {
    std::cout << "Error: Trying to set wrong trace length. Traces must be at least 1 value long, and have a maximal length of 16383." << std::endl;
    exit(-2);
  }
}

void TriggeredAcquisition::SetPretriggerlength(int p) {
  if(p >= 0 && p <= tracelength) {
    pretriggerlength = p;
  }
  else {
    std::cout << "Error: Trying to set wrong pretrigger length. Pretriggerlength must be at least 0 values long, and must be shorter then tracelength." << std::endl;
    exit(-2);
  }
}

void TriggeredAcquisition::SetTriggervalue(int tv)  {
  // Correct signed int to int
  if(tv < 0 && tv >= -8192) {
    triggervalue = tv + 16384;
  }
  else if (tv >= 0 && tv <= 8191){
    triggervalue = tv;
  }
  else {
    std::cout << tv << " is not allowed as trigger value (-8192 to 8191)" << std::endl;
  }
}

void TriggeredAcquisition::SetTriggervoltage(float vol) {
  if(trigger == TRIG_A_POS_EDGE
     || trigger == TRIG_A_NEG_EDGE
     || trigger == TRIG_B_POS_EDGE
     || trigger == TRIG_B_NEG_EDGE) {
    if(vol > -5.0 && vol < 5.0) {
      triggervoltage = vol;
    }
    else {
    std::cout << "Error: Trying to set wrong trigger voltage. Trigger voltage should be between -5.0 V and 5.0 V." << std::endl;
    }
    int tv = int(round(8192 * triggervoltage / 14.0));
    std::cout << "Voltage has been converted to triggervalue: " << tv << std::endl;
    SetTriggervalue(tv);
  }
  else {
    std::cout << "Warning: Trying to set trigger voltage, although triggering method is set to " << triggerString(trigger) << ", which does not care about voltage." << std::endl;
  }
}

void TriggeredAcquisition::SetTrigger(TriggerSetting ts) {
  trigger = ts;
}

void TriggeredAcquisition::SetVerboseLevel(int vl) {
  if(vl >= 0 && vl < 10) {
    verboseLevel = vl;
  }
}

void TriggeredAcquisition::SetWriteOff(WriteOffSetting ws) {
  writeoff = ws;
}

void TriggeredAcquisition::SetFilename(std::string filen) {
  filename = filen;
}


void TriggeredAcquisition::DumpSettings() {
  std::cout << std::endl;
  std::cout << "*** Sampling Settings" << std::endl;
  std::cout << "Decimation:               " << decimation << std::endl;
  std::cout << "Trace length:             " << tracelength << std::endl;
  std::cout << "Pretrigger length:        " << pretriggerlength << std::endl;
  //std::cout << "Trigger Voltage:      " << triggervoltage << " Volt" << std::endl;
  std::cout << "Trigger Value:            " << triggervalue << std::endl; 
  std::cout << "Triggering on:            " << triggerString(trigger) << std::endl;
  if (writeoff == WRITE_OFF_ASCII_INTEGRAL) { 
    std::cout << "Rejection Parameter <min> " << ratiomin << std::endl;
    std::cout << "Rejection Parameter <max> " << ratiomax << std::endl;
    std::cout << "Rejection Parameter <s>   " << channelstart << std::endl;
    std::cout << "Rejection Parameter <e>   " << channelend << std::endl;
  }
  std::cout << std::endl;
}

std::string TriggeredAcquisition::triggerString(TriggerSetting ts) {
  switch(ts){
  case TRIG_NO_ACQUISITION: return "No Acquisition";
  case TRIG_IMMEDIATE: return "Immediate";
  case TRIG_A_POS_EDGE: return "Channel A, positive edge";
  case TRIG_A_NEG_EDGE: return "Channel A, negative edge";
  case TRIG_B_POS_EDGE: return "Channel B, positive edge";
  case TRIG_B_NEG_EDGE: return "Channel B, negative edge";
  case TRIG_EXTERNAL_0: return "External trigger, port 0";
  case TRIG_EXTERNAL_1: return "External trigger, port 1";
  }
}
