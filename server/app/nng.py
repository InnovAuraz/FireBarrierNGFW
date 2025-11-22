import json
import time
import logging
import threading
from typing import Any, Optional

try:
    import pynng
    from pynng.exceptions import Closed
    PYNNG_AVAILABLE = True
except Exception:
    pynng = None
    Closed = Exception
    PYNNG_AVAILABLE = False

from .schemas import RequestFormat
from . import inference

LOG = logging.getLogger("firebarrier.nng")

# Globals to manage background thread and socket for clean shutdown
_nng_thread: Optional[threading.Thread] = None
_nng_stop_event = threading.Event()
_rep_socket: Optional[Any] = None


def nng_rep_listener(addr: str = "tcp://127.0.0.1:5555") -> None:
    global _rep_socket
    if not PYNNG_AVAILABLE:
        LOG.info("pynng not available; skipping NNG REP listener")
        return
    try:
        LOG.info("Starting NNG REP listener on %s", addr)
        with pynng.Rep0(listen=addr) as rep:
            _rep_socket = rep
            while not _nng_stop_event.is_set():
                try:
                    msg = rep.recv_msg()
                    text = msg.bytes.decode("utf-8")
                    LOG.info("NNG REP received: %s", text)
                    obj = json.loads(text)
                    if obj.get("type") == "flow_analysis_request":
                        try:
                            req = RequestFormat(**obj)
                            result = inference.simple_inference(req.flow)
                            resp = {
                                "version": "1.0",
                                "type": "flow_analysis_response",
                                "timestamp": int(time.time()),
                                "flow_id": req.flow.flow_id,
                                **{k: result[k] for k in ["classification", "threat_score", "risk_score", "zero_trust_decision", "category", "confidence"]},
                                "explanation": result["explanation"],
                            }
                            rep.send_msg(json.dumps(resp).encode("utf-8"))
                        except Exception:
                            LOG.exception("Failed handling flow request via NNG")
                            err = {"version": "1.0", "type": "error", "timestamp": int(time.time()), "error": {"code": "INFERENCE_FAILED", "message": "internal"}}
                            try:
                                rep.send_msg(json.dumps(err).encode("utf-8"))
                            except Exception:
                                LOG.exception("Failed sending error reply over NNG")
                    elif obj.get("type") == "daemon_status_ping":
                        ack = {"version": "1.0", "type": "ack", "timestamp": int(time.time()), "message": "received"}
                        rep.send_msg(json.dumps(ack).encode("utf-8"))
                    else:
                        rep.send_msg(json.dumps({"version": "1.0", "type": "ack", "timestamp": int(time.time()), "message": "unknown-type"}).encode("utf-8"))
                except Closed:
                    LOG.info("NNG socket closed, exiting listener loop")
                    break
                except Exception:
                    LOG.exception("Error in NNG REP loop")
                    time.sleep(0.5)
    except Exception:
        LOG.exception("NNG REP listener couldn't start")
    finally:
        _rep_socket = None


def start_background_nng():
    global _nng_thread, _nng_stop_event
    if not PYNNG_AVAILABLE:
        LOG.info("Skipping NNG background threads (pynng missing)")
        return
    if _nng_thread and _nng_thread.is_alive():
        LOG.info("NNG background thread already running")
        return
    _nng_stop_event.clear()
    _nng_thread = threading.Thread(target=nng_rep_listener, args=("tcp://127.0.0.1:5555",), daemon=True)
    _nng_thread.start()


def stop_background_nng(timeout: float = 2.0) -> None:
    """Signal the background listener to stop and close the socket to unblock recv."""
    global _nng_thread, _nng_stop_event, _rep_socket
    if not PYNNG_AVAILABLE:
        return
    _nng_stop_event.set()
    try:
        if _rep_socket is not None:
            try:
                _rep_socket.close()
            except Exception:
                LOG.exception("Error closing NNG socket during shutdown")
    finally:
        if _nng_thread is not None:
            _nng_thread.join(timeout)
            if _nng_thread.is_alive():
                LOG.warning("NNG background thread did not stop within timeout")

