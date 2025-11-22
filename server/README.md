# FireBarrier ML & Policy Engine (Prototype)

This folder contains a lightweight FastAPI prototype for the ML & policy engine used by the FireBarrier NGFW project.

Quick start (Windows PowerShell):

1. Create and activate a Python environment (recommended):

```powershell
python -m venv .venv; .\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

2. Run the server:

```powershell
.\run_server.bat
```

Endpoints implemented (prototype):
- `POST /api/v1/analyze_flow` : Flow analysis request (JSON matching `shared_protocol/request_format.json`).
- `POST /api/v1/ping` : Daemon ping (JSON matching `shared_protocol/daemon_ping_format.json`).
- `POST /api/v1/model/push` : Push a global model update (base64 payload inside JSON).
- `POST /api/v1/model/submit` : Submit a local model update for federated aggregation.
- `GET  /health` : Simple health check.

Notes:
- The inference in `app/main.py` is a small heuristic placeholder. Replace `simple_inference` with your trained model inference (Torch/ONNX) for production.
- The server will attempt to start a background NNG REP listener if `pynng` is installed.
