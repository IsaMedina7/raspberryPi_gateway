# CNC HMI Gateway - AI Coding Agent Instructions

## Project Overview

This is a **CNC Machine Gateway HMI** running on Raspberry Pi 3 as a central control hub connecting to both legacy MQTT machines and modern ESP32s running FluidNC via WebSocket.

### Architecture

The system uses **4 concurrent threads**:
1. **MQTT Thread** - Legacy machine communication (backward compatible)
2. **WebSocket Thread** - Direct ESP32/FluidNC control via native G-code
3. **UI Thread** - LVGL interface with SDL2 graphics
4. **AWS Thread** - Periodic cloud data uploads

## Critical Architecture Patterns

### 1. Dual Control Mode (MQTT vs WebSocket)

Controlled by `int use_websocket` flag in `src/ui/ui_events.c`:
```c
int use_websocket = 1;  // 1 = FluidNC WebSocket, 0 = Legacy MQTT
```

When WebSocket enabled:
- Commands formatted as FluidNC G-code: `$J=G91 G21 X+100 F1000`
- Responses parsed as JSON: `{"status":"Idle","pos":[x,y,z]}`
- Direct TCP port 81 connection to ESP32 machines

When MQTT enabled (legacy):
- Commands formatted as: `JOG:X:10`
- Uses traditional MQTT broker pubsub

### 2. Machine Configuration & Discovery

**File:** `machine_config.json`
```json
{
  "machines": [
    {"id": 1, "ip": "192.168.1.100"},
    {"id": 2, "ip": "192.168.1.101"}
  ]
}
```

Loading mechanism:
- `websocket_load_config()` called at thread startup
- IPs stored in `global_state_ws.maquinas[].ip_address` (mutex-protected)
- Auto-connect attempts every 6 seconds if disconnected
- Config can be updated via `websocket_set_machine_ip(id, ip_string)`

### 3. FluidNC Command Format

**Pattern:** `$J=G91 G21 [AXIS]+/-value F[feedrate]`

Components:
- `$J` = Jogging mode
- `G91` = Incremental positioning (relative movement)
- `G21` = Metric units (millimeters)
- `X/Y/Z` = Axis letter
- `+/-value` = Distance in mm (positive/negative direction)
- `F1000` = Feedrate in mm/min

**Examples from button clicks:**
```c
JogXPlus()  → "$J=G91 G21 X+10 F1000"  (10mm right)
JogZMinus() → "$J=G91 G21 Z-5 F1000"   (5mm down)
CmdHome()   → "$H"                      (home all axes)
CmdStop()   → "!"                       (soft reset)
```

Formatting handled by `fluidnc_format_jog(axis, value, feedrate, output)` in `src/websocket/fluidnc_formatter.c`.

### 4. WebSocket Thread Architecture

**File:** `src/websocket/websocket_service.c`

Flow:
1. Initialize libwebsockets context (single global)
2. Per-machine contexts in `machine_contexts[MAX_MAQUINAS]` array
3. Each machine has struct `ws_machine_context` holding:
   - `wsi` - WebSocket instance pointer
   - `rx_buffer` - Response buffer (1024 bytes)
   - `maquina_id` - Which machine this is

Callbacks:
- `LWS_CALLBACK_CLIENT_ESTABLISHED` - Connected to machine
- `LWS_CALLBACK_CLIENT_RECEIVE` - Got response from machine
- Auto-reconnect on `LWS_CALLBACK_CLOSED`

Response parsing `websocket_parse_response()` extracts:
- `"status"` field → `global_state_ws.maquinas[id].estado`
- `"pos"` array `[x,y,z]` → `pos_x`, `pos_y`, `pos_z`
- Updates UI via `ui_add_log()` and `global_state_ws.hay_actualizacion` flag

### 5. Global State Management

Two parallel state systems:
- `SystemState global_state` (MQTT legacy, in mqtt_service.h)
- `SystemStateWS global_state_ws` (WebSocket, in websocket_service.h)

Protected by separate mutexes:
- `state_mutex` for MQTT state
- `ws_state_mutex` for WebSocket state

UI thread polls both ~5ms interval, refreshes on `hay_actualizacion` flag.

