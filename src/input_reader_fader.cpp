#include "include/input_reader_fader.h"

#include <iostream>             // For std::cout and std::cerr
#include <iomanip>              // For std::hex (hexadecimal printing)
#include <cmath>                // For std::abs and std::round
// #include <hidapi/hidapi.h>   // included already in header
// #include <cstdint>          // included already in header

// =============================================================================
// FADER INPUT READER CLASS IMPLEMENTATION
// =============================================================================

/*
* Constructor/Initialization
*
* Initializes the fader input reader and tracking variables.
*/
bool FaderInputReader::initialize() {
    // Initialize all previous values to 0.0
    for (int i = 0; i < FADER_COUNT; i++) {
        previous_values[i] = 0.0f;
    }
    initialized = false;
    return true;
}

// =============================================================================
// MAIN FaderInputReader CLASS FUNCTIONS
// =============================================================================

/*
* Get the normalized fader value (0.000 to 1.000)
* 
* @param buffer: The 22-byte input report from readInputReport()
* @param fader_number: Which fader to read (1-4)
* @return: Normalized float value from 0.000 to 1.000
*/
float FaderInputReader::getFaderValue(const unsigned char* buffer, int fader_number) {
    // Step 3: Extract raw 12-bit value from buffer
    uint16_t raw_value = extractRawFaderValue(buffer, fader_number);

    // Step 4: Convert raw value to normalized float
    float normalized_value = rawToNormalized(raw_value);

    return normalized_value;
}

/*
* Update stored fader states for next frame comparison
* Call this once per frame after reading all fader values you need
* 
* @param buffer: The 22-byte input report from readInputReport()
*/
void FaderInputReader::updateFaderStates(const unsigned char* buffer) {
    for (int fader = 0; fader < FADER_COUNT; fader++) {
        previous_values[fader] = getFaderValue(buffer, fader);
    }

    // Step 3: Mark as initialized
    initialized = true;
}

/*
* Get the raw 12-bit fader value (for debugging or advanced use)
* 
* @param buffer: The 22-byte input report from readInputReport()
* @param fader_number: Which fader to read (1-4)
* @return: Raw 12-bit value (0-4095)
*/
uint16_t FaderInputReader::getRawFaderValue(const unsigned char* buffer, int fader_number) {
    // Step 3: Extract and return raw value
    return extractRawFaderValue(buffer, fader_number);
}

/*
* Print all fader values in a single line (utility for debugging/monitoring)
* 
* @param buffer: The 22-byte input report from readInputReport()
*/
void FaderInputReader::printFaderValues(const unsigned char* buffer) {
    // Step 1: Check if buffer is valid
    if (buffer == nullptr) {
        std::cerr << "FaderInputReader Error: Buffer is null in printFaderValues()" << std::endl;
        return;
    }

    // Step 2: Get all fader values
    float fader1 = getFaderValue(buffer, 0);
    float fader2 = getFaderValue(buffer, 1);
    float fader3 = getFaderValue(buffer, 2);
    float fader4 = getFaderValue(buffer, 3);

    // Step 3: Print values with consistent formatting
    std::cout << "Fader Values: "
              << "F1: " << std::fixed << std::setprecision(3) << fader1 << " | "
              << "F2: " << std::fixed << std::setprecision(3) << fader2 << " | "
              << "F3: " << std::fixed << std::setprecision(3) << fader3 << " | "
              << "F4: " << std::fixed << std::setprecision(3) << fader4
              << "        \r"; // Carriage return to overwrite the line
    std::cout.flush();
}

// =============================================================================
// PRIVATE HELPER FUNCTIONS
// =============================================================================

/*
* Extract raw 12-bit fader value from input buffer
* Handles LSB-first byte ordering and 12-bit masking
* 
* @param buffer: The 22-byte input report
* @param fader_number: Which fader to extract (1-4)
* @return: Raw 12-bit value, clamped to valid range
*/
uint16_t FaderInputReader::extractRawFaderValue(const unsigned char* buffer, int fader_number) const {
    // Step 1: Calculate byte positions for this fader
    // Fader 1: bytes 14-15, Fader 2: bytes 16-17, Fader 3: bytes 18-19, Fader 4: bytes 20-21
    int byte_offset = fader_number * FADER_BYTES_PER_FADER;
    int lsb_position = FADER_BYTE_START + byte_offset;      // LSB position
    int msb_position = FADER_BYTE_START + byte_offset + 1;  // MSB position

    // Step 2: Extract bytes and reconstruct 16-bit value (LSB first)
    uint16_t raw_value = buffer[lsb_position] | (buffer[msb_position] << 8);

    // Step 3: Apply 12-bit mask (upper 4 bits should be zero anyway)
    raw_value &= FADER_12BIT_MASK;

    // Step 4: Clamp to valid range and return
    return clampRawValue(raw_value);
}

/*
* Convert raw 12-bit value to normalized float (0.000 to 1.000)
* 
* @param raw_value: Raw 12-bit fader value (0-4095)
* @return: Normalized float value
*/
float FaderInputReader::rawToNormalized(uint16_t raw_value) const {
    // Simple division to get 0 to 127 range
    int normalized_value = (float)raw_value / (float)FADER_RAW_MAX * 127.0f;
    
    return normalized_value;
}

/*
* Clamp raw value to valid 12-bit range (simple calibration)
* 
* @param raw_value: Raw value to clamp
* @return: Clamped value within valid range
*/
uint16_t FaderInputReader::clampRawValue(uint16_t raw_value) const {
    if (raw_value < FADER_RAW_MIN) {
        return FADER_RAW_MIN;
    }
    if (raw_value > FADER_RAW_MAX) {
        return FADER_RAW_MAX;
    }
    return raw_value;
}
