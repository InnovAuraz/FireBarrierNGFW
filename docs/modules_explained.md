# Module Overview – FireBarrier

This document explains each module across the three components.

---

## 1. C++ Daemon Modules

### ✔ PacketCapture
Captures raw packets using Npcap/libpcap.  
Outputs basic metadata (src/dst IP, ports, sizes, timestamps).

### ✔ FlowBuilder
Aggregates packets into flows.  
Extracts statistics required for ML-based detection.

### ✔ NngServer / NngClient
Handles NNG sockets for:
- Sending flow analysis requests
- Receiving threat responses
- Sending daemon pings
- Receiving acknowledgements

### ✔ JsonProtocol
Loads and validates JSON messages defined in `/shared_protocol`.

### ✔ StateManager
Stores current monitoring state:
- Running / stopped
- Total flows processed
- Current model version

### ✔ SystemStats
Reads local machine usage (CPU, memory, uptime).

### ✔ DaemonController
Main control loop:
- Initializes subsystems
- Runs capture loop
- Dispatches flows to AI engine

---

## 2. Python Server Modules

### ✔ FastAPI App
Exposes:
- `/analyze_flow`
- `/ping`
- `/model_update`
- `/status`

### ✔ Threat Analyzer
Loads ML model (CNN or anomaly detection).  
Performs flow classification & Zero-Trust scoring.

### ✔ Feature Extractor
Transforms flow JSON into ML-ready tensors.

### ✔ Graph Engine
Optional module used for correlating flows into attack graphs.

### ✔ NNG Listener / Sender
Receives flows from daemon.  
Returns ML responses.

### ✔ Utils (logger, config)
Project-wide helpers.

---

## 3. Qt UI Modules

### ✔ AppWindow
Main window containing:
- Threat table
- Logs
- Start/Stop buttons

### ✔ ZmqClient / NngClient (UI)
Communicates with daemon:
- Requests current status
- Subscribes to new events

---

Each module is isolated and testable, making the codebase clean and team-friendly.

---
