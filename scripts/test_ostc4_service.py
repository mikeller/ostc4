"""Mocked command-0x71 response tests; run with python -m unittest scripts.test_ostc4_service."""

import contextlib
import io
import unittest
from unittest.mock import Mock, call

from scripts import ostc4_service


class ReadHardwareDataTest(unittest.TestCase):
    def read_response(self, data):
        ser = Mock()
        ser.read.side_effect = [b'\x71', bytes(data), b'\x4d']
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            ostc4_service.read_hardware_data(ser)
        ser.write.assert_called_once_with(b'\x71')
        self.assertEqual(ser.read.call_args_list, [call(1), call(64), call(1)])
        return output.getvalue()

    def test_displays_both_raw_flags_and_manufacturing_fields(self):
        data = bytearray(64)
        data[0:8] = bytes([0x34, 0x12, 3, 2, 24, 10, 31, 0x00])
        data[8:52] = b'Primary info'.ljust(44, b'\x00')
        data[52:60] = bytes([0x78, 0x56, 4, 5, 25, 1, 2, 0x7E])
        data[60:64] = b'TEST'

        output = self.read_response(data)
        primary, secondary = output.split('=== Secondary Manufacturing Data ===')
        self.assertIn('Serial Number: 4660 (0x1234)', primary)
        self.assertIn('Licence: 3', primary)
        self.assertIn('Revision: 2', primary)
        self.assertIn('Production Date: 2024-10-31', primary)
        self.assertIn('Info: Primary info', primary)
        self.assertIn('Bluetooth Name Flag: 0x00', primary)
        self.assertIn('Secondary Serial: 22136 (0x5678)', secondary)
        self.assertIn('Secondary Licence: 4', secondary)
        self.assertIn('Secondary Date: 2025-01-02', secondary)
        self.assertIn('Reason Code: 5', secondary)
        self.assertIn('Info: TEST', secondary)
        self.assertIn('Bluetooth Name Flag: 0x7E', secondary)

    def test_displays_secondary_flag_with_unset_secondary_serial(self):
        data = bytearray(64)
        data[52:54] = b'\xff\xff'
        data[7] = 0xA5
        data[59] = 0xFF

        output = self.read_response(data)
        primary, secondary = output.split('=== Secondary Manufacturing Data ===')
        self.assertIn('Bluetooth Name Flag: 0xA5', primary)
        self.assertIn('Secondary Serial: Not set', secondary)
        self.assertIn('Bluetooth Name Flag: 0xFF', secondary)
        self.assertNotIn('Secondary Licence:', secondary)

    def test_rejects_short_response_before_display_or_prompt(self):
        ser = Mock()
        ser.read.side_effect = [b'\x71', bytes(63)]
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            with self.assertRaisesRegex(Exception, 'Expected 64 bytes, got 63'):
                ostc4_service.read_hardware_data(ser)
        self.assertNotIn('Manufacturing Data', output.getvalue())
        self.assertEqual(ser.read.call_count, 2)
