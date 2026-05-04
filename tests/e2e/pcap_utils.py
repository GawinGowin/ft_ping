"""scapy-based helpers for inspecting captured ICMP traffic."""
from __future__ import annotations

from pathlib import Path
from typing import Any

# scapy is intentionally imported lazily so that the rest of the test
# module can load even if scapy isn't installed (gives clearer errors).
def _import_scapy():
    try:
        from scapy.all import rdpcap, IP, ICMP  # type: ignore
    except ImportError as e:  # pragma: no cover
        raise ImportError(
            "scapy is required for pcap parsing. "
            "Install with: pip install -r tests/e2e/requirements.txt"
        ) from e
    return rdpcap, IP, ICMP


ICMP_ECHO_REQUEST = 8
ICMP_ECHO_REPLY = 0


def read_icmp_packets(pcap_path: str | Path) -> list[dict[str, Any]]:
    """Return one dict per ICMP packet in the pcap, ordered by capture time."""
    rdpcap, IP, ICMP = _import_scapy()
    packets = rdpcap(str(pcap_path))
    out: list[dict[str, Any]] = []
    for p in packets:
        if IP not in p or ICMP not in p:
            continue
        ip = p[IP]
        icmp = p[ICMP]
        out.append({
            "type": int(icmp.type),
            "code": int(icmp.code),
            "id": int(getattr(icmp, "id", 0) or 0),
            "seq": int(getattr(icmp, "seq", 0) or 0),
            "ttl": int(ip.ttl),
            "tos": int(ip.tos),
            "src": str(ip.src),
            "dst": str(ip.dst),
            "payload_len": len(bytes(icmp.payload)),
        })
    return out


def echo_requests(packets: list[dict[str, Any]]) -> list[dict[str, Any]]:
    return [p for p in packets if p["type"] == ICMP_ECHO_REQUEST]


def echo_replies(packets: list[dict[str, Any]]) -> list[dict[str, Any]]:
    return [p for p in packets if p["type"] == ICMP_ECHO_REPLY]


def summarize(packets: list[dict[str, Any]]) -> dict[str, Any]:
    """Compact view of a packet stream: counts + first-packet field summary."""
    reqs = echo_requests(packets)
    reps = echo_replies(packets)
    head = reqs[0] if reqs else None
    return {
        "n_requests": len(reqs),
        "n_replies": len(reps),
        "ttl": head["ttl"] if head else None,
        "tos": head["tos"] if head else None,
        "payload_len": head["payload_len"] if head else None,
        "id": head["id"] if head else None,
    }
