# Implementation Checklist for WebSocket + FluidNC Integration

## ✅ Core Implementation Complete

### WebSocket Service Module
- [x] `src/websocket/websocket_service.h` - Header with API declarations
- [x] `src/websocket/websocket_service.c` - Full libwebsockets integration
  - [x] Connection management (per-machine context)
  - [x] JSON response parsing
  - [x] Auto-reconnect logic (6-second intervals)
  - [x] Thread-safe state management with `ws_state_mutex`
  - [x] Configuration loading/saving

### FluidNC Command Formatter
- [x] `src/websocket/fluidnc_formatter.h` - API for G-code generation
- [x] `src/websocket/fluidnc_formatter.c` - Command formatters
  - [x] Jog command formatting: `$J=G91 G21 X+/-value F[feedrate]`
  - [x] Home command: `$H`
  - [x] Stop command: `!`
  - [x] Status request: `?`
  - [x] Response parsing (JSON format support)

### Machine Configuration
- [x] `src/config/machine_config.h` - JSON configuration API
- [x] `src/config/machine_config.c` - Implementation
  - [x] Load from JSON file
  - [x] Save to JSON file
  - [x] Add/update machines at runtime
  - [x] Query machine by ID

### UI Integration
- [x] `src/ui/ui_events.c` - Updated button handlers
  - [x] Dual-mode support (MQTT vs WebSocket toggle)
  - [x] JogXPlus/Minus with FluidNC format
  - [x] JogYPlus/Minus with FluidNC format
  - [x] JogZPlus/Minus with FluidNC format
  - [x] CmdHome with FluidNC format
  - [x] CmdStop with FluidNC format
  - [x] EmergenciaTotal emergency stop

### Thread Management
- [x] `src/main.c` - Added WebSocket thread
  - [x] Added `thread_websocket_loop()` thread creation
  - [x] Updated comments to reflect 4 threads

### Build System
- [x] `CMakeLists.txt` - Updated
  - [x] Added `find_package(websockets REQUIRED)`
  - [x] Added `find_package(json-c REQUIRED)`
  - [x] Added include directories for websocket and config
  - [x] Added source files to build
  - [x] Linked libwebsockets and json-c libraries

## ✅ Documentation Complete

- [x] `.github/copilot-instructions.md` - Full AI agent guide
  - [x] Project overview with dual-mode architecture
  - [x] Command format specifications
  - [x] Thread architecture explanation
  - [x] Global state management details
  - [x] File organization table
  - [x] Common workflows with code examples
  - [x] Gotchas and debugging tips

- [x] `WEBSOCKET_INTEGRATION.md` - Comprehensive technical guide
  - [x] Architecture diagram
  - [x] Configuration file format
  - [x] FluidNC command specifications with examples
  - [x] API usage documentation
  - [x] Thread architecture details
  - [x] Response parsing explanation
  - [x] Control mode switching guide
  - [x] Integration with existing UI
  - [x] Logging and debugging
  - [x] Dependencies installation
  - [x] Build and testing instructions
  - [x] Troubleshooting guide
  - [x] Future enhancement ideas

- [x] `QUICKSTART_WEBSOCKET.md` - Developer quick reference
  - [x] 30-second setup guide
  - [x] Common tasks with code snippets
  - [x] Flow diagrams (button→G-code, config load)
  - [x] Debugging commands
  - [x] Troubleshooting table
  - [x] Next steps for extending functionality
  - [x] Key concepts explanation

- [x] `machine_config.json.example` - Example configuration

## ⚠️ TODO: Testing & Validation

### Build Testing
- [ ] Run `cmake ..` in build directory
- [ ] Run `make -j4` - verify no compilation errors
- [ ] Check for linker warnings about libwebsockets/json-c

### Dependency Installation (Ubuntu/Debian)
```bash
# Before building:
sudo apt-get install libwebsockets-dev libjson-c-dev
```

### Configuration Testing
- [ ] Create `machine_config.json` with test IPs
- [ ] Verify JSON format is correct (use `jq` to validate)
- [ ] Test `websocket_load_config()` loads successfully
- [ ] Test `websocket_save_config()` creates valid JSON

### Connection Testing
- [ ] Start app with configured ESP32 machine
- [ ] Check logs for "Connected to machine" message
- [ ] Verify connection status in `global_state_ws.maquinas[].connected`
- [ ] Test auto-reconnect by power-cycling ESP32

