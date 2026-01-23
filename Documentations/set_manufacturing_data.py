#!/usr/bin/env python3
"""
Set manufacturing data on OSTC4 via Bluetooth serial connection.

This script implements the OSTC4 service mode protocol to set:
- 0x80: Write manufacturing data (52 bytes)
- 0x81: Write secondary serial (12 bytes)  
- 0x82: Set Bluetooth name

The device must be in bootloader mode and connected via Bluetooth.
"""

import argparse
import serial
import struct
import sys
import time
from datetime import datetime

# Service mode bytes
BYTE_SERVICE_MODE = 0xAA

# Validation constants
MIN_SERIAL = 1
MAX_SERIAL = 65535
MIN_LICENCE = 0
MAX_LICENCE = 255
MIN_REVISION = 0
MAX_REVISION = 255
MIN_REASON = 0
MAX_REASON = 255
MIN_YEAR = 0
MAX_YEAR = 99
MIN_MONTH = 1
MAX_MONTH = 12
MIN_DAY = 1
MAX_DAY = 31
MAX_PROD_INFO_LEN = 44
MAX_SEC_INFO_LEN = 4


class ValidationError(Exception):
    """Raised when input validation fails"""
    pass


def validate_serial(value, name="Serial"):
    """Validate serial number is in valid range"""
    if not isinstance(value, int) or value < MIN_SERIAL or value > MAX_SERIAL:
        raise ValidationError(f"{name} must be an integer between {MIN_SERIAL} and {MAX_SERIAL}, got {value}")
    return value


def validate_byte(value, min_val, max_val, name):
    """Validate a byte value is in valid range"""
    if not isinstance(value, int) or value < min_val or value > max_val:
        raise ValidationError(f"{name} must be an integer between {min_val} and {max_val}, got {value}")
    return value


def validate_date(year, month, day):
    """Validate date components"""
    validate_byte(year, MIN_YEAR, MAX_YEAR, "Year")
    validate_byte(month, MIN_MONTH, MAX_MONTH, "Month")
    validate_byte(day, MIN_DAY, MAX_DAY, "Day")
    
    # Check if day is valid for the month
    full_year = 2000 + year
    try:
        datetime(full_year, month, day)
    except ValueError as e:
        raise ValidationError(f"Invalid date: {full_year}-{month:02d}-{day:02d}: {e}")
    
    return year, month, day


def validate_info_string(value, max_len, name):
    """Validate info string is ASCII and within length limit"""
    if not isinstance(value, str):
        raise ValidationError(f"{name} must be a string")
    try:
        value.encode('ascii')
    except UnicodeEncodeError:
        raise ValidationError(f"{name} must contain only ASCII characters")
    if len(value) > max_len:
        raise ValidationError(f"{name} must be at most {max_len} characters, got {len(value)}")
    return value

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
    """Wait for the prompt byte (0x4D or 0x4C)"""
    prompt = ser.read(1)
    if len(prompt) != 1 or prompt[0] not in (0x4D, 0x4C):
        raise Exception(f"Expected prompt 0x4D or 0x4C, got {prompt.hex() if prompt else 'nothing'}")
    print("Received prompt")

