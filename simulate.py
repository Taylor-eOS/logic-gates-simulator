import itertools
import time

class Gate:
    def __init__(self, gate_type, inputs):
        self.gate_type = gate_type
        self.inputs = tuple(sorted(inputs) if gate_type in ['AND', 'OR', 'XOR'] else inputs)
    def evaluate(self, signals):
        vals = [signals[i] for i in self.inputs]
        if self.gate_type == 'NOT':
            return 1 - vals[0]
        if self.gate_type == 'AND':
            for v in vals:
                if v == 0:
                    return 0
            return 1
        if self.gate_type == 'OR':
            for v in vals:
                if v == 1:
                    return 1
            return 0
        if self.gate_type == 'XOR':
            s = 0
            for v in vals:
                s ^= v
            return s
    def __eq__(self, other):
        return isinstance(other, Gate) and self.gate_type == other.gate_type and self.inputs == other.inputs
    def __hash__(self):
        return hash((self.gate_type, self.inputs))

def make_expected_triplets():
    expected = []
    for a in range(4):
        for b in range(4):
            s = a + b
            sum1 = (s >> 1) & 1
            sum0 = s & 1
            carry = (s >> 2) & 1
            expected.append([sum1, sum0, carry])
    return expected

def test_adder_function(circuit_func):
    expected = make_expected_triplets()
    idx = 0
    for a in range(4):
        a1, a0 = (a >> 1) & 1, a & 1
        for b in range(4):
            b1, b0 = (b >> 1) & 1, b & 1
            inputs = [a1, a0, b1, b0]
            if circuit_func(inputs) != expected[idx]:
                return False
            idx += 1
    return True

def simulate_circuit(input_bits, gates, outputs):
    signals = list(input_bits)
    for gate in gates:
        signals.append(gate.evaluate(signals))
    return [signals[i] for i in outputs]

def _compute_input_masks():
    masks = [0, 0, 0, 0]
    idx = 0
    for a in range(4):
        a1 = (a >> 1) & 1
        a0 = a & 1
        for b in range(4):
            b1 = (b >> 1) & 1
            b0 = b & 1
            if a1: masks[0] |= (1 << idx)
            if a0: masks[1] |= (1 << idx)
            if b1: masks[2] |= (1 << idx)
            if b0: masks[3] |= (1 << idx)
            idx += 1
    return masks

def _eval_gate_mask(gate, masks):
    if gate.gate_type == 'NOT':
        return (~masks[gate.inputs[0]]) & 0xFFFF
    if gate.gate_type == 'AND':
        if gate.inputs.__len__() == 2:
            return masks[gate.inputs[0]] & masks[gate.inputs[1]]
        return masks[gate.inputs[0]] & masks[gate.inputs[1]] & masks[gate.inputs[2]]
    if gate.gate_type == 'OR':
        if gate.inputs.__len__() == 2:
            return masks[gate.inputs[0]] | masks[gate.inputs[1]]
        return masks[gate.inputs[0]] | masks[gate.inputs[1]] | masks[gate.inputs[2]]
    if gate.gate_type == 'XOR':
        if gate.inputs.__len__() == 2:
            return masks[gate.inputs[0]] ^ masks[gate.inputs[1]]
        return masks[gate.inputs[0]] ^ masks[gate.inputs[1]] ^ masks[gate.inputs[2]]
    return 0

def generate_useful_gates(available_signals, allow_3input=True):
    gates = []
    for i in range(available_signals):
        gates.append(Gate('NOT', (i,)))
    for i in range(available_signals):
        for j in range(i + 1, available_signals):
            gates.append(Gate('AND', (i, j)))
            gates.append(Gate('OR', (i, j)))
            gates.append(Gate('XOR', (i, j)))
            if allow_3input:
                for k in range(j + 1, available_signals):
                    gates.append(Gate('AND', (i, j, k)))
                    gates.append(Gate('OR', (i, j, k)))
                    gates.append(Gate('XOR', (i, j, k)))
    return gates

