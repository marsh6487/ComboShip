#!/usr/bin/env python3
"""Schema-level controls for real-output validation; these are not native graph proof."""
from copy import deepcopy
from pathlib import Path
import unittest
from unittest.mock import patch
import native_boss_key_regression as native


class NativeOutputValidatorTests(unittest.TestCase):
    def sample(self, game):
        table = native.scenes(game)
        placements = {}; rows = {}
        for key, tokens in native.KEY_SCENES[game].items():
            check = 'check:' + key
            placements[check] = key + (' (OOT)' if game == 'oot' else ' (MM)')
            rows[check] = {'scene': table[tokens[0]], 'sourceName': 'RC_' + key.upper().replace(' ', '_'), 'excluded': False}
        return placements, {'checkMetadata': rows}

    def test_tagged_native_names_and_all_physical_owner_scenes(self):
        for game in ('oot', 'mm'):
            placements, data = self.sample(game)
            self.assertEqual(len(native.validate_keys(game, placements, data)), len(native.KEY_SCENES[game]))
        placements, data = self.sample('mm')
        check = 'check:Stone Tower Boss Key'
        for token in native.KEY_SCENES['mm']['Stone Tower Boss Key']:
            data['checkMetadata'][check]['scene'] = native.scenes('mm')[token]
            native.validate_keys('mm', placements, data)

    def test_wrong_scene_missing_duplicate_and_excluded_are_rejected(self):
        placements, data = self.sample('mm')
        check = 'check:Stone Tower Boss Key'
        bad = deepcopy(data); bad['checkMetadata'][check]['scene'] = -999
        with self.assertRaises(AssertionError): native.validate_keys('mm', placements, bad)
        with self.assertRaises(AssertionError): native.validate_keys('mm', {k:v for k,v in placements.items() if k != check}, data)
        with self.assertRaises(AssertionError): native.validate_keys('mm', {**placements, 'duplicate': placements[check]}, data)
        bad = deepcopy(data); bad['checkMetadata'][check]['excluded'] = True
        with self.assertRaises(AssertionError): native.validate_keys('mm', placements, bad)
        source = data['checkMetadata'][check]['sourceName']
        with self.assertRaises(AssertionError): native.validate_keys('mm', placements, data, [source])

    def test_stone_orientation_uses_native_source_identity(self):
        placements, data = self.sample('mm')
        row = data['checkMetadata']['check:Stone Tower Boss Key']
        row['sourceName'] = 'RC_STONE_TOWER_TEMPLE_INVERTED_BOSS_KEY_CHEST'
        native.validate_keys('mm', placements, data, orientation='stone-inverted')
        with self.assertRaises(AssertionError): native.validate_keys('mm', placements, data, orientation='stone-normal')

    def test_foreign_ownership_comes_from_records(self):
        spoiler = {'oot': {'placements': {'A': 'Foreign Key (MM)', 'B': 'Native Key (OOT)'}},
                   'mm': {'placements': {'C': 'Foreign Bow (OOT)'}},
                   'foreign': [{'checkGame': 'oot', 'checkName': 'A', 'itemGame': 'mm', 'itemName': 'Foreign Key'},
                               {'checkGame': 'mm', 'checkName': 'C', 'itemGame': 'oot', 'itemName': 'Foreign Bow'}]}
        owned = native.owner_placements(spoiler)
        self.assertEqual(owned['mm'], {'oot:A': 'Foreign Key'})
        self.assertEqual(owned['oot'], {'oot:B': 'Native Key (OOT)', 'mm:C': 'Foreign Bow'})

    def test_mm_solo_replay_installs_exact_generated_prices_before_reset(self):
        events=[]
        generated={'native paid check':17,'native free check':0,'goal':0}
        class NativeBoundary:
            prices={'native paid check':199,'native free check':88}
            def call(self,name,*values,**kwargs):
                events.append(name)
                if name=='SetComboRandoSeed':self.seed=values[0]
                elif name=='SetCheckPrices':self.prices=native.json.loads(values[0])
                else:raise AssertionError('unexpected native setup call: '+name)
            def bind(self,name,*args,**kwargs):
                def reset():
                    self_outer.assertEqual(self.prices,generated)
                    self_outer.assertEqual(self.seed,1234)
                    events.append('Reset')
                return {'Combo_MM_Rando_Reset':reset,'Combo_MM_Rando_SetOwnedItems':lambda _:None,
                        'Combo_MM_Rando_GetReachableChecks':lambda:native.json.dumps(list(generated)).encode(),
                        'Combo_MM_Rando_Restore':lambda:events.append('Restore')}[name]
        self_outer=self
        filled={'finalSeed':1234,'prices':generated,'placements':{x:'junk' for x in generated},
                'checks':{x:{'shuffled':False} for x in generated}}
        data={'checkMetadata':{'goal':{'sourceName':'RC_MOON_MAJORA_POT_01'}},'items':[]}
        native.mm_replay(NativeBoundary(),filled,data)
        self.assertEqual(events[:3],['SetComboRandoSeed','SetCheckPrices','Reset'])
        self.assertEqual(events[-1],'Restore')

    def test_mm_solo_replay_rejects_missing_or_untyped_prices_before_native_calls(self):
        filled={'finalSeed':1234,'placements':{'goal':'junk'},'checks':{'goal':{'shuffled':False}}}
        for prices in (None,{}, {'goal':True}, {'goal':-1}, {'goal':65536}):
            candidate={**filled}
            if prices is not None:candidate['prices']=prices
            with self.assertRaises(AssertionError):native.mm_replay(object(),candidate,{})

    def test_native_environment_identity_set_respects_solo_acquisition_routes(self):
        catalog = native.feature_catalogue()
        combined = native.expected_environment('all-parity', True, [])
        solo = native.expected_environment('all-parity', False, [])
        self.assertEqual(combined, catalog['environment'])
        self.assertEqual(combined - solo, catalog['ironBootRocks'])
        self.assertFalse(solo & catalog['ironBootRocks'])
        self.assertEqual(native.expected_environment('all-off', False, []), set())
        with self.assertRaises(AssertionError):
            native.expected_environment('all-parity', False, ['RI_OOT_IRON_BOOTS'])

    def test_all_features_are_explicit_and_new_features_off_by_default(self):
        on = native.settings_for('all-parity'); off = native.settings_for('all-off')
        for key in native.OOT_FEATURES:
            self.assertEqual(on['oot']['gRandoSettings.' + key], 1)
            self.assertEqual(off['oot']['gRandoSettings.' + key], 0)
        for key in native.MM_FEATURES:
            self.assertEqual(on['mm']['gRando.Options.RO_' + key], 15 if key == 'SHUFFLE_KEY_RINGS' else 1)
            self.assertEqual(off['mm']['gRando.Options.RO_' + key], 0)
        self.assertEqual(on['oot']['gRandoSettings.BossKeysanity'], 2)
        self.assertEqual(on['mm']['gRando.Options.RO_PLACEMENT_BOSS_KEYS'], 1)


if __name__ == '__main__': unittest.main()
