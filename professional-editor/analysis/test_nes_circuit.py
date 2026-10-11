import importlib.util, unittest
from pathlib import Path
HERE=Path(__file__).parent
spec=importlib.util.spec_from_file_location("nes_circuit",HERE/"nes_circuit.py")
circuit=importlib.util.module_from_spec(spec)
import sys
sys.modules[spec.name]=circuit
spec.loader.exec_module(circuit)

class CircuitTests(unittest.TestCase):
    def test_order_branch_loop_and_action_assessment(self):
        ins=[
            {"cpu_address_candidate":"0x8000","file_offset":16,"mnemonic":"LDA","operand":"#0x01","bytes":[169,1],"size":2,"bank":"test"},
            {"cpu_address_candidate":"0x8002","file_offset":18,"mnemonic":"STA","operand":"0x0002","bytes":[133,2],"size":2,"bank":"test"},
            {"cpu_address_candidate":"0x8004","file_offset":20,"mnemonic":"CMP","operand":"#0x03","bytes":[201,3],"size":2,"bank":"test"},
            {"cpu_address_candidate":"0x8006","file_offset":22,"mnemonic":"BNE","operand":"0x8002","target_cpu_address":"0x8002","flow":"conditional_branch","bytes":[208,250],"size":2,"bank":"test"},
            {"cpu_address_candidate":"0x8008","file_offset":24,"mnemonic":"RTS","operand":"","flow":"return","bytes":[96],"size":1,"bank":"test"},
        ]
        result=circuit.analyze_circuit(ins)
        self.assertGreaterEqual(result["edge_count"],4)
        self.assertTrue(result["loops"])
        self.assertTrue(any(x["type"]=="repeat-or-loop" for x in result["loops"]))
        self.assertTrue(any(x["instruction"]=="STA" for x in result["action_assessments"]))
        self.assertEqual(result["action_assessments"][0]["right_action_assessment"]["status"],"undetermined")
        self.assertIn("digraph NES_Circuit",circuit.to_dot(result))

    def test_repeated_instruction_run_is_separate_from_loop(self):
        result=circuit.analyze_circuit([
            {"cpu_address_candidate":"0x8000","mnemonic":"NOP","operand":""},
            {"cpu_address_candidate":"0x8001","mnemonic":"NOP","operand":""},
            {"cpu_address_candidate":"0x8002","mnemonic":"RTS","operand":""}
        ])
        self.assertTrue(result["repeated_instruction_runs"])

if __name__=="__main__": unittest.main()
