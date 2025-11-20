FireBarrier – AI-Driven Next-Gen Firewall (SIH 2025)

Team Project – Smart India Hackathon 2025
Problem Statement: SIH25160

FireBarrier is a cross-platform, AI-powered Next-Generation Firewall designed for dynamic threat detection and Zero Trust enforcement.
It combines C++ packet-flow analysis, NNG-based high-speed IPC, and a Python FastAPI ML engine.

This repository contains three major components:

🚀 System Overview
Qt UI App  ⇄  C++ Daemon (Packet Capture, Flow Builder)
               ⇄  NNG Messaging  ⇄
                      Python FastAPI ML Engine


FireBarrier performs:

Real-time packet capture

Flow-based feature extraction

ML-driven threat scoring

Zero-Trust policy decisions

Federated model update support

Background daemon service that keeps running even if UI is closed

📁 Repository Structure
project_root/
│
├── ui_app/           # Qt UI (C++, user-facing)
├── cpp_daemon/       # Background service (C++, packet capture & NNG)
├── python_server/    # FastAPI ML engine + NNG listener
│
├── shared_protocol/  # JSON communication formats
├── external/         # nng, libpcap/npcap, dependencies
├── deployment/       # Packaging scripts & runtime Python env
└── docs/             # Documentation for developers

📡 Communication Protocol (NNG)

Message exchange uses JSON schemas from shared_protocol/:

flow_analysis_request.json – Daemon → Python

flow_analysis_response.json – Python → Daemon

error_event.json

daemon_status_ping.json

server_ack.json

model_update_push.json

model_update_submission.json

These documents define strict, version-controlled data exchange.

⚙️ Components
1. Qt UI Application (C++)

Start/Stop monitoring

Live threat feed

System health display

Reads daemon status via NNG

Runs without showing a console window

2. C++ Daemon

Runs continuously in background

Captures packets using WinPcap/Npcap / libpcap

Converts packets into flows

Extracts statistical features

Sends flow JSON to Python ML engine

Receives threat decisions

Applies Zero-Trust rules

Logs events & exposes status

3. Python FastAPI Server

Receives flow data

Loads ML model (PyTorch)

Runs inference

Computes threat score + risk score

Generates zero-trust policy decision

Can send/receive federated model updates

Sends structured response back via nng

🛠 Tech Stack
Component	Technology
UI	Qt 6 + C++
Daemon	C++20, libpcap/Npcap, nng
ML Server	Python 3.10+, FastAPI, PyTorch
Packaging	Python venv bundling, batch/shell startup scripts
IPC	nng (Nanomsg Next Gen)
🎯 Current Status

Architecture finalized

JSON protocol published

Project skeleton created

Daemon module implementation in progress

Python ML service scaffolding ready

UI integration pending

📌 How to Build (Temporary)
Build Daemon + UI

Requires:

MSVC (recommended)

CMake

Qt6

NPCAP (Windows) or libpcap (Linux)

nng (built once into /external/)

cd cpp_daemon
mkdir build && cd build
cmake ..
cmake --build .

Run Python Server
cd python_server
./python_runtime/Scripts/python.exe -m uvicorn app.main:app --reload