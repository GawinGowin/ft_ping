"""pytest fixtures for ft_ping E2E tests.

These tests run inside a dedicated network namespace so the host
environment cannot influence (or be influenced by) the test traffic.
Root privileges are required (`ip netns`, raw sockets).
"""
from __future__ import annotations

import os
import shutil
import subprocess
import threading
import time
from contextlib import contextmanager
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[2]
NETNS_NAME = "ftping_e2e"


def _run(cmd: list[str], **kwargs) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True, **kwargs)


def _require_root() -> None:
    if os.geteuid() != 0:
        pytest.skip("E2E tests require root (ip netns + raw socket)")


def _require_tool(name: str) -> None:
    if shutil.which(name) is None:
        pytest.skip(f"Required tool not found in PATH: {name}")


@pytest.fixture(scope="session")
def ft_ping_bin() -> str:
    binary = REPO_ROOT / "ft_ping"
    if not binary.exists():
        pytest.fail(
            f"ft_ping binary not built. Run `make` in {REPO_ROOT} first."
            f" Looking for: {binary}"
        )
    return str(binary)


@pytest.fixture(scope="session")
def system_ping_bin() -> str:
    p = shutil.which("ping")
    if p is None:
        pytest.skip("system ping not found in PATH")
    return p


@pytest.fixture
def netns() -> str:
    """Create an isolated netns with loopback up. Yields the netns name."""
    _require_root()
    _require_tool("ip")

    # Best-effort cleanup of any leftover from a previous failed run.
    _run(["ip", "netns", "del", NETNS_NAME])

    res = _run(["ip", "netns", "add", NETNS_NAME])
    assert res.returncode == 0, f"netns add failed: {res.stderr}"

    res = _run(["ip", "netns", "exec", NETNS_NAME, "ip", "link", "set", "lo", "up"])
    assert res.returncode == 0, f"lo up failed: {res.stderr}"

    # Without an explicit address, source-address selection for 127.0.0.0/8
    # is not guaranteed in a fresh netns — assign it so probes/pings work.
    _run(["ip", "netns", "exec", NETNS_NAME,
          "ip", "addr", "add", "127.0.0.1/8", "dev", "lo"])

    try:
        yield NETNS_NAME
    finally:
        _run(["ip", "netns", "del", NETNS_NAME])


@pytest.fixture
def run_in_netns(netns: str):
    """Return a callable that runs a command inside the netns."""
    def _exec(argv: list[str], timeout: float = 10.0) -> subprocess.CompletedProcess:
        cmd = ["ip", "netns", "exec", netns] + list(argv)
        return subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    return _exec


@pytest.fixture
def pcap_dir(tmp_path: Path) -> Path:
    """Directory for pcap files; auto-cleaned with the test's tmp_path."""
    return tmp_path


@pytest.fixture
def capture_pcap(netns: str):
    """Return a context manager factory:

        with capture_pcap(pcap_path):
            run_in_netns([...])

    Blocks until we confirm that packets are actually being captured by
    sending a UDP probe and waiting for the pcap file to grow.
    This eliminates the race where the first packet escapes capture.
    """
    @contextmanager
    def _capture(pcap_path: Path, iface: str = "lo", bpf: str = "icmp",
                 ready_timeout: float = 5.0, drain: float = 0.3):
        _require_tool("tcpdump")

        # Ensure we capture the UDP probe even if the user only wants ICMP.
        # We use port 9 (Discard) to avoid side effects.
        actual_bpf = f"({bpf}) or (udp and port 9)" if bpf else "udp and port 9"

        # -U (packet-buffered) is essential for immediate file growth.
        proc = subprocess.Popen(
            ["ip", "netns", "exec", netns,
             "tcpdump", "-i", iface, "-w", str(pcap_path), "-U", actual_bpf],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
        )

        # Drain stderr in a thread so the pipe buffer can never fill and
        # block tcpdump. We don't gate readiness on stderr content (which
        # can be buffered); we use the pcap-grew probe as the real signal.
        stderr_buf: list[bytes] = []

        def _drain_stderr():
            assert proc.stderr is not None
            for chunk in iter(lambda: proc.stderr.read(4096), b""):
                stderr_buf.append(chunk)

        reader = threading.Thread(target=_drain_stderr, daemon=True)
        reader.start()

        def _stderr_text() -> str:
            return b"".join(stderr_buf).decode(errors="replace")

        # Probe and wait for file growth.
        # tcpdump writes a 24-byte pcap header immediately on open; we
        # consider capture ready once the file is bigger than that, which
        # only happens after a packet has actually been written.
        start_time = time.time()
        probe_cmd = [
            "ip", "netns", "exec", netns,
            "python3", "-c",
            "import socket; "
            "s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); "
            "s.sendto(b'probe', ('127.0.0.1', 9))"
        ]

        def get_pcap_size() -> int:
            try:
                return pcap_path.stat().st_size
            except FileNotFoundError:
                return 0

        ready = False
        while time.time() - start_time < ready_timeout:
            # Bail out fast if tcpdump exited (BPF parse error, EPERM, ...).
            if proc.poll() is not None:
                raise RuntimeError(
                    f"tcpdump exited early (rc={proc.returncode}).\n"
                    f"stderr:\n{_stderr_text()}"
                )
            _run(probe_cmd)
            time.sleep(0.1)
            if get_pcap_size() > 24:
                ready = True
                break

        if not ready:
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()
            raise RuntimeError(
                f"pcap file did not grow after UDP probe within {ready_timeout}s. "
                f"Current size: {get_pcap_size()}\n"
                f"tcpdump stderr:\n{_stderr_text()}"
            )

        try:
            yield pcap_path
        finally:
            # Let any in-flight packets reach the pcap before we tear down.
            time.sleep(drain)
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=2)
            reader.join(timeout=1)

    return _capture
