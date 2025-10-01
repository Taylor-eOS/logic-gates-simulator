import itertools

class Gate:
    def __init__(self, gate_type, inputs):
        self.gate_type = gate_type
        self.inputs = tuple(sorted(inputs) if gate_type in ['AND', 'OR', 'XOR'] else inputs)

    def evaluate(self, signals):
        vals = [signals[i] for i in self.inputs]
        if self.gate_type == 'NOT':
            return 1 - vals[0]
        elif self.gate_type == 'AND':
            return int(all(vals))
        elif self.gate_type == 'OR':
            return int(any(vals))
        elif self.gate_type == 'XOR':
            return vals[0] ^ vals[1] if len(vals) == 2 else vals[0] ^ vals[1] ^ vals[2]

    def __eq__(self, other):
        return self.gate_type == other.gate_type and self.inputs == other.inputs

    def __hash__(self):
        return hash((self.gate_type, self.inputs))

def make_expected_triplets():
    expected = []
    for a in range(4):
        a1, a0 = (a >> 1) & 1, a & 1
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

def generate_useful_gates(available_signals):
    gates = set()
    for i in range(available_signals):
        gates.add(Gate('NOT', [i]))
    for i in range(available_signals):
        for j in range(i+1, available_signals):
            gates.add(Gate('AND', [i, j]))
            gates.add(Gate('OR', [i, j]))
            gates.add(Gate('XOR', [i, j]))
            for k in range(j+1, available_signals):
                gates.add(Gate('AND', [i, j, k]))
                gates.add(Gate('OR', [i, j, k]))
                gates.add(Gate('XOR', [i, j, k]))
    return list(gates)

def search_circuits():
    found_circuits = []
    tested_count = 0
    for num_gates in range(8, 10):
        print(f"Testing {num_gates} gates...")

        def build_circuit(current_gates, remaining_gates):
            nonlocal tested_count, found_circuits
            if len(found_circuits) >= 3:
                return
            if remaining_gates == 0:
                total_signals = 4 + len(current_gates)
                for outputs in itertools.combinations(range(4, total_signals), 3):
                    tested_count += 1
                    if tested_count % 1000000 == 0:
                        print(f"  Tested {tested_count} configurations...")
                    circuit_func = lambda inputs: simulate_circuit(inputs, current_gates, outputs)
                    if test_adder_function(circuit_func):
                        found_circuits.append((list(current_gates), outputs))
                        print(f"  Found circuit with {len(current_gates)} gates!")
                        return
                return
            available_signals = 4 + len(current_gates)
            useful_gates = generate_useful_gates(available_signals)
            for gate in useful_gates:
                if gate not in current_gates:
                    current_gates.append(gate)
                    build_circuit(current_gates, remaining_gates - 1)
                    current_gates.pop()
                    if len(found_circuits) >= 3:
                        return
        build_circuit([], num_gates)
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

class Gate:
    def __init__(self, gate_type, inputs):
        self.gate_type = gate_type
        self.inputs = tuple(sorted(inputs) if gate_type in ['AND', 'OR', 'XOR'] else inputs)

    def evaluate(self, signals):
        vals = [signals[i] for i in self.inputs]
        if self.gate_type == 'NOT':
            return 1 - vals[0]
        elif self.gate_type == 'AND':
            return int(all(vals))
        elif self.gate_type == 'OR':
            return int(any(vals))
        elif self.gate_type == 'XOR':
            return sum(vals) % 2

    def __eq__(self, other):
        return self.gate_type == other.gate_type and self.inputs == other.inputs

    def __hash__(self):
        return hash((self.gate_type, self.inputs))

def simulate_circuit(input_bits, gates, outputs):
    signals = list(input_bits)
    for gate in gates:
        signals.append(gate.evaluate(signals))
    return [signals[i] for i in outputs]

def generate_useful_gates(available_signals):
    gates = set()
    for i in range(available_signals):
        gates.add(Gate('NOT', [i]))
    for i in range(available_signals):
        for j in range(i+1, available_signals):
            gates.add(Gate('AND', [i, j]))
            gates.add(Gate('OR', [i, j]))
            gates.add(Gate('XOR', [i, j]))
    return list(gates)

def search_circuits():
    found_circuits = []
    tested_count = 0
    progress_interval = 1000000
    max_found = 3
    min_gates = 6
    max_gates = 10
    for num_gates in range(min_gates, max_gates):
        print(f"Testing {num_gates} gates...")

        def build_circuit(current_gates, remaining_gates):
            nonlocal tested_count, found_circuits
            if len(found_circuits) >= max_found:
                return
            if remaining_gates == 0:
                total_signals = 4 + len(current_gates)
                for outputs in itertools.combinations(range(4, total_signals), max_found - 1):
                    tested_count += 1
                    if tested_count % progress_interval == 0:
                        print(f"  Tested {tested_count} configurations...")
                    circuit_func = lambda inputs: simulate_circuit(inputs, current_gates, outputs)
                    if test_adder_function(circuit_func):
                        found_circuits.append((list(current_gates), outputs))
                        print(f"  Found circuit with {len(current_gates)} gates!")
                        return
                return
            available_signals = 4 + len(current_gates)
            useful_gates = generate_useful_gates(available_signals)
            for gate in useful_gates:
                if gate not in current_gates:
                    current_gates.append(gate)
                    build_circuit(current_gates, remaining_gates - 1)
                    current_gates.pop()
                    if len(found_circuits) >= max_found:
                        return
        build_circuit([], num_gates)
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

