import threading
import logging
import time
from typing import Optional

from . import storage
from . import config
from . import stats

LOG = logging.getLogger("firebarrier.federated")

_fed_thread: Optional[threading.Thread] = None
_fed_stop = threading.Event()


def _aggregator_loop(interval: int):
    LOG.info("Federated aggregator started with interval %ds", interval)
    while not _fed_stop.is_set():
        try:
            ok, info = storage.aggregate_submissions()
            if ok:
                LOG.info("Aggregator produced: %s", info)
                try:
                    stats.app_stats.incr_aggregated()
                except Exception:
                    pass
                # In a real system we'd notify other components or push via NNG
            else:
                LOG.debug("Aggregator: %s", info)
        except Exception:
            LOG.exception("Federated aggregator error")
        _fed_stop.wait(interval)
    LOG.info("Federated aggregator stopping")


def start_aggregator():
    global _fed_thread, _fed_stop
    if _fed_thread and _fed_thread.is_alive():
        LOG.info("Federated aggregator already running")
        return
    _fed_stop.clear()
    _fed_thread = threading.Thread(target=_aggregator_loop, args=(config.FEDERATED_INTERVAL,), daemon=True)
    _fed_thread.start()


def stop_aggregator(timeout: float = 2.0):
    global _fed_thread, _fed_stop
    _fed_stop.set()
    if _fed_thread is not None:
        _fed_thread.join(timeout)
        if _fed_thread.is_alive():
            LOG.warning("Federated aggregator did not stop within timeout")
