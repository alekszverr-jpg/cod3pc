import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from list_debug_levels import list_debug_levels, select_debug_level
from test_campaign import synthetic_xex, TEMP_ROOT


class DebugLevels(unittest.TestCase):
    def fixture(self, root, guest='sp/mission/module.bin', command='actual_map'):
        game = root / 'assets/game'
        folder = game / 'sp/mission'
        folder.mkdir(parents=True)
        (folder / 'module.bin').write_bytes(synthetic_xex())
        manifest = root / 'cod3pc/project.toml'
        manifest.parent.mkdir()
        manifest.write_text('[project]\nname="fixture"\n[[modules]]\n'
                            'file_path="../assets/game/sp/mission/module.bin"\n'
                            f'guest_path="{guest}"\ndebug_map="{command}"\n')
        build = root / 'build'
        build.mkdir()
        return game, manifest, build

    def test_native_availability_and_actual_map_are_required(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            game, manifest, build = self.fixture(Path(tmp))
            levels = list_debug_levels(game, manifest, build)
            with self.assertRaises(ValueError):
                select_debug_level(levels, 'mission')
            (build / 'fixture_module.dll').write_bytes(b'MZsynthetic native fixture')
            selected = select_debug_level(list_debug_levels(game, manifest, build), 'mission')
            self.assertEqual(selected['map'], 'actual_map')

    def test_wrong_guest_path_blocks_existing_native_dll(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            game, manifest, build = self.fixture(Path(tmp), guest='sp/wrong/module.bin')
            (build / 'fixture_module.dll').write_bytes(b'MZsynthetic native fixture')
            self.assertFalse(list_debug_levels(game, manifest, build)[0]['ready'])

    def test_map_command_injection_is_rejected(self):
        with tempfile.TemporaryDirectory(dir=TEMP_ROOT) as tmp:
            game, manifest, build = self.fixture(Path(tmp), command='map;quit')
            with self.assertRaisesRegex(ValueError, 'unsafe debug map'):
                list_debug_levels(game, manifest, build)

    def test_unknown_level_cannot_be_selected(self):
        with self.assertRaisesRegex(ValueError, 'unknown debug level'):
            select_debug_level([], '../island')


if __name__ == '__main__':
    unittest.main()
