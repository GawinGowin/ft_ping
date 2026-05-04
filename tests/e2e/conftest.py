"""pytest fixtures for ft_ping E2E tests.

These tests run inside a dedicated network namespace so the host
environment cannot influence (or be influenced by) the test traffic.
Root privileges are required (`ip netns`, raw sockets).
"""
from __future__ import annotations

import os
import shutil
import subprocess
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
    """
    @contextmanager
    def _capture(pcap_path: Path, iface: str = "lo", bpf: str = "icmp",
                 settle: float = 0.3, drain: float = 0.3):
        _require_tool("tcpdump")
        proc = subprocess.Popen(
            ["ip", "netns", "exec", netns,
             "tcpdump", "-i", iface, "-w", str(pcap_path), "-U", "-q", bpf],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        time.sleep(settle)
        try:
            yield pcap_path
        finally:
            time.sleep(drain)
            proc.terminate()
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=2)

    return _capture
