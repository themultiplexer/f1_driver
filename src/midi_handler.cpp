#include "midi_handler.h"
#include "input_reader_base.h"  // For matrix button checking functions
#include "input_reader_knob.h"  // For knob input reading
#include "input_reader_fader.h" // For fader input reading
#include "led_controller_base.h"

MidiHandler::MidiHandler() : midi_out(nullptr), midi_in(nullptr) {
    // Constructor initializes pointers to null and sets initialized to false
}

MidiHandler::~MidiHandler() {
    // Destructor ensures cleanup is called
    cleanup();
}

static void static_callback(double deltatime, std::vector< unsigned char > *message, void *userData) {
    return static_cast<MidiHandler*>(userData)->mycallback(deltatime, message);
}

bool MidiHandler::initializeMIDI() {
    try {
        // Step 1: Create MIDI output port
        std::cout << "- Initializing MIDI output..." << std::endl;
        midi_out = new RtMidiOut();
        
        // Create virtual MIDI output port
        std::cout << "- Created MIDI output port: 0" << std::endl;
        midi_out->openVirtualPort("F1_Controller_Out");

        // Step 2: Create MIDI input port (for future LED control)
        std::cout << "- Initializing MIDI input..." << std::endl;
        midi_in = new RtMidiIn();
        midi_in->openVirtualPort("F1_Controller_In");
        midi_in->setCallback(static_callback, this);
        
        // Step 3: Print MIDI mappings for reference
        std::cout << "- Matrix buttons → MIDI notes 36-51" << std::endl;
        std::cout << "- Knobs 1-4 → MIDI CC 1-4" << std::endl;
        std::cout << "- Faders 1-4 → MIDI CC 5-8" << std::endl;
        std::cout << "- MIDI initialization successful!" << std::endl;
        
        return true;
        
    } catch (RtMidiError &error) {
        std::cerr << "MIDI initialization error: " << error.getMessage() << std::endl;
        cleanup();
        return false;
    }
}

void MidiHandler::cleanup() {
    std::cout << "- Cleaning up MIDI connections..." << std::endl;
    
    // Close and delete MIDI output
    if (midi_out) {
        midi_out->closePort();
        delete midi_out;
        midi_out = nullptr;
    }
    
    // Close and delete MIDI input
    if (midi_in) {
        midi_in->closePort();
        delete midi_in;
        midi_in = nullptr;
    }
    
    std::cout << "- MIDI cleanup complete." << std::endl;
}



void MidiHandler::mycallback(double deltatime, std::vector<unsigned char> *message)
{
    unsigned int nBytes = message->size();

    for (unsigned int i=0; i<nBytes; i++) {
        std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
    }

    if (nBytes > 2) {
        int num = (int)message->at(1);
        std::cout << num << std::endl;
        if (num >= 36 && num < (36 + 16)) {
            num = num - 36;
            if ((int)message->at(0) == 144) {
                // Button pressed
                std::cout << "pressed " << std::endl;
                setMatrixButtonLED(num / 4,num % 4, LEDColor::red, 0.2, false);
            }
            if ((int)message->at(0) == 128) {
                // Button released
                std::cout << "released " << std::endl;
                setMatrixButtonLED(num / 4,num % 4, LEDColor::blue, 0.8, false);
            }
            
        } else {
            if ((int)message->at(0) == 144) {
                setStopButtonLED(num - 16, 1.0);
            }
            if ((int)message->at(0) == 128) {
                setStopButtonLED(num - 16, 0.0);
            }
        }
    }
}

int MidiHandler::matrixPositionToMidiNote(int row, int col) {
    int row_index = row;
    int col_index = col;

    int note = MIDI_NOTE_MATRIX_BASE + (row_index * 4) + col_index;
    return note;
}

void MidiHandler::sendMidiMessage(std::vector<unsigned char>& message) {
    if (!midi_out) {
        std::cerr << "Error: MIDI not initialized, cannot send message" << std::endl;
        return;
    }
    
    try {
        midi_out->sendMessage(&message);
    } catch (RtMidiError &error) {
        std::cerr << "MIDI send error: " << error.getMessage() << std::endl;
    }
}

void MidiHandler::updateButtons(const unsigned char* input_buffer) {
    if (isSpecialButtonPressed(input_buffer, SpecialButton::SHIFT)) {
        std::cerr << "Shift pressed..." << std::endl;
        sendButtonPress(0);
    }

    for (int i = 0; i < 4; i++) {
        if (isStopButtonPressed(input_buffer, i)) {
            sendButtonPress(i);
        }
    }
}

