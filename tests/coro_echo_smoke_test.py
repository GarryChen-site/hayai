#!/usr/bin/env python3
"""Smoke test for the coro_echo_server (with spawn())."""
import subprocess, socket, time, sys

PORT = 18896
server = subprocess.Popen(
    ["./build/coro_echo_server", str(PORT)],
    stdout=subprocess.DEVNULL,   # server stdout buffered/not needed
    stderr=subprocess.DEVNULL,
)

try:
    # Give server time to start accepting
    time.sleep(0.8)

    if server.poll() is not None:
        print("Server crashed on startup!", file=sys.stderr)
        sys.exit(1)

    # Single echo
    s = socket.socket()
    s.settimeout(3.0)
    s.connect(("127.0.0.1", PORT))
    msg = b"Hello Coroutines!"
    s.sendall(msg)
    resp = s.recv(1024)
    s.close()
    assert resp == msg, f"Expected {msg!r}, got {resp!r}"
    print(f"Single echo: PASS ({resp.decode()!r})")

    # Multiple concurrent clients
    clients = []
    for i in range(3):
        c = socket.socket()
        c.settimeout(3.0)
        c.connect(("127.0.0.1", PORT))
        clients.append(c)

    for i, c in enumerate(clients):
        c.sendall(f"Client {i}".encode())

    for i, c in enumerate(clients):
        r = c.recv(1024)
        assert r == f"Client {i}".encode(), f"Mismatch client {i}: {r!r}"
        c.close()
        print(f"Concurrent client {i}: PASS")

    print("\nAll smoke tests PASSED!")
    sys.exit(0)

except Exception as e:
    print(f"FAILED: {e}", file=sys.stderr)
    import traceback; traceback.print_exc()
    sys.exit(1)

finally:
    server.terminate()
    server.wait(timeout=2)
