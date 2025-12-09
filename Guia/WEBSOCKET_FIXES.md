# WebSocket Connection Fixes for FluidNC

## Issues Fixed

The original WebSocket implementation had several connection errors that have been corrected:

### 1. **Protocol Name Mismatch**
- **Problem**: Used custom protocol name "cnc-fluidnc" which FluidNC doesn't recognize
- **Fix**: Changed to "http" protocol which is the standard for WebSocket upgrades

### 2. **Missing HTTP Headers**
- **Problem**: WebSocket handshake requires Host and Origin headers for proper HTTP upgrade
- **Fix**: Added `ccinfo.host` and `ccinfo.origin` fields in the connection info

### 3. **Missing User Data Assignment**
- **Problem**: Callback wasn't receiving machine context properly
- **Fix**: Added `ccinfo.opaque_user_data` to ensure context is available in callbacks

### 4. **Improper Context Initialization**
- **Problem**: Context creation info was incomplete for client-side WebSocket
- **Fix**: Added proper options flags for client context initialization

## Configuration

### Example `machine_config.json`

```json
{
  "machines": [
    {
      "id": 1,
      "ip": "192.168.1.100"
    },
    {
      "id": 2,
      "ip": "192.168.1.101"
    }
  ]
}
```

### FluidNC ESP32 Setup

Ensure your ESP32 is running FluidNC with WebSocket enabled (default on port 81):

1. Upload FluidNC firmware to ESP32
2. Configure network: WiFi SSID + password
3. Verify WebSocket is accessible:
   ```bash
   curl -i -N -H "Connection: Upgrade" -H "Upgrade: websocket" \
     http://192.168.1.100:81/
   ```

4. Test with Python to verify connectivity:
   ```python
   import websocket
   ws = websocket.create_connection("ws://192.168.1.100:81/")
   ws.send("$J=G91 G21 X+10 F1000")
   result = ws.recv()
   print(result)
   ws.close()
   ```

## Testing the Fixed Connection

1. Create `machine_config.json` in the build directory with your ESP32 IP addresses
2. Run the application:
   ```bash
   cd ~/proyecto_cnc_hmi/build
   ./cnc_app
   ```

3. Monitor output for connection messages:
   ```
   [WS] Attempting connection to M1 at ws://192.168.1.100:81
   [WS] Connected to machine M1 at 192.168.1.100
   ```

## Command Format

Once connected, jog buttons send FluidNC G-code commands:

```
$J=G91 G21 X+10 F1000    # Move X axis 10mm at 1000mm/min (jogging)
$H                        # Home all axes
!                         # Stop/emergency
?                         # Query status
```

## Debugging

If connection still fails:

1. **Check network connectivity:**
   ```bash
   ping 192.168.1.100
   nc -zv 192.168.1.100 81  # Test if port 81 is open
   ```

2. **Enable WebSocket logging in app** - Rebuild with verbose output:
   - Check `events.log` for detailed connection attempts
   - Terminal output shows `[WS]` prefixed messages

3. **Verify ESP32 is running FluidNC:**
   - Access web interface: `http://192.168.1.100`
   - Check firmware version and WebSocket support

4. **Network issues:**
   - Ensure same subnet as ESP32
   - Check firewall rules (port 81 must be open)
   - Verify WiFi signal strength on ESP32

## Code Changes Summary

| File | Change |
|------|--------|
| `websocket_service.c` | Protocol set to "http", added Host/Origin headers, improved context init |
| `websocket_service.h` | No changes (interface remains same) |
| `CMakeLists.txt` | Now links `json-c` library directly |

The fixed code should now successfully establish WebSocket connections to FluidNC machines matching what Python's websocket library achieves.
