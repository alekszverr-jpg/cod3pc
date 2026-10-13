import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from check_campaign import check_campaign

TEMP_ROOT = Path(__file__).resolve().parents[1] / 'reports/test-temp'
TEMP_ROOT.mkdir(parents=True, exist_ok=True)


def synthetic_xex():
    data = bytearray(40 + 0x17C)
    data[:4] = b'XEX2'
    struct.pack_into('>5I', data, 4, 8, 40, 0, 40, 2)
    struct.pack_into('>4I', data, 24, 0x10100, 0x89050000, 0x10201, 0x89000000)
    struct.pack_into('>I', data, 44, 0x100000)
    struct.pack_into('>I', data, 40 + 0x110, 0x89000000)
    return data


class CampaignCoverage(unittest.TestCase):
    def fixture(self, root):
        game = root / 'assets/game'
        folder = game / 'sp/test_level'
        folder.mkdir(parents=True)
        (folder / 'module.bin').write_bytes(synthetic_xex())
        (folder / 'decoy.dll').write_bytes(b'ordinary data')
        manifest = root / 'cod3pc/project.toml'
        manifest.parent.mkdir()
        return game, manifest

    def test_magic_detects_hidden_module_and_rejects_dll_decoy(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            game, manifest = self.fixture(Path(tmp))
            manifest.write_text('[project]\nname="fixture"\n', encoding='utf-8')
            report = check_campaign(game, manifest)
            self.assertEqual(report['count'], 1)
            self.assertEqual(report['declared_count'], 0)
            self.assertEqual(report['campaign_modules'][0]['path'], 'sp/test_level/module.bin')

    def test_manifest_source_and_guest_path_are_checked_separately(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            game, manifest = self.fixture(Path(tmp))
            manifest.write_text('[[modules]]\nfile_path="../assets/game/sp/test_level/module.bin"\n'
                                'guest_path="sp/wrong/module.bin"\n', encoding='utf-8')
            report = check_campaign(game, manifest)
            self.assertEqual(report['declared_count'], 1)
            self.assertFalse(report['campaign_modules'][0]['guest_path_matches'])

    def test_duplicate_source_is_rejected(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            game, manifest = self.fixture(Path(tmp))
            module = '[[modules]]\nfile_path="../assets/game/sp/test_level/module.bin"\n'
            manifest.write_text(module * 2, encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'duplicate module source'):
                check_campaign(game, manifest)


if __name__ == '__main__':
    unittest.main()
