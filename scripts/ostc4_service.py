#!/usr/bin/env python3
"""
Service OSTC4 manufacturing data and Bluetooth names over RFCOMM or a serial port.

This script implements the OSTC4 service mode protocol to set:
- 0x80: Write manufacturing data (52 bytes)
- 0x81: Write secondary serial (12 bytes)  
- 0x82: Set Bluetooth name

The device must be in bootloader communication mode and connected via Bluetooth.
"""

import argparse
import select
import socket
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


class RFCOMMConnection:
    """Provide the serial-like interface used by the OSTC service protocol."""

    def __init__(self, address, channel, timeout):
        self.socket = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM,
                                    socket.BTPROTO_RFCOMM)
        self.timeout = timeout
        self.socket.connect((address, channel))

    @property
    def timeout(self):
        return self.socket.gettimeout()

    @timeout.setter
    def timeout(self, value):
        self.socket.settimeout(value)

    def write(self, data):
        self.socket.sendall(data)
        return len(data)

    def read(self, size):
        data = bytearray()
        while len(data) < size:
            try:
                chunk = self.socket.recv(size - len(data))
            except socket.timeout:
                break
            if not chunk:
                break
            data.extend(chunk)
        return bytes(data)

    def reset_input_buffer(self):
        while select.select([self.socket], [], [], 0)[0]:
            if not self.socket.recv(256):
                break

    def reset_output_buffer(self):
        pass

    def close(self):
        self.socket.close()


def open_connection(args, timeout=None):
    timeout = args.timeout if timeout is None else timeout
    if args.address:
        print(f"Connecting to {args.address} RFCOMM channel {args.channel}...")
        return RFCOMMConnection(args.address, args.channel, timeout)

    try:
        import serial
    except ImportError as error:
        raise RuntimeError("pyserial is required for serial-port connections") from error

    print(f"Opening {args.port}...")
    return serial.Serial(args.port, 115200, timeout=timeout)


def open_service_connection(args, attempts=1):
    last_error = None
    for attempt in range(attempts):
        connection = None
        try:
            connection = open_connection(args, min(args.timeout, 2) if attempts > 1 else None)
            time.sleep(0.5)
            connection.reset_input_buffer()
            connection.reset_output_buffer()
            send_service_mode_init(connection)
            connection.timeout = args.timeout
            return connection
        except Exception as error:
            last_error = error
            if connection:
                connection.close()
            if attempt + 1 < attempts:
                print("Waiting for bootloader Bluetooth service...")
                time.sleep(2)
    raise last_error


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
    # Structure: serial(2) + licence(1) + revision(1) + date(3) + bt_name_set(1) + info(44)
    primary_serial = int.from_bytes(data[0:2], byteorder='little')
    primary_licence = data[2]
    primary_revision = data[3]
    primary_year = data[4]
    primary_month = data[5]
    primary_day = data[6]
    # data[7] = production_bluetooth_name_set (skip)
    primary_info = data[8:52].rstrip(b'\x00\xff').decode('ascii', errors='replace')
    
    # Parse secondary manufacturing data (last 12 bytes)
    # Structure: serial(2) + licence(1) + reason(1) + date(3) + bt_name_set(1) + info(4)
    secondary_serial = int.from_bytes(data[52:54], byteorder='little')
    secondary_licence = data[54]
    secondary_reason = data[55]
    secondary_year = data[56]
    secondary_month = data[57]
    secondary_day = data[58]
    # data[59] = secondary_bluetooth_name_set (skip)
    secondary_info = data[60:64].rstrip(b'\x00\xff').decode('ascii', errors='replace')
    
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
    
    # Display secondary data
    print("\n=== Secondary Manufacturing Data ===")
    if secondary_serial != 0xFFFF:
        print(f"Secondary Serial: {secondary_serial} (0x{secondary_serial:04X})")
        print(f"Secondary Licence: {secondary_licence}")
        if secondary_year != 0xFF:
            print(f"Secondary Date: 20{secondary_year:02d}-{secondary_month:02d}-{secondary_day:02d}")
        else:
            print("Secondary Date: Not set")
        print(f"Reason Code: {secondary_reason}")
        if secondary_info:
            print(f"Info: {secondary_info}")
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
    ser.write(b'\x82')
    try:
        response = ser.read(1)
    except OSError:
        response = b''

    if response in (b'\x4c', b'\x4d'):
        print("Application accepted the command and is restarting into the bootloader")
        return True

    if response != b'\x82':
        raise Exception(f"Command 0x82 was not acknowledged: got {response.hex() if response else 'nothing'}")

    print("Command 0x82 acknowledged by bootloader")
    # The bootloader exits communication mode immediately to configure Bluetooth.
    print("Bootloader is configuring the Bluetooth module")
    return False

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
        description="Service OSTC4 manufacturing data and Bluetooth names over RFCOMM or a serial port.",
        epilog="""
Examples:
  # Set primary manufacturing data
   %(prog)s --address 00:80:25:4A:E5:FD --serial 428 --date 2018-02-12 --info "OSTC 4 end-2019 hardware"
  
  # Set primary and secondary serial
   %(prog)s --address 00:80:25:4A:E5:FD --serial 428 --date 2018-02-12 --secondary-serial 428 --secondary-date 2024-12-25
  
   # Only set Bluetooth name (device must be in bootloader mode and have a serial)
   %(prog)s --address 00:80:25:4A:E5:FD --set-bluetooth-name

  # Use an existing RFCOMM serial port mapping
  %(prog)s /dev/rfcomm0 --set-bluetooth-name
        """,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    
    parser.add_argument('port', nargs='?', help='Serial port (e.g., /dev/rfcomm0)')
    connection = parser.add_argument_group('Connection')
    connection.add_argument('--address', help='Classic Bluetooth address for direct RFCOMM connection')
    connection.add_argument('--channel', type=int, default=1,
                            help='RFCOMM channel for --address (default: 1)')
    
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

    if bool(args.port) == bool(args.address):
        parser.error("Specify exactly one of a serial port or --address")

    if not 1 <= args.channel <= 30:
        parser.error("--channel must be between 1 and 30")
    
    # If primary serial is set, date is required
    if has_primary and args.date is None:
        parser.error("--date is required when setting primary serial")
    
    # If secondary serial is set, secondary date is required
    if has_secondary and args.secondary_date is None:
        parser.error("--secondary-date is required when setting secondary serial")
    
    return args


