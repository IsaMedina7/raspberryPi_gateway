# WebSocket + FluidNC Integration Guide

## Overview

This CNC HMI now supports direct WebSocket communication with ESP32 machines running FluidNC. This replaces the previous MQTT-only approach and provides native G-code command control.

### Architecture

```
┌─────────────────────────────────────────────┐
│   Raspberry Pi 3 CNC Gateway (This App)    │
│  ┌────────────────────────────────────────┐ │
│  │ UI Thread (LVGL)                      │ │
│  │ - Jog buttons (X, Y, Z)               │ │
│  │ - Home/Stop commands                  │ │
│  │ - Machine selection dropdown          │ │
│  └───────────────┬────────────────────────┘ │
│                  │                           │
│  ┌──────────────┴──────────────────────────┐ │
│  │ Command Converter                      │ │
│  │ - FluidNC Formatter                    │ │
│  │ - Converts jog commands to G-code      │ │
│  └──────────┬──────────────┬──────────────┘ │
│             │              │                 │
│  ┌──────────▼─┐   ┌────────▼──────────┐    │
│  │  MQTT      │   │  WebSocket Thread │    │
│  │  Thread    │   │ (New FluidNC)     │    │
│  │  (Legacy)  │   │                   │    │
│  └──────────┬─┘   └────────┬──────────┘    │
│             │              │                 │
└─────────────┼──────────────┼─────────────────┘
              │              │
         ┌────▼────────┬─────▼───────┐
         │   MQTT      │  WebSocket  │
         │   Broker    │  (Port 81)  │
         │             │             │
         └─────────────┴─────────────┘
              │              │
         ┌────▼────────┬─────▼───────────┐
         │Old Machines │ESP32 + FluidNC  │
         │(MQTT only)  │(WebSocket native│
         │             │G-code commands) │
         └─────────────┴─────────────────┘
```

## Configuration File Format

