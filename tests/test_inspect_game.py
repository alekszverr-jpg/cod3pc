import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from inspect_game import inspect_xex, version
TEMP_ROOT = Path(__file__).resolve().parents[1] / 'reports' / 'test-temp'
TEMP_ROOT.mkdir(parents=True, exist_ok=True)


class XexHeaders(unittest.TestCase):
    def test_version_msb_layout(self):
        self.assertEqual(version(0x12345678), '1.2.13398.120')
        self.assertEqual(version(1), '0.0.0.1')

    def test_truncation_rejected(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            path = Path(tmp) / 'bad.xex'
            path.write_bytes(b'XEX2' + bytes(20))
            with self.assertRaises(ValueError):
                inspect_xex(path)

    def test_optional_header_count_checked(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            path = Path(tmp) / 'bad.xex'
            path.write_bytes(b'XEX2' + struct.pack('>5I', 0, 24, 0, 24, 1000))
            with self.assertRaises(ValueError):
                inspect_xex(path)


if __name__ == '__main__':
    unittest.main()
