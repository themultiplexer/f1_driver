// On Linux: build with control+shift+b (builds with cmake, declared in tasks.json)
// On Mac: build with command+shift+b (builds with cmake, declared in tasks.json)
// run in terminal with ./main


#include <iostream>			// For standard input output operations
#include <stdio.h> 			// stdio.h gives printf() — basic text output (like print() in R).
#include <wchar.h> 			// wchar.h gives wide-char support (wchar_t) used because USB string descriptors are UTF-16/wide.
#include <unistd.h> 		// unistd.h gives access to the POSIX operating system API.
#include <stdint.h> 		// stdint.h gives uint8_t support. uint8_t is an unsigned 8-bit integer type.
#include <hidapi/hidapi.h>  // HIDAPI library for USB HID device access
#include <rtmidi/RtMidi.h>	// RtMidi library for MIDI handling
#include <fcntl.h>          // fcntl.h gives access to the POSIX file control API.
#include <cstring>          // string.h gives access to C-style string functions.
#include <chrono>
#include <thread>
#include <iomanip>
#include <cmath>

#include "include/input_reader_base.h"				// Include input read module
#include "include/input_reader_fader.h"       // Include fader input read module
#include "include/input_reader_knob.h"				// Include knob input read module
#include "include/input_reader_wheel.h"				// Include wheel input read module

#include "include/led_controller_base.h"			// Include LED control module
#include "include/led_controller_display.h"		// Include display control module

#include "include/startup_sequence.h"					// Include startup effects module
#include "include/midi_handler.h"							// Include MIDI handler module



