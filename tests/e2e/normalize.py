"""Normalize ping(8)-style output for structural comparison.

The goal is NOT to match real ping byte-for-byte (locale/version
dependent), but to mask values that legitimately vary between runs
(RTTs, total time, mdev) so that the *structure* can be compared.
"""
from __future__ import annotations

import re

# `time=0.123 ms` / `time<0.1ms` / `time=1 ms` etc.
_RE_TIME_FIELD = re.compile(r"time[=<]\s*[\d.]+\s*ms")
# trailing total-time in stats line: `time 2042ms`
_RE_TOTAL_TIME = re.compile(r"time\s+\d+\s*ms")
# `rtt min/avg/max/mdev = 0.038/0.041/0.045/0.003 ms`
# also matches the `round-trip` variant some pings emit.
_RE_RTT_SUMMARY = re.compile(
    r"(rtt|round-trip)\s+min/avg/max(/mdev|/stddev)?\s*=\s*[\d./]+\s*ms",
    re.IGNORECASE,
)
# host-name + parenthesized IP duplication: `localhost (127.0.0.1)` -> `127.0.0.1`
_RE_HOST_IP = re.compile(r"\b\w[\w.-]*\s+\((\d+\.\d+\.\d+\.\d+)\)")


def normalize_ping_output(text: str) -> str:
    """Mask volatile fields so two ping outputs can be compared structurally."""
    out = text
    out = _RE_TIME_FIELD.sub("time=<RTT>", out)
    out = _RE_TOTAL_TIME.sub("time=<TIME>", out)
    out = _RE_RTT_SUMMARY.sub("rtt=<STATS>", out)
    out = _RE_HOST_IP.sub(r"\1", out)
    return out


def extract_summary(text: str) -> dict[str, int | float | None]:
    """Pull the numeric parts of the trailing stats block."""
    m = re.search(
        r"(\d+)\s+packets transmitted,\s+(\d+)\s+received"
        r"(?:,\s+(\d+)\s+errors)?"
        r",\s+([\d.]+)%\s+packet loss",
        text,
    )
    if not m:
        return {}
    return {
        "transmitted": int(m.group(1)),
        "received": int(m.group(2)),
        "errors": int(m.group(3)) if m.group(3) else 0,
        "loss_pct": float(m.group(4)),
    }
