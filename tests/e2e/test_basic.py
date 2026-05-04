"""Basic E2E tests comparing ft_ping with the system ping(8).

These tests run inside an isolated network namespace and target
127.0.0.1, so they avoid touching the host network.
"""
from __future__ import annotations

import pytest

from normalize import extract_summary, normalize_ping_output
from pcap_utils import echo_requests, echo_replies, read_icmp_packets, summarize


@pytest.mark.parametrize("count", [1, 3, 5])
def test_count_sends_n_echo_requests(
    run_in_netns, capture_pcap, pcap_dir,
    ft_ping_bin, system_ping_bin, count,
):
    """`-c N` must put exactly N echo requests on the wire (both tools)."""
    for label, binary in [("system", system_ping_bin), ("ftping", ft_ping_bin)]:
        pcap = pcap_dir / f"{label}.pcap"
        with capture_pcap(pcap):
            res = run_in_netns([binary, "-c", str(count), "127.0.0.1"])
        assert res.returncode == 0, (
            f"{label} ping failed (rc={res.returncode}): {res.stderr}"
        )
        reqs = echo_requests(read_icmp_packets(pcap))
        assert len(reqs) == count, (
            f"{label}: expected {count} echo requests, got {len(reqs)}"
        )


def test_output_summary_agrees(run_in_netns, ft_ping_bin, system_ping_bin):
    """Stats footer must report the same transmitted/received/loss%."""
    sys_res = run_in_netns([system_ping_bin, "-c", "3", "127.0.0.1"])
    ft_res = run_in_netns([ft_ping_bin, "-c", "3", "127.0.0.1"])
    assert sys_res.returncode == 0
    assert ft_res.returncode == 0

    sys_summary = extract_summary(sys_res.stdout)
    ft_summary = extract_summary(ft_res.stdout)

    assert sys_summary, f"could not parse system ping summary:\n{sys_res.stdout}"
    assert ft_summary, f"could not parse ft_ping summary:\n{ft_res.stdout}"

    assert sys_summary["transmitted"] == ft_summary["transmitted"] == 3
    assert sys_summary["received"] == ft_summary["received"] == 3
    assert sys_summary["loss_pct"] == ft_summary["loss_pct"] == 0.0


def test_output_structure_shared(run_in_netns, ft_ping_bin, system_ping_bin):
    """Both outputs share: PING header, per-reply lines, stats footer, rtt line."""
    sys_lines = normalize_ping_output(
        run_in_netns([system_ping_bin, "-c", "3", "127.0.0.1"]).stdout
    ).splitlines()
    ft_lines = normalize_ping_output(
        run_in_netns([ft_ping_bin, "-c", "3", "127.0.0.1"]).stdout
    ).splitlines()

    for label, lines in [("system", sys_lines), ("ftping", ft_lines)]:
        assert lines, f"{label}: no output"
        assert lines[0].startswith("PING"), f"{label}: missing PING header: {lines[0]!r}"
        assert any("packets transmitted" in l for l in lines), (
            f"{label}: missing stats line"
        )
        assert any("rtt=<STATS>" in l for l in lines), (
            f"{label}: missing rtt summary"
        )
        # 3 reply lines (matching a normalized `time=<RTT>` token).
        reply_lines = [l for l in lines if "time=<RTT>" in l]
        assert len(reply_lines) == 3, (
            f"{label}: expected 3 reply lines, got {len(reply_lines)}"
        )


def test_default_payload_size_is_56(
    run_in_netns, capture_pcap, pcap_dir, ft_ping_bin, system_ping_bin,
):
    """Default ICMP data length is 56 bytes for both tools."""
    for label, binary in [("system", system_ping_bin), ("ftping", ft_ping_bin)]:
        pcap = pcap_dir / f"{label}.pcap"
        with capture_pcap(pcap):
            run_in_netns([binary, "-c", "1", "127.0.0.1"])
        reqs = echo_requests(read_icmp_packets(pcap))
        assert reqs, f"{label}: no echo requests captured"
        assert reqs[0]["payload_len"] == 56, (
            f"{label}: payload_len={reqs[0]['payload_len']} (expected 56)"
        )


def test_custom_payload_size(
    run_in_netns, capture_pcap, pcap_dir, ft_ping_bin, system_ping_bin,
):
    """`-s 32` reduces ICMP data length to 32 bytes."""
    for label, binary in [("system", system_ping_bin), ("ftping", ft_ping_bin)]:
        pcap = pcap_dir / f"{label}.pcap"
        with capture_pcap(pcap):
            run_in_netns([binary, "-c", "1", "-s", "32", "127.0.0.1"])
        reqs = echo_requests(read_icmp_packets(pcap))
        assert reqs, f"{label}: no echo requests captured"
        assert reqs[0]["payload_len"] == 32, (
            f"{label}: payload_len={reqs[0]['payload_len']} (expected 32)"
        )


def test_loopback_round_trip(
    run_in_netns, capture_pcap, pcap_dir, ft_ping_bin,
):
    """ft_ping should observe matching echo replies for every request."""
    pcap = pcap_dir / "ftping.pcap"
    with capture_pcap(pcap):
        run_in_netns([ft_ping_bin, "-c", "3", "127.0.0.1"])
    pkts = read_icmp_packets(pcap)
    reqs = echo_requests(pkts)
    reps = echo_replies(pkts)
    assert len(reqs) == 3
    assert len(reps) == 3
    # Every request should have a reply with the same id+seq.
    for req in reqs:
        match = [r for r in reps if r["id"] == req["id"] and r["seq"] == req["seq"]]
        assert match, f"no matching reply for id={req['id']} seq={req['seq']}"


def test_summary_view_shape(run_in_netns, capture_pcap, pcap_dir, ft_ping_bin):
    """Smoke test of the summary helper used by other tests."""
    pcap = pcap_dir / "ftping.pcap"
    with capture_pcap(pcap):
        run_in_netns([ft_ping_bin, "-c", "1", "127.0.0.1"])
    s = summarize(read_icmp_packets(pcap))
    assert s["n_requests"] == 1
    assert s["payload_len"] == 56
    assert s["ttl"] is not None
