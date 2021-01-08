import datetime


class Stopwatch:
    _MILLISECONDS_PER_SECOND = 1000

    def __init__(self):
        self._duration = None
        self._reset()

    def stop(self):
        if self._duration is None:
            end = datetime.datetime.now()
            delta = end - self.start
            # only return whole int number, don't need micro seconds
            self._duration = int(delta.total_seconds() * Stopwatch._MILLISECONDS_PER_SECOND)

        return self._duration

    def get(self):
        end = datetime.datetime.now()
        delta = end - self.start
        # only return whole int number, don't need micro seconds
        return int(delta.total_seconds() * Stopwatch._MILLISECONDS_PER_SECOND)
        
    def restart(self):
        ms = self.stop()
        self._reset()
        return ms

    def _reset(self):
        self.start = datetime.datetime.now()
        self._duration = None
