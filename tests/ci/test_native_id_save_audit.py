"""Audit tooling must reject omissions and preserve escaped native labels."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('native_id_save_audit', ROOT / 'scripts/diagnostics/native_id_save_audit.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class NativeAuditParserTests(unittest.TestCase):
    def test_terminal_sentinel_may_move_but_cannot_disappear(self):
        self.assertEqual(audit.compare({'Ids': {'VALUE': 1, 'ID_MAX': 2}}, {'Ids': {'VALUE': 1, 'ID_MAX': 3}})['incompatibleChanges'], [])
        self.assertEqual(len(audit.compare({'Ids': {'VALUE': 1, 'ID_MAX': 2}}, {'Ids': {'VALUE': 1}})['incompatibleChanges']), 1)

    def test_escaped_comma_in_real_style_label_does_not_shift_default(self):
        with tempfile.TemporaryDirectory(prefix='native-option-parser-') as td:
            root = Path(td)
            target = root / 'soh/soh/Enhancements/randomizer/settings.cpp'
            target.parent.mkdir(parents=True)
            target.write_text(r'''OPT_U8(RSK_FOREST, "Native label", {"Off", "On"}, OptionCategory::Setting,
                CVAR_RANDOMIZER_SETTING("ClosedForest"), "Quoted \"alpha,beta\" text",
                WIDGET_CVAR_COMBOBOX, RO_GENERIC_OFF);''')
            row = audit.option_calls(root)['RSK_FOREST']
            self.assertEqual(row['defaultExpression'], 'RO_GENERIC_OFF')
            self.assertEqual(row['cvarExpression'], 'CVAR_RANDOMIZER_SETTING("ClosedForest")')


if __name__ == '__main__':
    unittest.main()