void MidiHandler::updateMatrixButtonStates(const unsigned char* input_buffer) {
    // Update button states and send MIDI for any changes
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int row_index = row;
            int col_index = col;
            
            // Get current button state
            bool current_pressed = isMatrixButtonPressed(input_buffer, row, col);
            
            // Check if state has changed
            if (current_pressed != button_state.previous_state[row_index][col_index]) {
                if (current_pressed) {
                    // Button was just pressed
                    sendMatrixButtonPress(row, col);
                    std::cout << "Matrix button (" << row << "," << col << ") pressed - MIDI note " 
                             << matrixPositionToMidiNote(row, col) << std::endl;
                } else {
                    // Button was just released
                    sendMatrixButtonRelease(row, col);
                    std::cout << "Matrix button (" << row << "," << col << ") released - MIDI note " 
                             << matrixPositionToMidiNote(row, col) << std::endl;
                }
            }
            
            // Update states for next frame
            button_state.previous_state[row_index][col_index] = current_pressed;
            button_state.current_state[row_index][col_index] = current_pressed;
        }
    }
}

void MidiHandler::sendButtonPress(int index) {
    // Create MIDI Note On message
    std::vector<unsigned char> message(3);
    
    message[0] = MIDI_NOTE_ON + MIDI_CHANNEL;           // Note On + channel
    message[1] = 36 + 16 + index;    // Note number
    message[2] = MIDI_VELOCITY_ON;                      // Velocity (127)
    
    sendMidiMessage(message);
}

void MidiHandler::sendMatrixButtonPress(int row, int col) {
    // Create MIDI Note On message
    std::vector<unsigned char> message(3);
    
    message[0] = MIDI_NOTE_ON + MIDI_CHANNEL;           // Note On + channel
    message[1] = matrixPositionToMidiNote(row, col);    // Note number
    message[2] = MIDI_VELOCITY_ON;                      // Velocity (127)
    
    sendMidiMessage(message);
}

void MidiHandler::sendMatrixButtonRelease(int row, int col) {
    // Create MIDI Note Off message
    std::vector<unsigned char> message(3);
    
    message[0] = MIDI_NOTE_OFF + MIDI_CHANNEL;          // Note Off + channel
    message[1] = matrixPositionToMidiNote(row, col);    // Note number  
    message[2] = MIDI_VELOCITY_OFF;                     // Velocity (0)
    
    sendMidiMessage(message);
}

void MidiHandler::updateKnobStates(const unsigned char* input_buffer) {
    // Create knob input reader instance
    KnobInputReader knob_reader;
    
    // Update knob states and send MIDI for any changes
    for (int knob = 0; knob < 4; knob++) {
        // Get current knob value (0-127)
        int current_value = (int)knob_reader.getKnobValue(input_buffer, knob);
        
        // Check if value has changed (allow for some tolerance)
        int previous_value = analog_state.previous_knob_values[knob];
        
        if (current_value != previous_value) {
            // Send MIDI CC message for knob change
            sendKnobChange(knob, current_value);
            
            // Only print if significantly changed (reduce spam)
            std::cout << "Knob " << knob << " changed to " << current_value 
                        << " - MIDI CC " << (MIDI_CC_KNOB_BASE + knob) << std::endl;
        }
        
        // Update state for next frame
        analog_state.previous_knob_values[knob] = current_value;
    }
}

void MidiHandler::updateFaderStates(const unsigned char* input_buffer) {
    // Create fader input reader instance
    FaderInputReader fader_reader;
    
    // Update fader states and send MIDI for any changes
    for (int fader = 0; fader < 4; fader++) {
        // Get current fader value (0-127)
        int current_value = (int)fader_reader.getFaderValue(input_buffer, fader);
        
        // Check if value has changed (allow for some tolerance)
        int previous_value = analog_state.previous_fader_values[fader];
        
        if (current_value != previous_value) {
            // Send MIDI CC message for fader change
            sendFaderChange(fader, current_value);

            std::cout << "Fader " << fader << " changed to " << current_value 
                        << " - MIDI CC " << (MIDI_CC_FADER_BASE + fader) << std::endl;
        }
        
        // Update state for next frame
        analog_state.previous_fader_values[fader] = current_value;
    }
}

void MidiHandler::sendKnobChange(int knob_number, int value) {
    // Create MIDI Control Change message
    std::vector<unsigned char> message(3);
    
    message[0] = MIDI_CC + MIDI_CHANNEL;               // Control Change + channel
    message[1] = MIDI_CC_KNOB_BASE + knob_number;      // CC number (1-4)
    message[2] = std::min(std::max(value, 0), 127); // Clamp value to 0-127
    
    sendMidiMessage(message);
}

void MidiHandler::sendFaderChange(int fader_number, int value) {
    // Create MIDI Control Change message
    std::vector<unsigned char> message(3);
    
    message[0] = MIDI_CC + MIDI_CHANNEL;                    // Control Change + channel
    message[1] = MIDI_CC_FADER_BASE + fader_number;    // CC number (5-8)
    message[2] = std::min(std::max(value, 0), 127);        // Clamp value to 0-127
    
    sendMidiMessage(message);
}