# System Architecture – FireBarrier (SIH25160)

FireBarrier is a cross-platform AI-Driven Next-Generation Firewall prototype.  
It consists of three major components connected through secure asynchronous messaging (NNG).

---

## 1. C++ Daemon (Background Service)

The daemon runs continuously in the background and performs:

- High-speed packet capture using Npcap (Windows) or libpcap (Linux).
- Conversion of packets into *flows* (aggregated bi-directional sessions).
- Periodic system-health pings.
- Communication with the Python AI engine through NNG.
- Enforcing Zero-Trust decisions (e.g., block, restrict, alert).

The daemon runs even when the UI is closed.

---

## 2. Python FastAPI ML & Policy Engine

The Python server performs:

- Flow feature extraction (entropy, timing patterns, TLS metadata).
- ML-based threat classification.
- Zero-Trust risk scoring and decision generation.
- Federated learning support (model update push / submission).
- Response back to the daemon using NNG.

The server runs locally on the machine (localhost).

---

## 3. C++ Qt UI (Frontend Dashboard)

The Qt application:

- Starts/stops the daemon and ML server.
- Displays live threat reports, flow logs, system stats.
- Offers user control (Start Monitoring / Stop Monitoring).
- Reads data only from the daemon.
- Can be closed at any time without affecting the daemon.

---

## 4. Communication Layer (NNG)

NNG provides:

- PUB/SUB for streaming flow data
- REQ/REP for synchronous requests (e.g., pings)
- PUSH/PULL for ML results

It is fully embedded, cross-platform, and requires no external services.

---

## High-Level Flow

1. **Daemon captures packets → builds flow → sends JSON to ML engine**
2. **ML engine analyzes → sends back threat score & decision**
3. **Daemon enforces decision + forwards summary to UI**
4. **UI displays visual insights only when opened**

---

## Shared JSON Protocol

All communication uses structured JSON schemas located in `/shared_protocol/`:

- `request_format.json`
- `response_format.json`
- `daemon_ping_format.json`
- `server_ack_format.json`
- `error_event.json`
- `model_update_push_format.json`
- `model_update_submission_format.json`

These schemas guarantee consistent messaging across all 3 components.

---
