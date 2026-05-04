# ft_ping E2E Tests (pytest + scapy)

Black-box tests that compare `ft_ping` against the system `ping(8)` by
running both inside a dedicated network namespace and inspecting:

1. **Process output** — stdout structure and the stats summary
2. **Wire packets** — pcap captures via `tcpdump`, parsed with scapy

## Requirements

- Linux (uses `ip netns`)
- root (for `ip netns` and raw sockets)
- `ip`, `tcpdump`, system `ping` in `$PATH`
- [uv](https://docs.astral.sh/uv/) (pinned via `mise.toml` at repo root)
- `ft_ping` binary built (run `make` at the repo root)

## Running

```bash
# from the repo root
make                # build ft_ping
make e2e-deps       # uv sync (creates .venv with pytest + scapy)
make e2e            # sudo + uv run pytest

# or directly
cd tests/e2e
uv sync
sudo -E "$(command -v uv)" run pytest -v
```

`sudo -E` preserves env so `uv` can find its cache; passing the
absolute uv path keeps it working under sudo's secure_path.

If you're not root the suite skips itself; missing tools (`tcpdump`,
`ping`) also cause individual tests to skip.

## Layout

```
tests/e2e/
├── conftest.py        # fixtures: netns, ft_ping_bin, system_ping_bin,
│                      #           run_in_netns, capture_pcap, pcap_dir
├── normalize.py       # mask volatile fields (RTT/time/mdev)
├── pcap_utils.py      # scapy-based ICMP packet extraction
├── test_basic.py      # baseline tests: -c, -s, output structure
├── pyproject.toml     # uv project (deps + pytest config)
├── uv.lock
├── .python-version
└── README.md
```

## Adding a test

```python
def test_my_option(run_in_netns, capture_pcap, pcap_dir, ft_ping_bin):
    pcap = pcap_dir / "ftping.pcap"
    with capture_pcap(pcap):
        result = run_in_netns([ft_ping_bin, "-c", "1", "127.0.0.1"])
    assert result.returncode == 0
    pkts = read_icmp_packets(pcap)
    # ... assertions
```

## Future work

- `-t <ttl>` / `-Q <tos>` / `-e <id>` field-level pcap assertions
  (see `CLAUDE.local.md` Tasks 1–3)
- IPv6 (`ping6` / `ft_ping6`) once supported
- veth-based topology for non-loopback paths