def main():
    args = parse_args()
    needs_reinit_for_bt_name = False
    
    try:
        ser = open_service_connection(args)
        
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
            try:
                write_production_data(ser, args.serial, args.licence, args.revision,
                                    year, month, day, info)
            except Exception as e:
                # If the flash area is already programmed, the bootloader returns
                # without sending a prompt. Allow continuing to BT name set.
                if args.set_bluetooth_name and "Expected prompt" in str(e):
                    print(f"Warning: {e}. Assuming production data already set; continuing to Bluetooth name.")
                    needs_reinit_for_bt_name = True
                else:
                    raise
        
        # Write secondary serial if specified
        if args.secondary_serial is not None:
            year, month, day = args.secondary_date
            # Pad info to 4 chars with spaces
            info = args.secondary_info.ljust(MAX_SEC_INFO_LEN)[:MAX_SEC_INFO_LEN]
            write_secondary_serial(ser, args.secondary_serial, args.secondary_licence,
                                 args.secondary_reason, year, month, day, info)
        
        # Set Bluetooth name if requested
        if args.set_bluetooth_name:
            try:
                application_restarted = set_bluetooth_name(ser)
                if application_restarted:
                    ser.close()
                    ser = open_service_connection(args, attempts=20)
                    set_bluetooth_name(ser)
            except Exception as e:
                if needs_reinit_for_bt_name and "was not acknowledged" in str(e):
                    print(f"Warning: {e}. Bluetooth disconnected after 0x80.")
                    print("Please reconnect the device, then press Enter.")
                    try:
                        ser.close()
                    except Exception as close_err:
                        print(f"Note: could not close port: {close_err}")
                    try:
                        input()
                    except EOFError:
                        raise Exception("Reconnection required but no input available.")
                    ser = open_service_connection(args, attempts=20)
                    set_bluetooth_name(ser)
                else:
                    raise
            print("\nWait for the bootloader to finish configuring the Bluetooth module.")
        
        if not args.set_bluetooth_name:
            # Exit service mode cleanly if we didn't trigger BT name setting
            # (BT name setting causes device to exit comm mode automatically)
            exit_service_mode(ser)
        
        print("\nDone!")
        ser.close()
        
    except ValidationError as e:
        print(f"Validation error: {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
