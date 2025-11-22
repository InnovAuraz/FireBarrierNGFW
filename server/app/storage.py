import os
import base64
import logging
import time
from typing import Tuple
from .schemas import ModelUpdatePayload, ModelSubmission

LOG = logging.getLogger("firebarrier.storage")

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
MODELS_DIR = os.path.join(BASE_DIR, "models")
SUBMISSIONS_DIR = os.path.join(BASE_DIR, "submissions")
os.makedirs(MODELS_DIR, exist_ok=True)
os.makedirs(SUBMISSIONS_DIR, exist_ok=True)


def save_pushed_model(payload: ModelUpdatePayload) -> Tuple[bool, str]:
    try:
        data = payload.update.get("payload")
        if not data:
            raise ValueError("missing payload")
        binary = base64.b64decode(data.encode("utf-8"))
        fname = f"model_{payload.model_version}_r{payload.federated_round or 0}.bin"
        fpath = os.path.join(MODELS_DIR, fname)
        with open(fpath, "wb") as f:
            f.write(binary)
        LOG.info("Saved pushed model to %s", fpath)
        return True, fpath
    except Exception as e:
        LOG.exception("Failed to save model push: %s", e)
        return False, str(e)


def save_submission(sub: ModelSubmission) -> Tuple[bool, str]:
    try:
        payload_b64 = sub.update.get("payload")
        if not payload_b64:
            raise ValueError("missing update.payload")
        binary = base64.b64decode(payload_b64.encode("utf-8"))
        fname = f"submission_{sub.node_id}_{int(time.time())}.bin"
        fpath = os.path.join(SUBMISSIONS_DIR, fname)
        with open(fpath, "wb") as f:
            f.write(binary)
        LOG.info("Saved submission from %s to %s", sub.node_id, fpath)
        return True, fpath
    except Exception as e:
        LOG.exception("Failed to save submission: %s", e)
        return False, str(e)


def list_submissions():
    files = []
    for n in os.listdir(SUBMISSIONS_DIR):
        p = os.path.join(SUBMISSIONS_DIR, n)
        if os.path.isfile(p):
            files.append(p)
    return sorted(files)


def aggregate_submissions(output_name: str = None) -> Tuple[bool, str]:
    """Simple aggregation: concatenate all submission binaries into a single 'global' file.

    This is a placeholder for real federated aggregation logic (e.g., averaging weights).
    """
    subs = list_submissions()
    if not subs:
        return False, "no submissions"
    if output_name is None:
        output_name = f"global_model_{int(time.time())}.bin"
    out_path = os.path.join(MODELS_DIR, output_name)
    try:
        with open(out_path, "wb") as out:
            for p in subs:
                with open(p, "rb") as f:
                    out.write(f.read())
        LOG.info("Aggregated %d submissions into %s", len(subs), out_path)
        return True, out_path
    except Exception as e:
        LOG.exception("Aggregation failed: %s", e)
        return False, str(e)
