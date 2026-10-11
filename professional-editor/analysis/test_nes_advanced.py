"""Unit tests for advanced NES research helpers."""
import importlib.util
from pathlib import Path
import unittest

HERE = Path(__file__).parent
spec = importlib.util.spec_from_file_location("nes_advanced", HERE / "nes_advanced.py")
nes_advanced = importlib.util.module_from_spec(spec)
import sys
sys.modules[spec.name] = nes_advanced
spec.loader.exec_module(nes_advanced)

class AdvancedAnalysisTests(unittest.TestCase):
    def test_address_regions(self):
        self.assertIn("CPU RAM", nes_advanced.classify_address("0x0801")["region"])
        self.assertIn("PPU register", nes_advanced.classify_address("0x2008")["region"])
        self.assertIn("OAM DMA", nes_advanced.classify_address("0x4014")["region"])
        self.assertIn("save RAM", nes_advanced.classify_address("0x6000")["region"])
        self.assertIn("mapper-controlled", nes_advanced.classify_address("0x8000")["region"])

    def test_mapper_profiles(self):
        self.assertEqual(nes_advanced.mapper_profile(0, 16384)["name"], "NROM")
        self.assertEqual(nes_advanced.mapper_profile(2, 65536)["windows"][0]["banking"], "switchable-16KiB")
        self.assertFalse(nes_advanced.mapper_profile(999, 32768)["supported"])

    def test_rules_order_evaluation(self):
        rules = {"schema_version":"1.0", "title":"Test", "objective":"test objective",
            "constraints":[], "actions":[
                {"address":"0x8000","label":"start","verdict":"required","must_precede":["0x8002"]},
                {"address":"0x8002","label":"finish","verdict":"allowed"}]}
        instructions = [{"cpu_address_candidate":"0x8000"},{"cpu_address_candidate":"0x8002"}]
        trace = {"events":[{"pc":"0x8000"},{"pc":"0x8002"}]}
        report = nes_advanced.evaluate_rules(rules, instructions, trace)
        self.assertEqual(report["summary"]["conflicts"], 0)
        self.assertIn("consistent-with-declared-rule", [x["status"] for x in report["checks"]])

    def test_rules_order_conflict(self):
        rules = {"schema_version":"1.0","title":"Test","objective":"","constraints":[],
            "actions":[{"address":"0x8000","verdict":"allowed","must_precede":["0x8002"]}]}
        trace = {"events":[{"pc":"0x8002"},{"pc":"0x8000"}]}
        report = nes_advanced.evaluate_rules(rules, [{"cpu_address_candidate":"0x8000"}], trace)
        self.assertIn("conflicts-with-declared-rule", [x["status"] for x in report["checks"]])

    def test_forbidden_action_observed_is_conflict(self):
        rules = {"schema_version":"1.0","title":"Test","objective":"","constraints":[],
            "actions":[{"address":"0x8000","label":"bad route","verdict":"forbidden"}]}
        trace = {"events":[{"pc":"0x8000"}]}
        report = nes_advanced.evaluate_rules(rules, [{"cpu_address_candidate":"0x8000"}], trace)
        self.assertEqual(report["checks"][0]["status"], "conflicts-with-declared-rule")

    def test_state_assertion_uses_last_observed_write(self):
        rules = {"schema_version":"1.0","title":"Test","objective":"","constraints":[],
            "actions":[],"state_assertions":[{"address":"0x0002","operator":"equals","value":7,"label":"lives"}]}
        trace = {"events":[{"address":"0x0002","value":4},{"address":"0x0002","value":7}]}
        report = nes_advanced.evaluate_rules(rules, [], trace)
        state_check = next(x for x in report["checks"] if x.get("criterion") == "state assertion")
        self.assertEqual(state_check["status"], "consistent-with-declared-rule")

    def test_empty_rules_template_has_no_fake_actions(self):
        self.assertEqual(nes_advanced.rules_template("Demo")["actions"], [])

    def test_trace_alias_normalization_and_replay(self):
        result = nes_advanced.normalize_trace_events({"adapter":"test","events":[
            {"PC":"0x8000","frame_number":"2","buttons":"A"}]})
        self.assertEqual(result["events"][0]["pc"], 0x8000)
        self.assertEqual(result["events"][0]["frame"], 2)
        self.assertTrue(result["input_replay"]["supported"])
        self.assertFalse(result["stepping"]["supported"])

    def test_mapper_trace_tracks_uxrom_bank_write(self):
        state = nes_advanced.mapper_trace_state(2, 65536, 8192, [
            {"event_index": 3, "address": "0x8000", "value": "0x02", "pc": "0x8123"}
        ])
        self.assertEqual(state["registers"]["selected_prg_bank"], 2)
        self.assertEqual(state["observed_write_count"], 1)

    def test_palette_scene_requires_snapshot(self):
        self.assertFalse(nes_advanced.palette_scene_snapshot([], [], [])["supported"])
        scene = nes_advanced.palette_scene_snapshot([1]*960, [0]*64, list(range(32)))
        self.assertTrue(scene["supported"])
        self.assertEqual((scene["width"], scene["height"]), (32,30))

if __name__ == "__main__":
    unittest.main()
