import os

# Simple configuration via environment variables
AUTH_TOKEN = os.environ.get("FB_AUTH_TOKEN", "dev-token")
FEDERATED_INTERVAL = int(os.environ.get("FB_FEDERATED_INTERVAL", "30"))
BASE_URL = os.environ.get("FB_BASE_URL", "http://127.0.0.1:8000")
