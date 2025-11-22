from threading import Lock


class Stats:
    def __init__(self):
        self._lock = Lock()
        self.processed_flows = 0
        self.aggregated_models = 0

    def incr_flows(self, n: int = 1):
        with self._lock:
            self.processed_flows += n

    def incr_aggregated(self, n: int = 1):
        with self._lock:
            self.aggregated_models += n


app_stats = Stats()