Machine IP addresses are stored in `machine_config.json`:

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
    },
    {
      "id": 3,
      "ip": "192.168.1.102"
    }
  ]
}
```

## FluidNC Command Format

### Jog Commands (Movement)

Pattern: `$J=G91 G21 [AXIS]+/-value F[feedrate]`

- `G91` = Incremental mode (relative movement)
- `G21` = Metric units (millimeters)
- Axis: X, Y, or Z
- Value: Distance in mm (positive = forward, negative = backward)
- F: Feedrate in mm/min

**Examples:**

```
$J=G91 G21 X+100 F1000    # Move X axis +100mm at 1000mm/min
$J=G91 G21 Y-50 F500      # Move Y axis -50mm at 500mm/min
$J=G91 G21 Z+10 F800      # Move Z axis +10mm at 800mm/min
```

### Home Command

```
$H      # Home all axes (seeks zero position)
```

### Stop Command

```
!       # Soft reset/feed hold (stops current motion)
```

### Status Request

```
?       # Request current machine status and position
```

## API Usage

### Sending Commands to a Machine

```c
// From ui_events.c
void JogXPlus(lv_event_t * e) {
    if (use_websocket) {
        char cmd[128];
        fluidnc_format_jog('X', 10.0, 1000, cmd);  // 10mm right
        enviar_orden_cnc(cmd);  // Sends: $J=G91 G21 X+10 F1000
    }
}
```

### Setting Machine IP

```c
// From websocket_service.c
websocket_set_machine_ip(1, "192.168.1.100");  // Configure machine 1
websocket_connect_machine(1);                   // Connect to machine 1
```

### Loading/Saving Configuration

```c
websocket_load_config("machine_config.json");   // Load at startup
websocket_save_config("machine_config.json");   // Save after changes
```

## Thread Architecture

### 1. WebSocket Thread (`thread_websocket_loop`)
- Runs in `src/main.c` as separate pthread
- Initializes libwebsockets context
- Maintains connections to multiple ESP32 machines
- Handles async WebSocket callbacks
- Auto-reconnects to offline machines every 6 seconds
- Parses FluidNC JSON responses

### 2. UI Thread (Unchanged)
- Still runs LVGL event loop
- Now calls WebSocket commands instead of (or in addition to) MQTT
- Uses same button handlers for jog commands

### 3. MQTT Thread (Legacy Support)
- Still available for older machines
- Can run in parallel with WebSocket

## Response Parsing

FluidNC machines respond with JSON status:

```json
{
  "status": "Idle",
  "pos": [0.000, 10.500, 5.250],
  "state": 0
}
```

Parser in `websocket_service.c`:
- Extracts status string → `global_state_ws.maquinas[id].estado`
- Extracts position array → `pos_x, pos_y, pos_z`
- Updates UI automatically

## Switching Between MQTT and WebSocket

In `src/ui/ui_events.c`:

```c
int use_websocket = 1;  // 1 = WebSocket, 0 = MQTT (legacy)
```

Set to `1` to use WebSocket + FluidNC (new machines)
Set to `0` to use MQTT (legacy machines)

Or implement auto-detection based on machine configuration.

## Integration with Existing UI

### Button Handlers Updated

| Button | MQTT Format | WebSocket Format | Example |
|--------|-------------|------------------|---------|
| X+ | `JOG:X:10` | `$J=G91 G21 X+10 F1000` | 10mm right |
| X- | `JOG:X:-10` | `$J=G91 G21 X-10 F1000` | 10mm left |
| Y+ | `JOG:Y:10` | `$J=G91 G21 Y+10 F1000` | 10mm forward |
| Y- | `JOG:Y:-10` | `$J=G91 G21 Y-10 F1000` | 10mm backward |
| Z+ | `JOG:Z:5` | `$J=G91 G21 Z+5 F1000` | 5mm up |
| Z- | `JOG:Z:-5` | `$J=G91 G21 Z-5 F1000` | 5mm down |
| Home | `HOME` | `$H` | Home all axes |
| Stop | `STOP` | `!` | Emergency stop |

## Logging

All WebSocket commands are logged to `events.log`:

```
[2025-12-03 14:23:45] [WS] Comando enviado a M1: $J=G91 G21 X+10 F1000
[2025-12-03 14:23:46] [WS] Machine M1 status: Idle
[2025-12-03 14:23:46] [WS] M1 position: X=10.000 Y=0.000 Z=0.000
```

## Dependencies

### New Libraries Required

- `libwebsockets` - WebSocket client
- `json-c` - JSON parsing for FluidNC responses

### Installation

```bash
# Ubuntu/Debian
sudo apt-get install libwebsockets-dev libjson-c-dev

# Fedora
sudo dnf install libwebsockets-devel json-c-devel

# Arch
sudo pacman -S libwebsockets json-c
```

## Building

```bash
cd ~/proyecto_cnc_hmi/build
cmake ..
make -j4
./cnc_app
```

## Testing

1. **Create machine config:**
   ```json
   {
     "machines": [
       {"id": 1, "ip": "192.168.1.100"}
     ]
   }
   ```

2. **Start the app:**
   ```bash
   ./cnc_app
   ```

3. **Monitor logs:**
   ```bash
   tail -f events.log
   ```

4. **Click jog buttons** - should see FluidNC commands in the log

## Troubleshooting

### "Connection refused" errors

- Check ESP32 machine IP address in `machine_config.json`
- Verify ESP32 is powered on and running FluidNC firmware
- Ensure network connectivity: `ping 192.168.1.100`

### "Failed to parse JSON" warnings

- FluidNC version mismatch - may use different response format
- Check FluidNC firmware version on ESP32
- Update `websocket_parse_response()` if format changed

### No position updates

- Machine may not support JSON responses
- Try requesting status manually: send `?` command
- Check WebSocket connection logs: `grep "\[WS\]" events.log`

### Commands not reaching machine

- Verify machine IP is configured correctly
- Check WebSocket connection status in UI
- Review libwebsockets log output for connection errors

## Future Enhancements

1. **Multi-axis jog** - Send combined commands: `X+10 Y-5 Z+2`
2. **Feedrate control** - Slider to adjust F parameter dynamically
3. **Auto-detection** - Scan network for FluidNC machines
4. **Machine profiles** - Save/restore tool offsets and work offsets
5. **File upload** - Send GCode files to ESP32 SPIFFS storage
6. **Real-time status** - Automatic periodic status polling
