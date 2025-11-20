# Installer Guide – FireBarrier

This guide explains how end users install and run the FireBarrier system.

---

## 1. Included in the Installer

The installer bundles:

### ✔ daemon/
Prebuilt C++ daemon executable.

### ✔ ui_app/
Qt application (FireBarrier Dashboard).

### ✔ python_runtime/
A complete portable Python environment:
- python.exe
- Lib/
- Scripts/
- site-packages/ with all dependencies

### ✔ config/
Default configuration:
- daemon_config.json
- server_config.json

### ✔ startup scripts
- `start_daemon.bat`
- `start_server.bat`
- `stop_all.bat`

The user **does not need to install Python or pip**.

---

## 2. Dependencies Included

- Npcap redistributable installer (Windows only)
- NNG static library
- PyTorch CPU-only wheel inside `python_runtime/`

---

## 3. Installation Steps (End User)

1. Run `FireBarrierSetup.exe`.
2. Installer:
   - Copies all files
   - Installs Npcap silently (if missing)
   - Registers daemon service (optional mode)

3. User opens **FireBarrier** (Qt app).

---

## 4. How Monitoring Works

- The Qt UI starts the daemon using `start_daemon.bat`.
- Daemon starts ML server using `start_server.bat`.
- Both run invisibly in background.
- UI can be closed anytime; background monitoring continues.

---

## 5. Uninstall

Running `Uninstall.exe` removes:
- UI app
- Daemon
- Python runtime
- Logs
- Config files

Npcap is not automatically removed.

---

This installer design ensures **zero technical skill required** for end users.

---
