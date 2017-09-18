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

#include <iostream>
#include <string>
#include <cstdlib>

#include "TriggeredAcquisition.hh"

void usage() {
      std::cout << "Usage:" << std::endl;
      std::cout << "acquisition [options] <measurementlength>" << std::endl;
      std::cout << std::endl;
      std::cout << "<measurementlength> is the length of measurement." << std::endl;
      std::cout << "Per default, the length is time measured in seconds, but" << std::endl;
      std::cout << "it can be alternatively interpreted as a number of traces" << std::endl;
      std::cout << "when option '-n' is specified" << std::endl;
      std::cout << std::endl;
      std::cout << "Options:" << std::endl;
      std::cout << "   -n                     take <measurementlength> to be number of measured traces" << std::endl;
      std::cout << "   -f <filename>          measured traces will be stored in <filename>";
      std::cout << std::endl;
      std::cout << "   -d <decimation>        set decimation" << std::endl;
      std::cout << "   -t <triggermethod>     set triggering method, details below" << std::endl;
      std::cout << "   -u <triggervoltage>    set trigger (voltage)" << std::endl;
      std::cout << "   -v <triggervalue>      set trigger (channel, -8192 to 8191)" << std::endl;
      std::cout << "   -p <pretriggerlength>  set length of data recorded pre trigger" << std::endl;
      std::cout << "   -l <tracelength>       set total length of single trace" << std::endl;
      std::cout << "                          (includes <pretriggerlength>)" << std::endl;
      std::cout << "   -o <outputmethod>      set output method, details below" << std::endl;
      std::cout << "   -r <min> <max> <s> <e> Rejection parameters for integration (see below)" << std::endl;
      //std::cout << "   -s <min> <max> <s> <e> <tilt> Rejection parameters for improved rej/integ (see below)" << std::endl;
      std::cout << "   -c                     acquire 100 traces for calibration" << std::endl;
      std::cout << "   -a <offset>            offset (in bins) for channel A" << std::endl;
      std::cout << "   -b <offset>            offset (in bins) for channel B" << std::endl;
      std::cout << "   -i <channel>           0 for channel A, 1 for channel B, 2 for both channels" << std::endl;
      std::cout << "   -g                     Run PMT as counter (no traces are written)" << std::endl;
      std::cout << std::endl;
      std::cout << std::endl;
      std::cout << "Trigerring methods:" << std::endl;
      std::cout << " " << TRIG_IMMEDIATE << "   Immediate" << std::endl;
      std::cout << " " << TRIG_A_POS_EDGE << "   Channel A, positive edge" << std::endl;
      std::cout << " " << TRIG_A_NEG_EDGE << "   Channel A, negative edge" << std::endl;
      std::cout << " " << TRIG_B_POS_EDGE << "   Channel B, positive edge" << std::endl;
      std::cout << " " << TRIG_B_NEG_EDGE << "   Channel B, negative edge" << std::endl;
      //std::cout << " " << TRIG_EXTERNAL_0 << "   External trigger, port 0" << std::endl;
      //std::cout << " " << TRIG_EXTERNAL_1 << "   External trigger, port 1" << std::endl;
      std::cout << std::endl;
      std::cout << "Output methods:" << std::endl;
      std::cout << " " << WRITE_OFF_ASCII_SINGLE << "   Ascii file, write every data point separately" << std::endl;
      std::cout << " " << WRITE_OFF_BINARY_SINGLE << "   Binary file, write every data point separately" << std::endl;
      std::cout << " " << WRITE_OFF_ASCII_INTEGRAL << "   Ascii file, write integral over peak, baseline substracted, simple double rejection" << std::endl;
      std::cout << " " << WRITE_OFF_JUST_CHECK << "   No output, just some information on measured data (recommended use with -n)" << std::endl;
      std::cout << " " << std::endl;
      std::cout << "Rejection Parameters:" << std::endl;
      std::cout << "With the -r <min> <max> <s> <e> option, will reject detected peaks if " << std::endl;
      std::cout << "either one of the following conditions is true: " << std::endl;
      std::cout << "integral < peak * <min>" << std::endl;
      std::cout << "integral > peak * <max>" << std::endl;
      std::cout << "Main peaks are searched for between channel <s> and <e>" << std::endl;
}


