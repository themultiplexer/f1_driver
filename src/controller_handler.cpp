#include "controller_handler.h"

ControllerHandler::ControllerHandler() : current_effect_page(1), display_controller(), wheel_input_reader() {
    // Constructor initializes pointers to null and sets initialized to false
    // =============================================================================
    // START UP SEQUENCE
    // =============================================================================

    // create start up message
    std::cout << "" << std::endl;
    std::cout << "=== Starting Visual Sync Kontrol F1 ===" << std::endl;
    std::cout << "" << std::endl;

    // Initialize HIDAPI, get return code, send error message if initialization fails
    int res = hid_init();
    if (res != 0) {
        std::cout << "- Failed to initialize HIDAPI" << std::endl;
        return;
    } else {
        std::cout << "- HID_API initialized successfully!" << std::endl;
    }


    // Open the device using the VendorID, ProductID, and optionally the Serial number.
    // If the device is opened successfully, the pointer will not be null.
    device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);
    if (device) {
        std::cout << "- Opening Traktor Kontrol F1..." << std::endl;

        // Initialize the LED controller
        initializeLEDController(device);

        // Run startup sequence
        startupSequence(device);

        // Initialize wheel input reader and set first page
        wheel_input_reader.initialize();

        // Set first effects page on display
        // Turn on left dot to indicate page is loaded
        display_controller.setDisplayNumber(current_effect_page);
        display_controller.setDisplayDot(1, true);

        // Send success message
        std::cout << "" << std::endl;
        std::cout << "- Traktor Kontrol F1 opened successfully!" << std::endl;
    } else {
        // Send error message and close program
        std::cout << "- Unable to open device..." << std::endl;
        std::cout << "Shutting down..." << std::endl;
        hid_exit();
        return;
    }

    // Send startup message how to terminate while(true) loop
    std::cout << "" << std::endl;
    std::cout << "+++ Press Ctrl+C to exit. +++" << std::endl;
    std::cout << "" << std::endl;

}

ControllerHandler::~ControllerHandler() {
    // Destructor ensures cleanup is called
}

void ControllerHandler::setDelegate(ControllerDelegate *delegate){
    this->delegate = delegate;
}

void ControllerHandler::close() {
    // Close the device
    hid_close(device);

    // Finalize the hidapi library
    hid_exit();
}

bool ControllerHandler::run() {
            // =======================================
        // Read input report
        // =======================================
        unsigned char input_report_buffer[INPUT_REPORT_SIZE];
        if (!readInputReport(device, input_report_buffer)) {
            return false;
        }

        // =======================================
        // MIDI: Process matrix button changes
        // =======================================
        updateButtons(input_report_buffer);
        updateMatrixButtonStates(input_report_buffer);
        updateKnobStates(input_report_buffer);
        updateFaderStates(input_report_buffer);

        // =======================================
        // Read and update Selector Wheel rotation
        // =======================================
        WheelDirection selector_wheel_direction = wheel_input_reader.checkWheelRotation(input_report_buffer);

        if (selector_wheel_direction == WheelDirection::CLOCKWISE) {
            current_effect_page = std::min(current_effect_page + 1, 99);
            delegate->onWheelChanged(current_effect_page);
        } else if (selector_wheel_direction == WheelDirection::COUNTER_CLOCKWISE) {
            current_effect_page = std::max(current_effect_page - 1, 1);
            delegate->onWheelChanged(current_effect_page);
        }


        return true;
}

void ControllerHandler::setStopButton(int index, float brightness) {
    setStopButtonLED(index, brightness);
}

void ControllerHandler::setMatrixButton(int row, int col, BRGColor color, float brightness) {
    setMatrixButtonLED(row, col, color, brightness, false);
}

void ControllerHandler::setMatrixButton(int row, int col, LEDColor color, float brightness) {
    setMatrixButtonLED(row, col, color, brightness, false);
}

void ControllerHandler::setPage(int page) {
    current_effect_page = page;
    display_controller.setDisplayDot(1, false);
    display_controller.setDisplayNumber(current_effect_page);
}

void ControllerHandler::setButton(LEDButton button, float brightness) {
    setButtonLED(button, brightness);
}

bool specialPressed[9] = {false};

void ControllerHandler::updateButtons(const unsigned char* input_buffer) {

    for (int i = 0; i < 9; i++) {
        if (isSpecialButtonPressed(input_buffer, i)) {
            if (!specialPressed[i]) {
                std::cerr << "Special pressed..." << std::endl;
                delegate->onButtonPress(4 + i);
                specialPressed[i] = true;
            }
        } else if (specialPressed[i]) {
            delegate->onButtonRelease(4 + i);
            specialPressed[i] = false;
        }
    }

    for (int i = 0; i < 4; i++) {
        if (isStopButtonPressed(input_buffer, i)) {
            delegate->onButtonPress(i);
        }
    }
}

void ControllerHandler::updateMatrixButtonStates(const unsigned char* input_buffer) {
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
                    delegate->onMatrixButtonPress(row, col);
                } else {
                    // Button was just released
                    delegate->onMatrixButtonRelease(row, col);
                }
            }
            
            // Update states for next frame
            button_state.previous_state[row_index][col_index] = current_pressed;
            button_state.current_state[row_index][col_index] = current_pressed;
        }
    }
}

void ControllerHandler::updateKnobStates(const unsigned char* input_buffer) {
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
            delegate->onKnobChanged(knob, current_value * 2);
        }
        analog_state.previous_knob_values[knob] = current_value;
    }
}

void ControllerHandler::updateFaderStates(const unsigned char* input_buffer) {
    // Create fader input reader instance
    FaderInputReader fader_reader;
    
    // Update fader states and send MIDI for any changes
    for (int fader = 0; fader < 4; fader++) {
        // Get current fader value (0-127)
        int current_value = (int)fader_reader.getFaderValue(input_buffer, fader);
        
        // Check if value has changed (allow for some tolerance)
        int previous_value = analog_state.previous_fader_values[fader];
        auto now = std::chrono::system_clock::now();
        if (current_value != previous_value) {
            if (!analog_state.is_fader_value_dirty[fader]) {
                analog_state.last_slider_change[fader] = now;
                analog_state.is_fader_value_dirty[fader] = true;
            }
        }

        if (analog_state.is_fader_value_dirty[fader] && std::chrono::duration_cast<std::chrono::milliseconds>(now - analog_state.last_slider_change[fader]).count() > 50) {
            delegate->onSliderChanged(fader, current_value * 2);
            analog_state.is_fader_value_dirty[fader] = false;
            analog_state.last_slider_change[fader] = now;
        }

        analog_state.previous_fader_values[fader] = current_value;
    }
}
