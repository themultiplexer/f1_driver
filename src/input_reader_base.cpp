#include "input_reader_base.h"       // Include header file

#include <iostream>             // For std::cout and std::cerr
#include <cmath>
#include <iomanip>              // For std::hex (hexadecimal printing)
#include <ostream>
#include <sys/types.h>
// #include <hidapi/hidapi.h>   // included already in header

/*
* Reads an input report from the Traktor Kontrol F1 device
* 
* @param device: Pointer to the opened HID device
* @param buffer: Array to store the 22-byte input report
* @return: true if read was successful, false if there was an error
*/

// Function:
bool readInputReport(hid_device *device, unsigned char *buffer) {

    // Step 1: Check if device is valid
    // Checks if pointers are valid before using them! This prevents crashes.
    if (device == nullptr) {
        std::cerr << "readInputReport Error: Device is null (not connected any longer?)" << std::endl;
        return false;
    }

    // Step 2: Check if buffer is valid
    // Checks if pointers are valid before using them! This prevents crashes.
    if (buffer == nullptr) {
        std::cerr << "readInputReport Error: Buffer is null" << std::endl;
        return false;
    }
    hid_set_nonblocking(device,1);
    // Step 3: Try to read input report from the F1
    // hid_read() returns the number of bytes actually read
    int bytes_read = hid_read(device, buffer, INPUT_REPORT_SIZE);


    if (bytes_read <= 0) {
        return false;
    }
    
    // Step 6: Verify this is the correct type of report
    // The F1 always starts input reports with 0x01
    if (buffer[0] != INPUT_REPORT_ID) {
        std::cerr << "readInputReport Error: Wrong report ID. Expected 0x"
                  << std::hex << (int)INPUT_REPORT_ID
                  << ", got 0x" << std::hex << (int)buffer[0] << std::endl;
        return false;
    }

    // Step 7: Success! There is a valid 22-byte input report
    return true;
}

// =============================================================================
// SPECIAL BUTTON CHECKING FUNCTIONS
// =============================================================================

/*
* Checks if a specific special button is currently pressed
* Special buttons are: SHIFT, REVERSE, TYPE, SIZE, BROWSE, SELECTOR_WHEEL
* 
* @param buffer: The 22-byte input report from readInputReport()
* @param button: Which special button to check (using the enum)
* @return: true if the button is pressed, false if not pressed
*/

bool isSpecialButtonPressed(const unsigned char* buffer, SpecialButton button) {
    // Step 2: Get the special buttons byte (byte 3 in the report)
    unsigned char special_byte = buffer[BUTTON_BYTE_SPECIAL];  // buffer[3]

    // Step 3: Determine which bit mask to use based on the button requested
    unsigned char button_mask;

    switch (button) {
        case SpecialButton::SHIFT:
            button_mask = BIT_MASK_SHIFT;           // 0x80 (bit 7)
            break;
        case SpecialButton::REVERSE:
            button_mask = BIT_MASK_REVERSE;         // 0x40 (bit 6)
            break;
        case SpecialButton::TYPE:
            button_mask = BIT_MASK_TYPE;            // 0x20 (bit 5)
            break;
        case SpecialButton::SIZE:
            button_mask = BIT_MASK_SIZE;            // 0x10 (bit 4)
            break;
        case SpecialButton::BROWSE:
            button_mask = BIT_MASK_BROWSE;          // 0x08 (bit 3)
            break;
        case SpecialButton::SELECTOR_WHEEL:
            button_mask = BIT_MASK_SELECTOR_WHEEL;  // 0x04 (bit 2)
            break;
        default:
            std::cerr << "Error: Unknown special button requested" << std::endl;
            return false;
    }

    // Step 4: Use bitwise AND to check if the specific bit is set
    bool is_pressed = (special_byte & button_mask) != 0;

    // Step 5: Return the result
    return is_pressed;
}

// =============================================================================
// STOP BUTTON CHECKING FUNCTIONS
// =============================================================================

