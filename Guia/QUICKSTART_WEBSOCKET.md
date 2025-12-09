# Quick Start: WebSocket + FluidNC Integration

## What Changed?

This project now supports **direct WebSocket communication** with ESP32 machines running FluidNC. You can:

✅ Send native G-code motion commands (e.g., `$J=G91 G21 X+100 F1000`)
✅ Receive real-time position updates as JSON
✅ Configure machine IPs via `machine_config.json`
✅ Maintain backward compatibility with legacy MQTT machines

## Files You Need to Know

### Core WebSocket Implementation
- **`src/websocket/websocket_service.c`** - Connection management, JSON parsing
- **`src/websocket/fluidnc_formatter.c`** - Convert button clicks to G-code
- **`src/config/machine_config.c`** - Load/save machine IPs

### Modified Existing Files
- **`src/ui/ui_events.c`** - Jog buttons now send FluidNC commands
- **`src/main.c`** - Added WebSocket thread
- **`CMakeLists.txt`** - Added libwebsockets + json-c dependencies

### Configuration
- **`machine_config.json`** - Store ESP32 machine IP addresses
- **`.github/copilot-instructions.md`** - Full architecture docs

## 30-Second Setup

### 1. Install Dependencies
```bash
sudo apt-get install libwebsockets-dev libjson-c-dev
```

### 2. Create Configuration
Copy the example and edit machine IPs:
```bash
cp machine_config.json.example machine_config.json
# Edit with your ESP32 IPs: 192.168.1.100, etc.
```

### 3. Build
```bash
cd build && cmake .. && make -j4
```

### 4. Run
```bash
./cnc_app
```

Watch the logs:
```bash
tail -f events.log
```

## Common Tasks

### Add a Machine via UI
You'll want to create a settings screen for this. Add to UI event handlers:

```c
void SetMachineIP(lv_event_t *e) {
    // Get IP from text input
    const char *ip = lv_textarea_get_text(ui_ipInput);
    int id = 1;  // Or get from dropdown
    
    // Save to config
    websocket_set_machine_ip(id, ip);
    websocket_connect_machine(id);
    websocket_save_config("machine_config.json");
}
```

### Change Jog Distance
Open `src/ui/ui_events.c`, find the jog function:

```c
void JogXPlus(lv_event_t * e) {
    if (use_websocket) {
        char cmd[128];
        fluidnc_format_jog('X', 10.0, 1000, cmd);  // 10mm = 10.0, adjust here
        enviar_orden_cnc(cmd);
    }
}
```

### Increase Feedrate
Same location, change the `1000`:

```c
fluidnc_format_jog('X', 10.0, 2000, cmd);  // 1000 mm/min → 2000 mm/min
```

### Switch Between MQTT and WebSocket
In `src/ui/ui_events.c`:

```c
int use_websocket = 1;  // Change to 0 for MQTT
```

Or per-machine:
```c
if (maquina_activa_id <= 3) use_websocket = 1;  // ESP32
else use_websocket = 0;                          // MQTT legacy
```

## Understanding the Flow

### Button Click → G-Code Command

```
User clicks "X+" button
    ↓
JogXPlus() called
    ↓
fluidnc_format_jog('X', 10.0, 1000, cmd)
    ↓
cmd = "$J=G91 G21 X+10 F1000"
    ↓
websocket_send_command(machine_id, cmd)
    ↓
[libwebsockets sends via TCP port 81]
    ↓
ESP32 receives and executes
    ↓
Responds: {"status":"Idle","pos":[10.000,0.000,0.000]}
    ↓
websocket_parse_response() extracts position
    ↓
UI updates position labels
```

### Machine Configuration Load

```
App starts
    ↓
thread_websocket_loop() launches
    ↓
websocket_load_config("machine_config.json")
    ↓
Parses JSON → fills global_state_ws.maquinas[] IP addresses
    ↓
websocket_connect_machine() for each configured IP
    ↓
Auto-reconnect every 6 seconds if disconnected
```

