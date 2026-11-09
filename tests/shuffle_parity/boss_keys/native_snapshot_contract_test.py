#!/usr/bin/env python3
"""Snapshot serialization contracts; no native graph/fill execution is claimed."""
import unittest

import native_boss_key_regression as native


class SnapshotContracts(unittest.TestCase):
    def setUp(self):
        # Native numeric order deliberately opposes lexical source-name order.
        self.z = 'RC_STONE_TOWER_TEMPLE_INVERTED_Z_TEST'
        self.a = 'RC_STONE_TOWER_TEMPLE_INVERTED_A_TEST'
        self.normal = 'RC_STONE_TOWER_TEMPLE_A_TEST'
        self.data = {'checkMetadata': {
            'A': {'id': 20, 'sourceName': self.a},
            'Normal': {'id': 1, 'sourceName': self.normal},
            'Z': {'id': 3, 'sourceName': self.z},
            'Other': {'id': 2, 'sourceName': 'RC_SNOWHEAD_TEMPLE_MAP_CHEST'},
        }}

    def assert_snapshot_matches(self, actual, key, expected):
        # Same strict comparison used by preflight, solo and combined validation.
        native.require(native.saved_setting(actual, key) == expected, 'snapshot differs')

    def test_mm_requested_order_matches_native_numeric_serialization(self):
        expected = native.exclusions('stone-normal', 'mm', self.data)
        self.assertEqual(expected, [self.z, self.a])
        self.assertNotEqual(expected, sorted(expected))
        self.assertEqual(len(expected), len(set(expected)))
        self.assert_snapshot_matches({'gRando.ExcludedChecks': expected}, 'gRando.ExcludedChecks', expected)

    def test_exact_exclusion_comparison_rejects_missing_unknown_duplicate_and_order(self):
        expected = native.exclusions('stone-normal', 'mm', self.data)
        key = 'gRando.ExcludedChecks'
        for actual in ({}, {key: expected[:-1]}, {key: expected + ['RC_UNKNOWN']},
                       {key: expected + expected[:1]}, {key: list(reversed(expected))}):
            with self.subTest(actual=actual), self.assertRaises(AssertionError):
                self.assert_snapshot_matches(actual, key, expected)

    def test_other_orientation_retains_exact_membership(self):
        self.assertEqual(native.exclusions('stone-inverted', 'mm', self.data), [self.normal])
        self.assertEqual(native.exclusions('own', 'mm', self.data), [])

    def test_oot_exclusion_order_remains_lexical(self):
        data = {'checkMetadata': {
            'Z': {'id': 1, 'sourceName': 'RC_Z_BOSS_KEY_CHEST'},
            'A': {'id': 9, 'sourceName': 'RC_A_BOSS_KEY_CHEST'},
        }}
        self.assertEqual(native.exclusions('own-excluded', 'oot', data),
                         ['RC_A_BOSS_KEY_CHEST', 'RC_Z_BOSS_KEY_CHEST'])

    def test_only_legacy_catalog_value_may_be_omitted(self):
        key = 'gRandoSettings.RustyDoorKeyCatalogVersion'
        self.assert_snapshot_matches({}, key, 0)
        self.assert_snapshot_matches({key: 0}, key, 0)
        self.assert_snapshot_matches({key: 1}, key, 1)
        for actual, name, value in (({}, key, 1), ({key: 0}, key, 1),
                                    ({}, 'gRandoSettings.LockOverworldDoors', 0),
                                    ({}, 'gRando.StartingItems', [])):
            with self.subTest(actual=actual, key=name), self.assertRaises(AssertionError):
                self.assert_snapshot_matches(actual, name, value)


if __name__ == '__main__':
    unittest.main()