def read_hardware_data(ser):
    """Read hardware data (command 0x71) and display it"""
    print("\nReading hardware data...")
    send_command(ser, 0x71)
    
    # Read 64 bytes of hardware data
    data = ser.read(64)
    if len(data) != 64:
        raise Exception(f"Expected 64 bytes, got {len(data)}")
    
    # Parse primary manufacturing data (first 52 bytes)
    primary_serial = int.from_bytes(data[0:2], byteorder='little')
    primary_licence = data[2]
    primary_revision = data[3]
    primary_year = data[4]
    primary_month = data[5]
    primary_day = data[6]
    primary_info = data[7:51].rstrip(b'\x00\xff').decode('ascii', errors='replace')
    primary_checksum = data[51]
    
    # Parse secondary manufacturing data (last 12 bytes)
    secondary_serial = int.from_bytes(data[52:54], byteorder='little')
    secondary_year = data[54]
    secondary_month = data[55]
    secondary_day = data[56]
    secondary_reason = data[57]
    secondary_info = data[58:62].rstrip(b'\x00\xff').decode('ascii', errors='replace')
    secondary_checksum = data[62]
    
    # Display primary data
    print("\n=== Primary Manufacturing Data ===")
    print(f"Serial Number: {primary_serial} (0x{primary_serial:04X})")
    print(f"Licence: {primary_licence}")
    print(f"Revision: {primary_revision}")
    if primary_year != 0xFF:
        print(f"Production Date: 20{primary_year:02d}-{primary_month:02d}-{primary_day:02d}")
    else:
        print("Production Date: Not set")
    if primary_info:
        print(f"Info: {primary_info}")
    print(f"Checksum: 0x{primary_checksum:02X}")
    
    # Display secondary data
    print("\n=== Secondary Manufacturing Data ===")
    if secondary_serial != 0xFFFF:
        print(f"Secondary Serial: {secondary_serial} (0x{secondary_serial:04X})")
        print(f"Secondary Date: 20{secondary_year:02d}-{secondary_month:02d}-{secondary_day:02d}")
        print(f"Reason Code: {secondary_reason}")
        if secondary_info:
            print(f"Info: {secondary_info}")
        print(f"Checksum: 0x{secondary_checksum:02X}")
    else:
        print("Secondary Serial: Not set")
    
    wait_for_prompt(ser)

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
    
    Raises:
        ValidationError: If any parameter is invalid
    """
    # Validate all inputs
    validate_serial(serial_num, "Primary serial")
    validate_byte(licence, MIN_LICENCE, MAX_LICENCE, "Licence")
    validate_byte(revision, MIN_REVISION, MAX_REVISION, "Revision")
    validate_date(year, month, day)
    validate_info_string(info, MAX_PROD_INFO_LEN, "Production info")
    
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
    
    Raises:
        ValidationError: If any parameter is invalid
    """
    # Validate all inputs
    validate_serial(serial_num, "Secondary serial")
    validate_byte(licence, MIN_LICENCE, MAX_LICENCE, "Secondary licence")
    validate_byte(reason, MIN_REASON, MAX_REASON, "Reason")
    validate_date(year, month, day)
    validate_info_string(info, MAX_SEC_INFO_LEN, "Secondary info")
    
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


def parse_date(date_str):
    """Parse date string in YYYY-MM-DD format, return (year_offset, month, day)"""
    try:
        dt = datetime.strptime(date_str, "%Y-%m-%d")
        year_offset = dt.year - 2000
        if year_offset < 0 or year_offset > 99:
            raise ValueError(f"Year must be between 2000 and 2099")
        return year_offset, dt.month, dt.day
    except ValueError as e:
        raise argparse.ArgumentTypeError(f"Invalid date '{date_str}': {e}")


