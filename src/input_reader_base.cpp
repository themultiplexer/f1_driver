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

bool isSpecialButtonPressed(const unsigned char* buffer, int index) {
    if (index < 6) {
        unsigned char special_byte = buffer[BUTTON_BYTE_SPECIAL];
        unsigned char button_mask = 0x01 << (7-index); // SHIFT,REVERSE,TYPE,SIZE,BROWSE,SELECTOR_WHEEL
        return (special_byte & button_mask) != 0;
    } else {
        unsigned char control_byte = buffer[BUTTON_BYTE_STOP_AND_CONTROL];  //SYNC, QUANT, CAPTURE
        unsigned char button_mask = 0x01 << (3 - (index-6));
        return (control_byte & button_mask) != 0;
    }
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
    unsigned char button_mask = 0x01 << (7 - button); // STOP1, STOP2, STOP3, STOP4
    bool is_pressed = (stop_byte & button_mask) != 0;

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