/*
* Checks if a specific stop button is currently pressed
* Special buttons are: STOP1, STOP2, STOP3, STOP4
* 
* @param buffer: The 22-byte input report from readInputReport()
* @param button: Which special button to check (using the enum)
* @return: true if the button is pressed, false if not pressed
*/

bool isStopButtonPressed(const unsigned char* buffer, int button) {
    // Step 2: Get the stop buttons byte (byte 3 in the report)
    unsigned char stop_byte = buffer[BUTTON_BYTE_STOP_AND_CONTROL];  // buffer[4]
    unsigned char button_mask = 0x01 << (7 - button);
    bool is_pressed = (stop_byte & button_mask) != 0;

    return is_pressed;
}

/*
* Checks if a specific control button is currently pressed
* Control buttons are: SYNC, QUANT, CAPTURE
* 
* @param buffer: The 22-byte input report from readInputReport()
* @param button: Which special button to check (using the enum)
* @return: true if the button is pressed, false if not pressed
*/

bool isControlButtonPressed(const unsigned char* buffer, ControlButton button) {

    // Step 1: Safety check - make sure buffer is valid
    if (buffer == nullptr) {
        std::cerr << "Error: Buffer is null in isControlButtonPressed()" << std::endl;
        return false;
    }

    // Step 2: Get the control buttons byte (byte 3 in the report)
    unsigned char control_byte = buffer[BUTTON_BYTE_STOP_AND_CONTROL];  // buffer[4]

    // Step 3: Determine which bit mask to use based on the button requested
    unsigned char button_mask;

    switch (button) {
        case ControlButton::SYNC:
            button_mask = BIT_MASK_SYNC;           // 0x80 (bit 7)
            break;
        case ControlButton::QUANT:
            button_mask = BIT_MASK_QUANT;           // 0x40 (bit 6)
            break;
        case ControlButton::CAPTURE:
            button_mask = BIT_MASK_CAPTURE;       // 0x20 (bit 5)
            break;
        default:
            std::cerr << "Error: Unknown control button requested" << std::endl;
            return false;
    }

    // Step 4: Use bitwise AND to check if the specific bit is set
    bool is_pressed = (control_byte & button_mask) != 0;

    // Step 5: Return the result
    return is_pressed;
}

/*
* Checks if a specific matrix button is currently pressed
 * Matrix is a 4x4 grid: rows 1-4, columns 1-4
 * 
 * @param buffer: The 22-byte input report from readInputReport()
 * @param row: Matrix row (1-4)  
 * @param col: Matrix column (1-4)
 * @return: true if the button is pressed, false if not pressed

Matrix Layout:          Byte Mapping:
(1,1) (2,1) (3,1) (4,1)   Byte 1: bits 7,6,5,4
(1,2) (2,2) (3,2) (4,2)   Byte 1: bits 3,2,1,0  
(1,3) (2,3) (3,3) (4,3)   Byte 2: bits 7,6,5,4
(1,4) (2,4) (3,4) (4,4)   Byte 2: bits 3,2,1,0
*/

bool isMatrixButtonPressed(const unsigned char* buffer, int row, int col) {
    unsigned char bit_mask = (0x01 << (3-col)) << ((1-(row%2)) * 4);
    return (buffer[(row/2)+1] & bit_mask) != 0;
}


/*
* Prints the raw input report in hexadecimal format
* Useful for debugging and understanding what the F1 is sending
* 
* @param buffer: The 22-byte input report to print
*/

void printRawInputReport(const unsigned char* buffer) {
    std::cout << "Raw Input Report (22 bytes): ";
    
    // Loop through all 22 bytes and print each one in hex format
    for (int i = 0; i < INPUT_REPORT_SIZE; i++) {
        // Print each byte as a 2-digit hexadecimal number
        std::cout << std::hex << std::setfill('0') << std::setw(2) 
                  << (int)buffer[i] << " ";
    }
    
    std::cout << std::endl;  // New line at the end
}