def search_circuits():
    found_circuits = []
    tested_count = 0
    progress_interval = 10000000
    max_found = 3
    min_gates = 6
    max_gates = 10
    input_masks = _compute_input_masks()
    expected_masks = make_expected_triplets()
    expected_mask_vals = [0, 0, 0]
    for idx, trip in enumerate(make_expected_triplets()):
        if trip[0]: expected_mask_vals[0] |= (1 << idx)
        if trip[1]: expected_mask_vals[1] |= (1 << idx)
        if trip[2]: expected_mask_vals[2] |= (1 << idx)
    start_time = time.time()
    for num_gates in range(min_gates, max_gates):
        print(f"Testing {num_gates} gates...")
        base_signals = 4
        max_total = base_signals + num_gates
        masks = [0] * max_total
        for i in range(4):
            masks[i] = input_masks[i]
        current_gates = []
        useful_cache = {}
        def build_circuit(depth, total_signals):
            nonlocal tested_count, found_circuits, masks, current_gates
            if len(found_circuits) >= max_found:
                return
            if depth == num_gates:
                total_signals = 4 + len(current_gates)
                candidates = range(4, total_signals)
                for outs in itertools.combinations(candidates, 3):
                    tested_count += 1
                    if tested_count % progress_interval == 0:
                        elapsed = time.time() - start_time
                        print(f"  Tested {tested_count} configurations... elapsed {int(elapsed)}s")
                    if masks[outs[0]] == expected_mask_vals[0] and masks[outs[1]] == expected_mask_vals[1] and masks[outs[2]] == expected_mask_vals[2]:
                        found_circuits.append((list(current_gates), outs))
                        print(f"  Found circuit with {len(current_gates)} gates!")
                        if len(found_circuits) >= max_found:
                            return
                return
            available = 4 + len(current_gates)
            key = available
            if key in useful_cache:
                useful = useful_cache[key]
            else:
                useful = generate_useful_gates(available, allow_3input=True)
                useful_cache[key] = useful
            for gate in useful:
                if len(found_circuits) >= max_found:
                    return
                if gate in current_gates:
                    continue
                outmask = _eval_gate_mask(gate, masks)
                if outmask == 0 or outmask == 0xFFFF:
                    continue
                already = False
                for s in range(available):
                    if masks[s] == outmask:
                        already = True
                        break
                if already:
                    continue
                masks[available] = outmask
                current_gates.append(gate)
                build_circuit(depth + 1, total_signals + 1)
                current_gates.pop()
                masks[available] = 0
                if len(found_circuits) >= max_found:
                    return
        build_circuit(0, 4)
        if found_circuits:
            break
    print(f"Total configurations tested: {tested_count}")
    return found_circuits

def print_circuit(gates, outputs, circuit_num):
    print(f"\nCircuit {circuit_num} ({len(gates)} gates):")
    print("Inputs: A1(0), A0(1), B1(2), B0(3)")
    for i, gate in enumerate(gates):
        signal_num = 4 + i
        input_names = []
        for inp in gate.inputs:
            if inp < 4:
                names = ['A1', 'A0', 'B1', 'B0']
                input_names.append(names[inp])
            else:
                input_names.append(f"S{inp}")
        input_str = ",".join(input_names)
        print(f"  S{signal_num} = {gate.gate_type}({input_str})")
    print("Outputs:")
    output_names = ['Sum1', 'Sum0', 'Carry']
    for i, out_sig in enumerate(outputs):
        print(f"  {output_names[i]} = S{out_sig}")

def verify_circuit(gates, outputs):
    print("\nVerification:")
    print("A1A0 + B1B0 = Sum1Sum0 Carry")
    for a in range(4):
        for b in range(4):
            a1, a0 = (a >> 1) & 1, a & 1
            b1, b0 = (b >> 1) & 1, b & 1
            inputs = [a1, a0, b1, b0]
            result = simulate_circuit(inputs, gates, outputs)
            expected_sum = a + b
            actual_val = result[0] * 2 + result[1] + result[2] * 4
            status = "✓" if actual_val == expected_sum else "✗"
            print(f"  {a1}{a0} + {b1}{b0} = {result[0]}{result[1]} (C={result[2]}) | {a}+{b}={expected_sum} {status}")

if __name__ == "__main__":
    print("Optimized 2-bit adder circuit search")
    print("Finding minimal gate implementations...\n")
    circuits = search_circuits()
    if circuits:
        for i, (gates, outputs) in enumerate(circuits):
            print_circuit(gates, outputs, i + 1)
            if i == 0:
                verify_circuit(gates, outputs)
    else:
        print("No circuits found within search limits")