def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(
        description="Set manufacturing data on OSTC4 via Bluetooth serial connection.",
        epilog="""
Examples:
  # Set primary manufacturing data
  %(prog)s /dev/rfcomm0 --serial 428 --date 2018-02-12 --info "OSTC 4 end-2019 hardware"
  
  # Set primary and secondary serial
  %(prog)s /dev/rfcomm0 --serial 428 --date 2018-02-12 --secondary-serial 428 --secondary-date 2024-12-25
  
  # Only set Bluetooth name (device must have serial already set)
  %(prog)s /dev/rfcomm0 --set-bluetooth-name
        """,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument('port', help='Serial port (e.g., /dev/rfcomm0)')
    
    # Primary manufacturing data
    primary = parser.add_argument_group('Primary manufacturing data (command 0x80)')
    primary.add_argument('--serial', '-s', type=int, 
                        help=f'Primary serial number ({MIN_SERIAL}-{MAX_SERIAL})')
    primary.add_argument('--licence', '-l', type=int, default=0xFF,
                        help=f'Licence byte ({MIN_LICENCE}-{MAX_LICENCE}, default: 255)')
    primary.add_argument('--revision', '-r', type=int, default=0x02,
                        help=f'Hardware revision ({MIN_REVISION}-{MAX_REVISION}, default: 2)')
    primary.add_argument('--date', '-d', type=parse_date,
                        help='Production date in YYYY-MM-DD format')
    primary.add_argument('--info', '-i', type=str, default='',
                        help=f'Production info string (max {MAX_PROD_INFO_LEN} chars)')
    
    # Secondary serial data  
    secondary = parser.add_argument_group('Secondary serial data (command 0x81)')
    secondary.add_argument('--secondary-serial', type=int,
                          help=f'Secondary serial number ({MIN_SERIAL}-{MAX_SERIAL})')
    secondary.add_argument('--secondary-licence', type=int, default=0xFF,
                          help=f'Secondary licence ({MIN_LICENCE}-{MAX_LICENCE}, default: 255)')
    secondary.add_argument('--secondary-reason', type=int, default=0x00,
                          help=f'Reason code ({MIN_REASON}-{MAX_REASON}, default: 0)')
    secondary.add_argument('--secondary-date', type=parse_date,
                          help='Secondary date in YYYY-MM-DD format')
    secondary.add_argument('--secondary-info', type=str, default='',
                          help=f'Secondary info string (max {MAX_SEC_INFO_LEN} chars)')
    
    # Bluetooth name
    bt = parser.add_argument_group('Bluetooth name (command 0x82)')
    bt.add_argument('--set-bluetooth-name', '-b', action='store_true',
                   help='Set Bluetooth name based on serial number')
    
    # Read command
    parser.add_argument('--read', action='store_true',
                       help='Read current manufacturing data (command 0x71)')
    
    # Other options
    parser.add_argument('--timeout', '-t', type=float, default=5.0,
                       help='Serial timeout in seconds (default: 5.0)')
    
    args = parser.parse_args()
    
    # Validation: at least one action must be specified
    has_primary = args.serial is not None
    has_secondary = args.secondary_serial is not None
    has_bt = args.set_bluetooth_name
    has_read = args.read
    
    if not has_primary and not has_secondary and not has_bt and not has_read:
        parser.error("At least one of --serial, --secondary-serial, --set-bluetooth-name, or --read must be specified")
    
    # If primary serial is set, date is required
    if has_primary and args.date is None:
        parser.error("--date is required when setting primary serial")
    
    # If secondary serial is set, secondary date is required
    if has_secondary and args.secondary_date is None:
        parser.error("--secondary-date is required when setting secondary serial")
    
    return args


def main():
    args = parse_args()
    
    try:
        print(f"Opening {args.port}...")
        ser = serial.Serial(args.port, 115200, timeout=args.timeout)
        time.sleep(0.5)  # Give the connection time to stabilize
        
        # Flush any pending data
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Initialize service mode
        send_service_mode_init(ser)
        
        # Read hardware data if requested
        if args.read:
            read_hardware_data(ser)
            if not args.serial and not args.secondary_serial and not args.set_bluetooth_name:
                # Only reading, exit cleanly
                exit_service_mode(ser)
                print("\nDone!")
                ser.close()
                return
        
        # Write primary production data if specified
        if args.serial is not None:
            year, month, day = args.date
            # Pad info to 44 chars with spaces
            info = args.info.ljust(MAX_PROD_INFO_LEN)[:MAX_PROD_INFO_LEN]
            write_production_data(ser, args.serial, args.licence, args.revision,
                                year, month, day, info)
        
        # Write secondary serial if specified
        if args.secondary_serial is not None:
            year, month, day = args.secondary_date
            # Pad info to 4 chars with spaces
            info = args.secondary_info.ljust(MAX_SEC_INFO_LEN)[:MAX_SEC_INFO_LEN]
            write_secondary_serial(ser, args.secondary_serial, args.secondary_licence,
                                 args.secondary_reason, year, month, day, info)
        
        # Set Bluetooth name if requested
        if args.set_bluetooth_name:
            set_bluetooth_name(ser)
            print("\nThe device will now configure the Bluetooth module.")
            print("Wait for the bootloader to finish and reconnect.")
        
        if not args.set_bluetooth_name:
            # Exit service mode cleanly if we didn't trigger BT name setting
            # (BT name setting causes device to exit comm mode automatically)
            exit_service_mode(ser)
        
        print("\nDone!")
        ser.close()
        
    except serial.SerialException as e:
        print(f"Serial error: {e}", file=sys.stderr)
        sys.exit(1)
    except ValidationError as e:
        print(f"Validation error: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
