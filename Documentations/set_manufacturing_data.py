#!/usr/bin/env python3
"""
Set manufacturing data on OSTC4 via Bluetooth serial connection.

This script implements the OSTC4 service mode protocol to set:
- 0x80: Write manufacturing data (52 bytes)
- 0x81: Write secondary serial (12 bytes)  
- 0x82: Set Bluetooth name

The device must be in bootloader mode and connected via Bluetooth.
"""

import serial
import struct
import sys
import time

# Service mode bytes
BYTE_SERVICE_MODE = 0xAA

def send_service_mode_init(ser):
    """Send service mode initialization sequence: 0xAA 0xAB 0xCD 0xEF"""
    init_seq = bytes([0xAA, 0xAB, 0xCD, 0xEF])
    ser.write(init_seq)
    # Expected response: 0x4B 0xAB 0xCD 0xEF 0x4C
    response = ser.read(5)
    if len(response) != 5:
        raise Exception(f"Service mode init failed: got {len(response)} bytes, expected 5")
    if response != bytes([0x4B, 0xAB, 0xCD, 0xEF, 0x4C]):
        raise Exception(f"Service mode init failed: unexpected response {response.hex()}")
    print("Service mode initialized successfully")

def send_command(ser, cmd_byte):
    """Send a command byte and wait for echo acknowledgment"""
    ser.write(bytes([cmd_byte]))
    # Device echoes the command byte back as acknowledgment
    echo = ser.read(1)
    if len(echo) != 1 or echo[0] != cmd_byte:
        raise Exception(f"Command 0x{cmd_byte:02X} not echoed: got {echo.hex() if echo else 'nothing'}")
    print(f"Command 0x{cmd_byte:02X} acknowledged (echo)")

def wait_for_prompt(ser):
    """Wait for the prompt byte (0x4D)"""
    prompt = ser.read(1)
    if len(prompt) != 1 or prompt[0] != 0x4D:
        raise Exception(f"Expected prompt 0x4D, got {prompt.hex() if prompt else 'nothing'}")
    print("Received prompt")

def write_production_data(ser, serial_num, licence, revision, year, month, day, info):
    """
    Write production data (command 0x80).
    
    Parameters:
    - serial_num: 16-bit serial number
    - licence: 8-bit licence
    - revision: 8-bit revision  
    - year: production year (e.g., 18 for 2018)
    - month: production month (1-12)
    - day: production day (1-31)
    - info: production info string (max 44 chars)
    """
    # Build 52-byte buffer
    buffer = bytearray(52)
    
    # Serial (little endian)
    buffer[0] = serial_num & 0xFF
    buffer[1] = (serial_num >> 8) & 0xFF
    
    # Licence
    buffer[2] = licence
    
    # Revision
    buffer[3] = revision
    
    # Production date
    buffer[4] = year
    buffer[5] = month
    buffer[6] = day
    
    # Bluetooth name set flag (will be set to 0xFF by firmware)
    buffer[7] = 0xFF
    
    # Production info (44 bytes, padded with spaces)
    info_bytes = info.encode('ascii')[:44].ljust(44)
    buffer[8:52] = info_bytes
    
    print(f"Writing production data:")
    print(f"  Serial: {serial_num}")
    print(f"  Licence: {licence}")
    print(f"  Revision: {revision}")
    print(f"  Date: {year + 2000}-{month:02d}-{day:02d}")
    print(f"  Info: '{info}'")
    
    # Send command
    send_command(ser, 0x80)
    
    # Send data
    ser.write(buffer)
    
    # Wait for prompt
    wait_for_prompt(ser)
    print("Production data written successfully")

def write_secondary_serial(ser, serial_num, licence, reason, year, month, day, info):
    """
    Write secondary serial data (command 0x81).
    
    Parameters:
    - serial_num: 16-bit secondary serial number  
    - licence: 8-bit licence
    - reason: 8-bit reason code
    - year: year (e.g., 24 for 2024)
    - month: month (1-12)
    - day: day (1-31)
    - info: secondary info string (max 4 chars)
    """
    # Build 12-byte buffer
    buffer = bytearray(12)
    
    # Serial (little endian)
    buffer[0] = serial_num & 0xFF
    buffer[1] = (serial_num >> 8) & 0xFF
    
    # Licence
    buffer[2] = licence
    
    # Reason
    buffer[3] = reason
    
    # Date
    buffer[4] = year
    buffer[5] = month
    buffer[6] = day
    
    # Bluetooth name set flag (will be set to 0xFF by firmware)
    buffer[7] = 0xFF
    
    # Secondary info (4 bytes)
    info_bytes = info.encode('ascii')[:4].ljust(4)
    buffer[8:12] = info_bytes
    
    print(f"Writing secondary serial:")
    print(f"  Serial: {serial_num}")
    print(f"  Licence: {licence}")
    print(f"  Reason: {reason}")
    print(f"  Date: {year + 2000}-{month:02d}-{day:02d}")
    print(f"  Info: '{info}'")
    
    # Send command
    send_command(ser, 0x81)
    
    # Send data
    ser.write(buffer)
    
    # Wait for prompt
    wait_for_prompt(ser)
    print("Secondary serial written successfully")

def set_bluetooth_name(ser):
    """
    Set Bluetooth name (command 0x82).
    This triggers the bootloader to set the BT name based on the serial number.
    """
    print("Setting Bluetooth name...")
    send_command(ser, 0x82)
    # The bootloader will return from communication mode to set the BT name
    print("Bluetooth name set command sent (device will configure BT module)")

def exit_service_mode(ser):
    """Send exit command (0xFF)"""
    ser.write(bytes([0xFF]))
    print("Exited service mode")

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <serial_port> [--secondary]")
        print()
        print("This script sets manufacturing data for OSTC4 serial 428.")
        print("The device must be in bootloader mode and connected via Bluetooth.")
        print()
        print("Options:")
        print("  --secondary   Also set secondary serial to 428")
        sys.exit(1)
    
    port = sys.argv[1]
    set_secondary = '--secondary' in sys.argv
    
    # Manufacturing data for serial 428 (from backup)
    PRIMARY_SERIAL = 428
    PRIMARY_LICENCE = 0xFF
    REVISION = 0x02
    PROD_YEAR = 18  # 2018
    PROD_MONTH = 2
    PROD_DAY = 12
    PROD_INFO = "       OSTC 4 end-2019 hardware            "
    
    # Secondary serial data
    SECONDARY_SERIAL = 428
    SECONDARY_LICENCE = 0xFF
    SECONDARY_REASON = 0x00
    SEC_YEAR = 24  # 2024
    SEC_MONTH = 12
    SEC_DAY = 25
    SEC_INFO = "    "
    
    try:
        print(f"Opening {port}...")
        ser = serial.Serial(port, 115200, timeout=5)
        time.sleep(0.5)  # Give the connection time to stabilize
        
        # Flush any pending data
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Initialize service mode
        send_service_mode_init(ser)
        
        # Write production data
        write_production_data(ser, PRIMARY_SERIAL, PRIMARY_LICENCE, REVISION,
                            PROD_YEAR, PROD_MONTH, PROD_DAY, PROD_INFO)
        
        if set_secondary:
            # Write secondary serial
            write_secondary_serial(ser, SECONDARY_SERIAL, SECONDARY_LICENCE, 
                                 SECONDARY_REASON, SEC_YEAR, SEC_MONTH, SEC_DAY, SEC_INFO)
        
        # Set Bluetooth name based on serial
        set_bluetooth_name(ser)
        
        print("\nAll done! The device will now configure the Bluetooth module.")
        print("Wait for the bootloader to finish and reconnect.")
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
