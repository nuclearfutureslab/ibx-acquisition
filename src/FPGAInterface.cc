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


#include "FPGAInterface.hh"

#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cstddef>
#include <iostream>

FPGAInterface::FPGAInterface() {

  // Oscilloscope module
  oinit = false;
  omem_fd = -1;
  omem = NULL;
  ochA = NULL;
  ochB = NULL;
}

FPGAInterface::~FPGAInterface() {
  stopOscilloscope();
}

int FPGAInterface::initOscilloscope() {
  // Basically this method is taken from the RedPitaya Github repository:

  // Clean up before Opening
  if(stopOscilloscope() < 0) {
    return -1;
  }

  void *fpgaptr;
  long pageaddress;
  long pageoffset;
  long pagesize = sysconf(_SC_PAGESIZE);

  // Open /dev/mem
  omem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if(omem_fd < 0) {
    std::cout << "Error opening /dev/mem" << std::endl;
    return -1;
  }

  pageaddress = OSCBASE & (~(pagesize-1));
  pageoffset = OSCBASE - pageaddress;

  fpgaptr = mmap(NULL, OSCBASESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, omem_fd, pageaddress);

  if(fpgaptr == MAP_FAILED) {
    std::cout << "Error while starting oscilloscope, mmap() error" << std::endl;
    std::cout << strerror(errno);
    stopOscilloscope();
    return -1;
  }

  omem = (oscilloscope_mem*) (fpgaptr + pageoffset);
  ochA = (uint32_t *)omem + (OSCCHAOFFSET / sizeof(uint32_t));
  ochB = (uint32_t *)omem + (OSCCHBOFFSET / sizeof(uint32_t));

  oinit = true;
  return 0;
}

int FPGAInterface::stopOscilloscope() {
  if(omem) {
    if(munmap(omem, OSCBASESIZE) < 0) {
      std::cout << "Error while stopping oscilloscope, munmap() error" << std::endl;
      return -1;
    }
  }
  omem = NULL;
  ochA = NULL;
  ochB = NULL;
  if(omem_fd > 0) {
    close(omem_fd);
    omem_fd = -1;
  }
  return 0;
}

oscilloscope_mem * FPGAInterface::GetOscilloscopeMemory() {
  if(oinit) {
    return omem;
  }
  else {
    return NULL;
  }
}

uint32_t * FPGAInterface::GetOscilloscopeChannelA() {
  if(oinit) {
    return ochA;
  }
  else {
    return NULL;
  }
}

uint32_t * FPGAInterface::GetOscilloscopeChannelB() {
  if(oinit) {
    return ochA;
  }
  else {
    return NULL;
  }
}


