# QNOS-QVM technical notes

This is the initial software laboratory for independent QNOS research, distinct from QMINIX/HQCOF.

**Host boundary:** QNOS-QVM executable is a C++17 userspace process running on a classical operating system. The virtual quantum machine abstracts virtual qubit ownership, compilation of a minimal human-readable QISA, quantum gate dispatch, measurement and FIFO scheduling.

**Mathematical representation:** An n-qubit pure state is stored in a complex-valued array of 2^n amplitudes. Index bit k represents qk. A Hadamard gate changes the pair of amplitudes with qk=0/1; controlled-X swaps target-paired amplitudes when the control bit equals one. Z-basis measurement samples a Born probability and projects the state. Independent shots begin from |0...0>.

**Qubit lifecycle:** QALLOC grants ownership, QRESET prepares |0>, operations require owned registers, QMEASURE produces a classical cN readout, QRELEASE clears the ownership record. QRELEASE is *not* a guarantee that a physical device has returned to |0>.

**Job lifecycle:** QUEUED -> RUNNING -> COMPLETE or ERROR. The shell uses an explicit dispatcher. Job IDs are monotonic within each process. There is no interprocess isolation, deadline scheduling or hardware interrupt in version 0.1.

**Noise:** Simplified stochastic Pauli-X insertion after gates and independent readout-bit flips. Correlated noise, leakage, thermal relaxation, coherence times and true quantum error correction are out of scope.

**Security and validation boundaries:** The QISA parser checks known instructions and operand counts, with dynamic ownership checks at runtime. This is an experimental interpreter, not a verified secure sandbox for untrusted code.