int main(int argc, char **argv)
{
  TriggeredAcquisition * ta = new TriggeredAcquisition();
  ta->SetVerboseLevel(1);
  ta->SetTrigger(TRIG_IMMEDIATE);
  ta->SetWriteOff(WRITE_OFF_ASCII_SINGLE);

  MeasurementLengthType mt = LENGTH_IS_TIME;
  float triggervoltage = 0;
  int tracelength = 256;
  int pretriggerlength = 0;
  bool counter = false;
  
  for ( int i=1; i<argc; i=i+1 ) {
    if ( std::string(argv[i]) == "-h" || std::string(argv[i]) == "--help") {
      usage();
      return 0;
    }
    else if ( std::string(argv[i]) == "-c") {
      std::cout << "Average value for Channel A: " << ta->MeasureCalibrationA() << std::endl;
      std::cout << "Average value for Channel B: " << ta->MeasureCalibrationB() << std::endl;
      exit(0);
    }
    else if ( std::string(argv[i]) == "-n" ) {
      mt = LENGTH_IS_TRACENO;
    }
    else if ( std::string(argv[i]) == "-f" ) {
      i++;
      ta->SetFilename(std::string(argv[i]));
    }
    else if ( std::string(argv[i]) == "-d" ) {
      i++;
      ta->SetDecimation(std::atoi(argv[i]));
    }
    else if ( std::string(argv[i]) == "-l" ) {
      i++;
      tracelength = std::atoi(argv[i]);
    }
    else if ( std::string(argv[i]) == "-p") { // pretriggerlength
      i++;
      pretriggerlength = std::atoi(argv[i]);
    }
    else if ( std::string(argv[i]) == "-t" ) {
      i++;
      int tstmp = std::atoi(argv[i]);
      if(tstmp > 0 && tstmp < 8) {
	ta->SetTrigger((TriggerSetting) tstmp);
      }
      else {
	std::cout << "Error: Not a valid triggering method. Run 'acquire -h' to see help." << std::endl;
	exit(-2);
      }
    }
    else if ( std::string(argv[i]) == "-u" ) {
      i++;
      ta->SetTriggervoltage(std::atof(argv[i]));
    }
    else if ( std::string(argv[i]) == "-v" ) {
      i++;
      ta->SetTriggervalue(std::atoi(argv[i]));
    }
    else if ( std::string(argv[i]) == "-o" ) {
      i++;
      int wotmp = std::atoi(argv[i]);
      if(wotmp >= 0 && wotmp < 6) {
	ta->SetWriteOff((WriteOffSetting) wotmp);
      }
      else {
	std::cout << "Error: Not a valid output method. Run 'acquire -h' to see help." << std::endl;
	exit(-2);
      }
    }
    else if (std::string(argv[i]) == "-r") {
      i++;
      float rmin = std::atof(argv[i]);
      i++;
      float rmax = std::atof(argv[i]);
      i++;
      float cstart = std::atof(argv[i]);
      i++;
      float cend = std::atof(argv[i]);
      ta->SetRejectionParameters(rmin, rmax, cstart, cend);
    }
    else if (std::string(argv[i]) == "-s") {
      i++;
      float rmin = std::atof(argv[i]);
      i++;
      float rmax = std::atof(argv[i]);
      i++;
      float cstart = std::atof(argv[i]);
      i++;
      float cend = std::atof(argv[i]);
      i++;
      float tilt = std::atof(argv[i]);
      ta->SetRejectionParameters(rmin, rmax, cstart, cend, tilt);
    }
    else if (std::string(argv[i]) == "-g") {
      counter = true;
    }
  }
  ta->SetTracelength(tracelength);
  ta->SetPretriggerlength(pretriggerlength);

  ta->Init();
    
  float length = atof(argv[argc - 1]);
  if(counter) {
    // Counter Mode
    ta->Geiger(length, mt);
  }
  else {
    // Measurement Mode
    ta->DumpSettings();
    ta->Measure(length, mt);
  }

  // Cleanup
  delete ta;

  // End Program
  return 0;
}
