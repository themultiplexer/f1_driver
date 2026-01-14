#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <rtmidi/RtMidi.h>    // RtMidi library for MIDI handling
#include <vector>             // For std::vector used in RtMidi
#include <iostream>           // For std::cout and std::cerr
#include "input_reader_base.h"  // For matrix button checking functions
#include "input_reader_knob.h"  // For knob input reading
#include "input_reader_fader.h" // For fader input reading
#include "input_reader_wheel.h"
#include "led_controller_base.h"
#include "led_controller_display.h"
#include "startup_sequence.h"


// F1 device identifiers
const unsigned short VENDOR_ID = 0x17cc;
const unsigned short PRODUCT_ID = 0x1120;

// =============================================================================
// STATE TRACKING STRUCTURES
// =============================================================================

struct MatrixButtonState {
    bool current_state[4][4];   // Current state of all matrix buttons
    bool previous_state[4][4];  // Previous state for change detection

    // Constructor to initialize all states to false
    MatrixButtonState() {
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                current_state[row][col] = false;
                previous_state[row][col] = false;
            }
        }
    }
};

struct AnalogControlState {
    int previous_knob_values[4];    // Previous knob values for change detection
    int previous_fader_values[4];   // Previous fader values for change detection

    // Constructor to initialize all values to -1 (invalid)
    AnalogControlState() {
        for (int i = 0; i < 4; i++) {
            previous_knob_values[i] = -1;
            previous_fader_values[i] = -1;
        }
    }
};

class ControllerDelegate {
public:
    virtual void sendButtonPress(int index) = 0;
    virtual void sendButtonRelease(int index) = 0;
    virtual void sendKnobChanged(int index, int value) = 0;
    virtual void sendSliderChanged(int index, int value) = 0;
    virtual void sendMatrixButtonPress(int row, int col) = 0;
    virtual void sendMatrixButtonRelease(int row, int col) = 0;
};


class ControllerHandler {
private:
    RtMidiOut* midi_out;                    // MIDI output port
    RtMidiIn* midi_in;                      // MIDI input port (for future LED control)
    MatrixButtonState button_state;         // Track button states for change detection
    AnalogControlState analog_state;        // Track knob and fader states for change detection
    ControllerDelegate *delegate;

    // Declare current effects page variable
    int current_effect_page ;

    hid_device *device;
    // Declare wheel reader system
    WheelInputReader wheel_input_reader;
    // Declare knob input reader
    KnobInputReader knob_input_reader;
    // Declare fader input reader
    FaderInputReader fader_input_reader;
    // Declare display controller
    DisplayController display_controller;

    
public:
// Constructor and destructor
    ControllerHandler();
    ~ControllerHandler();
    
    // Matrix button MIDI functions
    void updateButtons(const unsigned char* input_buffer);
    void updateMatrixButtonStates(const unsigned char* input_buffer);

    void setLED();
    // Analog control MIDI functions (NEW)
    void mycallback(double deltatime, std::vector<unsigned char> *message);
    void updateKnobStates(const unsigned char* input_buffer);
    void updateFaderStates(const unsigned char* input_buffer);
    void sendKnobChange(int knob_number, int value);
    void sendFaderChange(int fader_number, int value);
    void setDelegate(ControllerDelegate *delegate);
    bool run();
    void close();
    void setStopButton(int index, float brightness);
    void setMatrixButton(int row, int col, LEDColor color, float brightness);
    void setSpecialButton(SpecialLEDButton button, float brightness);
};

#endif // MIDI_HANDLER_H