## File Organization

| File | Purpose |
|------|---------|
| `src/websocket/websocket_service.c` | libwebsockets integration, connection management, response parsing |
| `src/websocket/fluidnc_formatter.c` | Convert jog events → G-code format |
| `src/config/machine_config.c` | JSON config file loading/saving for machine IPs |
| `src/ui/ui_events.c` | Button handlers (now dual-mode MQTT/WebSocket) |
| `src/main.c` | Thread spawning (now 4 threads instead of 3) |
| `machine_config.json` | Runtime machine IP configuration |

## Build & Dependencies

**New CMake entries:**
```cmake
find_package(websockets REQUIRED)    # libwebsockets library
find_package(json-c REQUIRED)         # JSON parsing
```

**New source files in SOURCES:**
- `src/websocket/websocket_service.c`
- `src/websocket/fluidnc_formatter.c`
- `src/config/machine_config.c`

**System packages required:**
```bash
sudo apt-get install libwebsockets-dev libjson-c-dev
```

## Common Workflows

### Add a New Machine (Runtime)

```c
// In UI event handler or configuration screen
websocket_set_machine_ip(4, "192.168.1.103");  // Set IP
websocket_connect_machine(4);                   // Connect
websocket_save_config("machine_config.json");   // Persist
```

### Change Jog Distance

Edit jog button function in `src/ui/ui_events.c`:
```c
void JogXPlus(lv_event_t * e) {
    char cmd[128];
    fluidnc_format_jog('X', 20.0, 1500, cmd);  // Change 10→20mm, F1000→1500
    enviar_orden_cnc(cmd);
}
```

### Switch Between MQTT and WebSocket

Toggle in `src/ui/ui_events.c`:
```c
int use_websocket = 0;  // 0 = use MQTT (legacy)
// or
int use_websocket = 1;  // 1 = use WebSocket (FluidNC)
```

Or implement per-machine logic:
```c
if (maquina_activa_id <= 3) {
    use_websocket = 1;  // Machines 1-3 are ESP32
} else {
    use_websocket = 0;  // Machine 4+ are MQTT legacy
}
```

### Debug WebSocket Connection

Check logs:
```bash
grep "\[WS\]" events.log              # All WebSocket messages
grep "\[WS ERROR\]" events.log        # Connection errors
tail -f events.log                     # Live monitoring
```

Or add debug output:
```c
printf("[WS DEBUG] Machine %d connected: %d\n", id, 
       global_state_ws.maquinas[id-1].connected);
```

## Important Gotchas

1. **Mutex Protection**: Always lock `ws_state_mutex` before accessing `global_state_ws` in non-thread-safe contexts
2. **libwebsockets Service Loop**: `lws_service()` must be called regularly (done every 50ms in thread)
3. **Command Format Validation**: FluidNC is picky about spacing in `$J=` commands—use formatter functions, don't hardcode
4. **JSON Response Format**: Different FluidNC firmware versions may return different JSON structure—update parser if needed
5. **Port Hardcoded**: WebSocket port is `#define WS_PORT 81`—change if machines use different port
6. **Buffer Sizes**: `WS_BUFFER_SIZE` is 1024 bytes—insufficient for large responses or multiple axis updates
7. **Thread-Safe Logging**: `logger_log()` uses file locking internally, safe to call from any thread

## Testing

1. Create `machine_config.json` with test IPs
2. Start app: `./cnc_app`
3. Select machine in UI dropdown
4. Click jog buttons → watch `events.log` for WebSocket commands
5. Verify ESP32 receives commands and responds

Example log output:
```
[2025-12-03 14:30:22] [WS] Connected to machine M1 at 192.168.1.100
[2025-12-03 14:30:23] [CMD TX] $J=G91 G21 X+10 F1000
[2025-12-03 14:30:24] [WS RX] Machine M1: {"status":"Idle","pos":[10.000,0.000,0.000]}
```

## Integration with Existing Systems

- **MQTT still runs** → dual-protocol capable
- **UI buttons unchanged** → same event handlers, different output
- **Global state both available** → can migrate machines incrementally
- **Logging unified** → all commands logged to `events.log`
