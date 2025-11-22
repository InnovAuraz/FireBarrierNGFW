"""Simple smoke-test that posts shared_protocol JSONs to the running server.

Run from the `server` directory after starting the server:

    python tests/smoke_test.py

It sends:
- request_format.json -> POST /api/v1/analyze_flow
- daemon_ping_format.json -> POST /api/v1/ping
- model_update_push_format.json -> POST /api/v1/model/push (inserts a small base64 payload)
- model_update_submission_format.json -> POST /api/v1/model/submit (inserts a small base64 payload)

"""
import json
import base64
import os
import requests

BASE_URL = os.environ.get("FB_BASE_URL", "http://127.0.0.1:8000")
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SHARED = os.path.join(ROOT, "shared_protocol")


def load_json(name):
    path = os.path.join(SHARED, name)
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def post(path, data):
    url = BASE_URL + path
    try:
        r = requests.post(url, json=data, timeout=5)
        print(f"POST {path} -> {r.status_code}")
        try:
            print(r.json())
        except Exception:
            print(r.text)
    except Exception as e:
        print(f"POST {path} failed: {e}")


def main():
    print("Smoke test to", BASE_URL)

    # 1) flow analysis
    req = load_json("request_format.json")
    post("/api/v1/analyze_flow", req)

    # 2) daemon ping
    ping = load_json("daemon_ping_format.json")
    post("/api/v1/ping", ping)

    # 3) model push (insert small base64 payload)
    push = load_json("model_update_push_format.json")
    push_update = push.get("update", {})
    push_update["payload"] = base64.b64encode(b"dummy-global-model").decode("utf-8")
    push["update"] = push_update
    post("/api/v1/model/push", push)

    # 4) model submission (insert small base64 payload)
    sub = load_json("model_update_submission_format.json")
    sub_update = sub.get("update", {})
    sub_update["payload"] = base64.b64encode(b"dummy-local-deltas").decode("utf-8")
    sub["update"] = sub_update
    post("/api/v1/model/submit", sub)


if __name__ == "__main__":
    main()