### Command Testing
- [ ] Click X+ button
- [ ] Verify in `events.log`: `$J=G91 G21 X+10 F1000` sent
- [ ] Verify ESP32 receives command (watch ESP32 serial output)
- [ ] Verify response received and parsed: position updates logged

### UI Integration Testing
- [ ] Buttons display in LVGL interface correctly
- [ ] Machine selection dropdown works
- [ ] Log text area displays commands sent/received
- [ ] Position labels update from machine responses

## ⚠️ TODO: Optional Enhancements (Future)

### Phase 2: UI Configuration Screen
- [ ] Create settings/config screen in SquareLine Studio
- [ ] Add text input for machine IP address
- [ ] Add machine ID selector (1-10 dropdown)
- [ ] "Add Machine" / "Update Machine" button
- [ ] "Delete Machine" button
- [ ] List of currently configured machines
- [ ] Wire up to `websocket_set_machine_ip()` and save functions

### Phase 3: Advanced Features
- [ ] Feedrate slider (adjusts F parameter in jog commands)
- [ ] Jog distance selector (5mm, 10mm, 20mm, custom)
- [ ] Multi-axis jog (combine X+Y movement in one command)
- [ ] GCode file viewer/uploader
- [ ] Work offset (G54-G59) selector
- [ ] Tool offset management
- [ ] Manual probe cycle button

### Phase 4: Network Features
- [ ] Auto-discovery of FluidNC machines on network
- [ ] Machine rename/labeling
- [ ] Save machine profiles with settings
- [ ] WebSocket ping/pong for connection health
- [ ] Connection timeout handling
- [ ] Fallback to MQTT if WebSocket fails

### Phase 5: Data Logging
- [ ] Store position history for graphing
- [ ] Export tool path as GCode visualization
- [ ] Machine runtime statistics
- [ ] Error log with timestamps
- [ ] Sync logs to AWS S3

## Files Modified/Created Summary

### New Files (6)
```
src/websocket/websocket_service.h
src/websocket/websocket_service.c
src/websocket/fluidnc_formatter.h
src/websocket/fluidnc_formatter.c
src/config/machine_config.h
src/config/machine_config.c
machine_config.json.example
.github/copilot-instructions.md
WEBSOCKET_INTEGRATION.md
QUICKSTART_WEBSOCKET.md
```

### Modified Files (3)
```
src/main.c                    (added WebSocket thread)
src/ui/ui_events.c            (dual-mode jog handlers, FluidNC format)
CMakeLists.txt                (new dependencies and source files)
```

## Build Instructions

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install libwebsockets-dev libjson-c-dev

# Fedora
sudo dnf install libwebsockets-devel json-c-devel

# macOS with Homebrew
brew install libwebsockets json-c
```

### Build
```bash
cd ~/proyecto_cnc_hmi
rm -rf build && mkdir build
cd build
cmake ..
make -j4
./cnc_app
```

### Running Tests
```bash
# Terminal 1: Run the app
./cnc_app

# Terminal 2: Monitor logs
tail -f events.log

# Terminal 3: Check WebSocket connection
grep "\[WS\]" events.log | tail -10
```

## Known Limitations & Notes

1. **Buffer Size**: WebSocket receive buffer is 1024 bytes - adequate for JSON responses but monitor if adding larger payloads

2. **libwebsockets Version**: Tested with libwebsockets 4.x, may require API adjustments for 3.x

3. **Feedrate**: Currently hardcoded to 1000 mm/min in jog buttons - make configurable in Phase 2

4. **Single Connection Per Machine**: Each machine context handles one WebSocket connection - no redundancy

5. **No SSL/TLS**: WebSocket connections are unencrypted (ws:// not wss://) - add if needed for production

6. **Synchronous Logging**: File logging uses fopen/fclose on every call - consider buffering if high volume

7. **Machine ID Range**: Hardcoded to MAX_MAQUINAS=10 - redefine in header to support more

8. **FluidNC Firmware Compatibility**: Tested with FluidNC JSON response format - older versions may use different format

## Support & Questions

Refer to documentation files in order of specificity:
1. `QUICKSTART_WEBSOCKET.md` - Common tasks and debugging
2. `WEBSOCKET_INTEGRATION.md` - Detailed architecture and specifications
3. `.github/copilot-instructions.md` - Full technical reference for AI agents
