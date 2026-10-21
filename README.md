# QNOS-QVM 0.1 — Quantum Native OS Research Virtual Machine

**Standalone research project** —

This first release is a portable **C++17 quantum-state simulator and quantum resource/job runtime**. It does not boot physical qubits, does not implement a self-hosted quantum OS, and does not run OpenQASM directly. Instead it executes the explicitly specified **QNOS-QISA v0.1** subset through a statevector engine. Its FIFO job runtime runs on a conventional macOS/Linux host.

## Build on macOS

Install Xcode Command Line Tools (`xcode-select --install`) and CMake (`brew install cmake`) if needed.

```sh
cd QNOS-QVM-0.1
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/qnos-qvm run examples/bell.qisa --shots 1024 --seed 42 --verify bell
./build/qnos-qvm shell
```

## Build on Ubuntu/Debian Linux

```sh
sudo apt update
sudo apt install -y build-essential cmake
cd QNOS-QVM-0.1
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
./build/qnos-qvm run examples/ghz5.qisa --shots 1024 --seed 42 --verify ghz
```

## Use the interactive shell

```text
qnos$ info
qnos$ load examples/bell.qisa
qnos$ submit 256
qnos$ jobs
qnos$ dispatch
qnos$ result 1
qnos$ noise 0.02 0.05
qnos$ submit 256
qnos$ dispatch
qnos$ result 2
qnos$ exit
```

`submit` queues a job and returns immediately; `dispatch` executes **one** pending job synchronously. This is a deterministic FIFO simulator, **not yet** true parallel asynchronous scheduling, a hardware IRQ runtime, or preemptive process isolation.

## Included QISA examples

| Source | Description |
|---|---|
| `examples/bell.qisa` | H + controlled-X gives Z-basis 00/11 correlations |
| `examples/ghz3.qisa` | 3-qubit GHZ state |
| `examples/ghz4.qisa` | 4-qubit GHZ state |
| `examples/ghz5.qisa` | 5-qubit GHZ state |
| `examples/hh.qisa` | H followed by H = identity |
| `examples/dj_constant.qisa` | Single-input constant Deutsch–Jozsa phase oracle |
| `examples/dj_balanced.qisa` | Single-input balanced Deutsch–Jozsa phase oracle |

## QNOS-QISA v0.1

Every instruction is one line, whitespace or comma-separated; `;` and `#` start comments.

| Instruction | Function |
|---|---|
| `QALLOC qN` | Claim qubit register; double allocation is rejected |
| `QRELEASE qN` | Release register ownership (simulation metadata; not a physical reset) |
| `QRESET qN` | Computational-basis reset through measurement and conditional X |
| `QH qN` | Hadamard |
| `QX qN` | Pauli X |
| `QZ qN` | Pauli Z |
| `QS qN` | Phase S |
| `QCX qC qT` | Controlled X; control and target must differ |
| `QMEASURE qN cN` | Projective Z measurement and classical register write |
| `QEND` | End program |

Register indexes are 0–11; the maximum allocated statevector is 2^12 complex amplitudes. QNOS-QISA is **not** RISC-V or standard OpenQASM and has no conditional branch/teleportation support in this release. Classical `cN` readout follows little-endian bit numbering. Stochastic gate noise is a deliberately simplified per-gate X insertion and stochastic measurement noise flips the reported classical bit; it is not a calibrated physical decoherence model.

## Architecture

```text
Host terminal (macOS/Linux)
         |
 QNOS-QVM CLI / shell
         |
 QNOS-QISA parser / validation
         |
 Quantum job queue (FIFO) / job IDs
         |
 Quantum register ownership and lifecycle
         |
 C++ statevector quantum engine (up to 12 qubits)
         |
 Measurement histogram + seeded noise injection
```

## Test claims and limitations

Six CTest integration checks are included: Bell, GHZ-5, H-H identity, Deutsch–Jozsa constant, Deutsch–Jozsa balanced, and invalid double allocation. These test the host simulator and resource model; they cannot demonstrate physical entanglement or hardware boot. Future releases should add property-based randomized circuits, independent verification against Aer/QuEST, detailed lifecycle tests, quantum feedback and a physical-backend specification.

## Future QNOS milestones

- 0.2: process ownership, isolation, cross-process qubit allocation, stronger tests.
- 0.3: scheduler events, virtual clock, cancellation/timeouts, measurement feedback.
- 0.4: noise channels, density-matrix reference, quantum error-correction simulation.
- 0.5: native gate timing/topology and device-controller backend specification.
- 1.0: integrated research demonstrator; physical hardware remains an independently validated milestone.