## Debugging

### Check Connection Status
```bash
grep "Connected to machine" events.log
grep "Connection failed" events.log
```

### Monitor Commands Being Sent
```bash
grep "\[WS TX\]" events.log
```

### Monitor Responses
```bash
grep "\[WS RX\]" events.log
```

### Full WebSocket Debug
```bash
grep "\[WS\]" events.log | tail -20
```

### Test Machine is Reachable
```bash
ping 192.168.1.100  # Or your ESP32 IP
# Or try WebSocket connection
curl -i -N -H "Connection: Upgrade" -H "Upgrade: websocket" \
  ws://192.168.1.100:81/
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "Connection refused" | Check IP in `machine_config.json`, verify ESP32 is on |
| "Failed to parse JSON" | FluidNC firmware may use different format, update parser in `websocket_parse_response()` |
| No position updates | Machine not responding, check `/opt/fluiddnc/config.yaml` on ESP32 |
| Commands not received | Verify WebSocket connected in logs, check firewall |
| App crashes on startup | Missing libwebsockets/json-c, reinstall dependencies |

## Next Steps

### To Add UI for IP Configuration

Create a new screen in SquareLine Studio with:
- Text input for IP address
- Dropdown to select machine ID (1-10)
- "Add Machine" button
- List of configured machines

Then hook up event handlers:
```c
void AddMachineButtonClick(lv_event_t *e) {
    const char *ip = lv_textarea_get_text(ui_ipInput);
    int id = lv_dropdown_get_selected(ui_machineDropdown) + 1;
    
    websocket_set_machine_ip(id, ip);
    websocket_connect_machine(id);
    websocket_save_config("machine_config.json");
    ui_add_log("Machine added/updated");
}
```

### To Support Multi-Axis Jog

Modify formatter to accept multiple axes:
```c
int fluidnc_format_multi_jog(const char *axes, const float *values, int feedrate, char *output)
{
    // Format: $J=G91 G21 X+10 Y-5 Z+2 F1000
    // axes = "XYZ", values = [10, -5, 2]
}
```

### To Add GCode File Upload

Use websocket_send_command() to stream file contents:
```c
void UploadGCodeFile(const char *filename, int machine_id) {
    FILE *f = fopen(filename, "r");
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Strip newline
        line[strcspn(line, "\n")] = 0;
        websocket_send_command(machine_id, line);
        usleep(100000);  // 100ms between commands
    }
}
```

## Architecture Diagram

```
┌──────────────────────────────────────┐
│  Jog Button in UI (LVGL)             │
│  JogXPlus() → X+10mm movement        │
└──────────────┬───────────────────────┘
               │
┌──────────────▼───────────────────────┐
│  enviar_orden_cnc()                  │
│  Smart dispatcher:                    │
│  - if use_websocket=1 → call WS      │
│  - if use_websocket=0 → call MQTT    │
└──────────────┬───────────────────────┘
               │
       ┌───────┴────────┐
       │                │
       ▼                ▼
   [WebSocket]     [MQTT]
   Port 81         Broker
       │                │
       ▼                ▼
   ESP32           Old Machine
   FluidNC         (MQTT mode)
```

## Key Concepts

**FluidNC Command**: `$J=G91 G21 X+100 F1000`
- Sending position update commands to ESP32
- Jogging mode ($J), incremental (G91), metric (G21)

**WebSocket**: Async TCP connection, real-time bidirectional
- Button click → send command immediately
- Machine response → received in callback (no polling)

**JSON Responses**: `{"status":"Idle","pos":[10,0,0]}`
- Parsed in `websocket_parse_response()`
- Position auto-updates UI via `hay_actualizacion` flag

**Machine Config**: Simple JSON file listing IPs
- Loaded at startup
- Edited at runtime
- Persisted with `websocket_save_config()`

Happy coding! 🚀
