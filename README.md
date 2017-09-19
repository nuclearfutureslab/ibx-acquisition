# RedPitaya Data Acquisition

## Introduction

This tool was written as part of the "Information Barrier eXperimental" project ([Repository](https://github.com/nuclearfutureslab/ibx)), [Description](https://nuclearfutures.princeton.edu/projects/ibx/). The [Red Pitaya](https://redpitaya.com/) is a signal processing device with fast capture capabilities. The hardware together with this software is used in the IBX to provide a Gamma Spectrometer.

The software makes use of the FPGA module oscilloscope that is shipped with the RedPitaya OS. It can be used for any triggered data acquisition task using a Red Pitaya. It was greatly inspired by [this project](https://github.com/Grozomah/trigger), thanks to Grozomah for that!

## Installation

### On RedPitaya itself

This should be very straightforward. If you have a C++ compiler (`g++`) and `cmake` installed, it just boils down to:
```
git clone <repoaddress>
mkdir build_acquisition
cd build_acquisition
cmake ../ibx-acquisition/
make
```

After that, you can either run the file from the directory you build in (`./acquisition`), or put it in any folder that is included in your `$PATH` variable. No `make install` rules are included so far.

### Cross Compiling

This is rather complicated, but possible.

## Usage

The following examples assume that the binary is in a directory that is included in your `$PATH` variable. 

All options can be seen using

```
acquisition -h
```

Usually, you might need many options for a run. A good example is
```
acquisition -t 3 -v -150 -o 4 -p 32 -l 384 -r 90 110 30 200 -f best_measurement_ever 60
```

The line above will do the following:
- Trigger for Input on Channel A, negative edge (`-t 3`)
- Trigger when input voltage is recorde in ADC channel below -150 (`-v -150`)
- Use output mode 4 (`-o 4`), output mode 4 is specifically useful for gamma spectroscopy. It integrates the incoming signal, and stores only! the integral in the output file. A simple method is implemented to reject traces that would contain two or more gamma peaks. `-r 90 110 30 200` are the rejection settings. For more details see notes on rejections.
- The output of each event will include 32 datapoints that were recorded before the trigger triggered `-p 32`
- Each event will be output as a set of 384 datapoints `-l 384`
- Output will be written to file `best_measurement_ever.txt`, the suffix is added automatically (`-f best_measurement_ever`)
- Measurement will take about 60 seconds (stops at first event after 60s are over)



For more info refer to the `acquisition -h`.

### Some notes on rejection algorithm

A very simple rejection has been implemented (only for output type 4). For this output, the `-r <min> <max> <s> <e>` option should be specified. For each trace, the code calculates the integral and finds a peak between channel `<s>` and `<e>`.
The code will reject detected traces if either one of the following conditions is true:
- integral < peak * <min>
- integral > peak * <max>
(or accept traces only if the negation of both is true together)

