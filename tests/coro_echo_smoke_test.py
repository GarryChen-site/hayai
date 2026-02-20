#!/usr/bin/env python3
"""Verbose smoke test for the coro_echo_server."""
import subprocess, socket, time, sys, os

PORT = 18890
server = subprocess.Popen(
    ["./build/coro_echo_server", str(PORT)],
    stdout=subprocess.PIPE, stderr=subprocess.STDOUT
)

try:
    # Give server more time to start
    time.sleep(0.8)

    # Check server is running
    if server.poll() is not None:
        out, _ = server.communicate(timeout=2)
        print("SERVER DIED:", out.decode())
        sys.exit(1)

    # Single echo
    s = socket.socket()
    s.settimeout(3.0)
    try:
        s.connect(("127.0.0.1", PORT))
    except ConnectionRefusedError:
        print("Connection refused - server not accepting!")
        out = server.stdout.read1(4096)
        print("Server output:", out.decode())
        sys.exit(1)
    msg = b"Hello Coroutines!"
    s.sendall(msg)
    try:
        resp = s.recv(1024)
    except socket.timeout:
        print("Timeout waiting for echo!")
        s.close()
        sys.exit(1)
    s.close()

    assert resp == msg, f"Expected {msg!r}, got {resp!r}"
    print(f"Single echo: PASS ({resp.decode()!r})")
    print("\nAll smoke tests PASSED!")
    sys.exit(0)

except Exception as e:
    print(f"FAILED: {e}", file=sys.stderr)
    import traceback
    traceback.print_exc()
    sys.exit(1)

finally:
    server.terminate()
    out, _ = server.communicate(timeout=2)
    if out:
        print("\nServer output:")
        print(out.decode())
