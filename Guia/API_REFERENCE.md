# WebSocket + FluidNC API Reference

## Quick API Overview

### Core Functions

#### WebSocket Service (`src/websocket/websocket_service.h`)

```c
// Thread management
void* thread_websocket_loop(void* arg);

// Connection management
int websocket_connect_machine(int maquina_id);
int websocket_disconnect_machine(int maquina_id);

// Command sending
void websocket_send_command(int maquina_id, const char* cmd_format);

// Configuration
int websocket_set_machine_ip(int maquina_id, const char* ip_address);
int websocket_load_config(const char* config_file);
int websocket_save_config(const char* config_file);
```

#### FluidNC Formatter (`src/websocket/fluidnc_formatter.h`)

```c
// Command formatting
int fluidnc_format_jog(char axis, float value, int feedrate, char *output);
int fluidnc_format_home(char *output);
int fluidnc_format_stop(char *output);
int fluidnc_format_status(char *output);

// Response parsing
int fluidnc_parse_mpos(const char *response, float *x, float *y, float *z);
```

#### Machine Config (`src/config/machine_config.h`)

```c
// File I/O
int config_load(const char *filename, MachinesConfigList *config);
int config_save(const char *filename, MachinesConfigList *config);

// Runtime management
int config_add_machine(MachinesConfigList *config, int id, const char *ip);
const char* config_get_machine_ip(MachinesConfigList *config, int id);
```

---

## Detailed API Reference

### WebSocket Service API

#### `void* thread_websocket_loop(void* arg)`

**Purpose**: Main WebSocket thread - starts libwebsockets context and manages connections

**Parameters**:
- `arg`: Thread argument (unused, pass NULL)

**Returns**: NULL on thread exit (never returns in normal operation)

**Behavior**:
- Initializes `global_state_ws` and `ws_state_mutex`
- Loads `machine_config.json`
- Creates libwebsockets context
- Attempts connection to all configured machines
- Main event loop calls `lws_service(ws_context, 50)` every 50ms
- Auto-reconnects offline machines every 6 seconds

**Example**:
```c
pthread_t t_ws;
pthread_create(&t_ws, NULL, thread_websocket_loop, NULL);
```

---

#### `int websocket_connect_machine(int maquina_id)`

**Purpose**: Initiate WebSocket connection to a specific machine

**Parameters**:
- `maquina_id`: Machine ID (1-10)

**Returns**:
- `0` on success (connection initiated, may take time)
- `-1` on error (invalid ID or libwebsockets error)

**Requirements**:
- Machine IP must be set via `websocket_set_machine_ip()` first
- Machine must be on network and running FluidNC on port 81

**Behavior**:
- Initiates TCP connection (async, not blocking)
- Creates per-machine context in `machine_contexts[id-1]`
- Callback will fire `LWS_CALLBACK_CLIENT_ESTABLISHED` on success
- Sets `global_state_ws.maquinas[id-1].connected = 1` on success

**Example**:
```c
websocket_set_machine_ip(1, "192.168.1.100");
websocket_connect_machine(1);
// Wait for callback... check: global_state_ws.maquinas[0].connected == 1
```

---

#### `int websocket_disconnect_machine(int maquina_id)`

**Purpose**: Close WebSocket connection to a machine

**Parameters**:
- `maquina_id`: Machine ID (1-10)

**Returns**:
- `0` on success
- `-1` on error (invalid ID)

**Behavior**:
- Calls `lws_set_timeout()` to gracefully close connection
- Sets `wsi` to NULL in per-machine context
- Callback will fire `LWS_CALLBACK_CLOSED`
- Sets `global_state_ws.maquinas[id-1].connected = 0`

**Example**:
```c
websocket_disconnect_machine(1);
```

---

#### `void websocket_send_command(int maquina_id, const char* cmd_format)`

**Purpose**: Send a command to a machine

**Parameters**:
- `maquina_id`: Machine ID (1-10)
- `cmd_format`: Command string (FluidNC format, e.g., `"$J=G91 G21 X+10 F1000"`)

**Returns**: void

**Requirements**:
- Machine must be connected (`global_state_ws.maquinas[id-1].connected == 1`)
- Command must be valid FluidNC format

**Behavior**:
- Validates machine is connected
- Allocates LWS_PRE buffer space
- Calls `lws_write()` with LWS_WRITE_TEXT
- Logs to file and stdout on success
- Logs error if write fails (machine not connected)
- Does NOT wait for response (async)

**Example**:
```c
// Send jog command
websocket_send_command(1, "$J=G91 G21 X+10 F1000");

// Send home command
websocket_send_command(1, "$H");

// Send status request
websocket_send_command(1, "?");
```

**Error Handling**:
```c
// Check connection before sending
if (global_state_ws.maquinas[0].connected) {
    websocket_send_command(1, "$J=G91 G21 X+10 F1000");
} else {
    printf("Machine not connected\n");
}
```

---

#### `int websocket_set_machine_ip(int maquina_id, const char* ip_address)`

**Purpose**: Configure machine IP address

**Parameters**:
- `maquina_id`: Machine ID (1-10)
- `ip_address`: IP address string (e.g., `"192.168.1.100"`)

**Returns**:
- `0` on success
- `-1` on error (invalid ID or NULL pointer)

**Behavior**:
- Acquires `ws_state_mutex`
- Stores IP in `global_state_ws.maquinas[id-1].ip_address`
- Sets `global_state_ws.maquinas[id-1].id = maquina_id`
- Releases mutex
- Logs to system

**Example**:
```c
websocket_set_machine_ip(1, "192.168.1.100");
websocket_set_machine_ip(2, "192.168.1.101");
websocket_set_machine_ip(3, "192.168.1.102");
```

---

#### `int websocket_load_config(const char* config_file)`

**Purpose**: Load machine configuration from JSON file

**Parameters**:
- `config_file`: Path to JSON config file (e.g., `"machine_config.json"`)

**Returns**:
- `0` on success
- `-1` on error (file not found, parse error)

**File Format**:
```json
{
  "machines": [
    {"id": 1, "ip": "192.168.1.100"},
    {"id": 2, "ip": "192.168.1.101"}
  ]
}
```

**Behavior**:
- Opens file and reads content (max 4096 bytes)
- Parses JSON using json-c library
- For each machine in array:
  - Extracts `id` and `ip` fields
  - Calls `websocket_set_machine_ip(id, ip)` internally
- Logs loaded machines

**Example**:
```c
// In thread_websocket_loop at startup:
websocket_load_config("machine_config.json");

// After loading, machines are configured
// websocket_connect_machine() can then be called
```

---

#### `int websocket_save_config(const char* config_file)`

**Purpose**: Save current machine configuration to JSON file

**Parameters**:
- `config_file`: Path to output JSON file

**Returns**:
- `0` on success
- `-1` on error (can't open file for writing)

**Behavior**:
- Acquires `ws_state_mutex`
- Iterates through all machines with configured IPs
- Creates JSON object with machines array
- Releases mutex
- Opens file for writing
- Writes JSON with pretty formatting
- Closes file

**Example**:
```c
// After adding/modifying machines at runtime:
websocket_set_machine_ip(1, "192.168.1.100");
websocket_save_config("machine_config.json");  // Persist changes
```

---

### FluidNC Formatter API

#### `int fluidnc_format_jog(char axis, float value, int feedrate, char *output)`

**Purpose**: Format a jog (jogging) motion command for FluidNC

**Parameters**:
- `axis`: Axis character ('X', 'Y', or 'Z')
- `value`: Distance in mm (positive = forward, negative = backward)
- `feedrate`: Speed in mm/min
- `output`: Output buffer (must be at least FLUIDNC_CMD_MAX=128 bytes)

**Returns**: Length of formatted string

**Output Format**: `$J=G91 G21 X±value Ffeedrate`

**Behavior**:
- Validates inputs
- Generates FluidNC jogging command
- Handles sign correctly (+ or -)
- Writes to output buffer
- Ensures null termination

**Example**:
```c
char cmd[128];

// 10mm right on X axis
fluidnc_format_jog('X', 10.0, 1000, cmd);
// Result: "$J=G91 G21 X+10.0 F1000"

// 5mm left on Y axis
fluidnc_format_jog('Y', -5.0, 1000, cmd);
// Result: "$J=G91 G21 Y-5.0 F1000"

// Used in button handlers:
void JogXPlus(lv_event_t * e) {
    char cmd[128];
    fluidnc_format_jog('X', 10.0, 1000, cmd);
    websocket_send_command(maquina_activa_id, cmd);
}
```

---

#### `int fluidnc_format_home(char *output)`

**Purpose**: Format homing command (return to zero position)

**Parameters**:
- `output`: Output buffer (must be at least FLUIDNC_CMD_MAX)

**Returns**: Length of formatted string (typically 2)

**Output Format**: `$H`

**Example**:
```c
char cmd[128];
fluidnc_format_home(cmd);
// Result: "$H"
websocket_send_command(machine_id, cmd);
```

---

#### `int fluidnc_format_stop(char *output)`

**Purpose**: Format stop/feed hold command

**Parameters**:
- `output`: Output buffer (must be at least FLUIDNC_CMD_MAX)

**Returns**: Length of formatted string (typically 1)

**Output Format**: `!`

**Behavior**: Sends feed hold (stops current motion without hard reset)

**Example**:
```c
char cmd[128];
fluidnc_format_stop(cmd);
// Result: "!"
websocket_send_command(machine_id, cmd);
```

---

#### `int fluidnc_format_status(char *output)`

**Purpose**: Format status request command

**Parameters**:
- `output`: Output buffer (must be at least FLUIDNC_CMD_MAX)

**Returns**: Length of formatted string (typically 1)

**Output Format**: `?`

**Behavior**: Requests machine status and position from ESP32

**Example**:
```c
char cmd[128];
fluidnc_format_status(cmd);
// Result: "?"
websocket_send_command(machine_id, cmd);
// Machine will respond with {"status":"Idle","pos":[...]}
```

---

#### `int fluidnc_parse_mpos(const char *response, float *x, float *y, float *z)`

**Purpose**: Parse position from FluidNC response

**Parameters**:
- `response`: Response string from machine
- `x`, `y`, `z`: Pointers to float variables to store results

**Returns**:
- `0` on success
- `-1` on parse error

**Supported Formats**:
- JSON: `{"pos":[10.0,20.0,30.0]}`
- Legacy: `MPos:10.0,20.0,30.0`

**Behavior**:
- Tries JSON format first
- Falls back to legacy format
- Extracts three float values
- Returns success only if all three values parsed

**Example**:
```c
float x, y, z;
const char *response = "{\"status\":\"Idle\",\"pos\":[10.0,20.0,5.0]}";

if (fluidnc_parse_mpos(response, &x, &y, &z) == 0) {
    printf("Position: X=%.3f Y=%.3f Z=%.3f\n", x, y, z);
} else {
    printf("Failed to parse position\n");
}
```

---

### Machine Config API

#### `int config_load(const char *filename, MachinesConfigList *config)`

**Purpose**: Load machine configuration from JSON file

**Parameters**:
- `filename`: Path to JSON file
- `config`: Pointer to MachinesConfigList struct to fill

**Returns**:
- `0` on success
- `-1` on error

**File Format**:
```json
{
  "machines": [
    {"id": 1, "ip": "192.168.1.100"},
    {"id": 2, "ip": "192.168.1.101"}
  ]
}
```

**Behavior**:
- Opens and reads file
- Parses JSON
- Fills config->machines array
- Sets config->count
- Logs each loaded machine

**Example**:
```c
MachinesConfigList config = {0};
if (config_load("machine_config.json", &config) == 0) {
    printf("Loaded %d machines\n", config.count);
    for (int i = 0; i < config.count; i++) {
        printf("  M%d: %s\n", config.machines[i].id, config.machines[i].ip_address);
    }
}
```

---

#### `int config_save(const char *filename, MachinesConfigList *config)`

**Purpose**: Save machine configuration to JSON file

**Parameters**:
- `filename`: Output file path
- `config`: Pointer to MachinesConfigList struct

**Returns**:
- `0` on success
- `-1` on error (can't open file)

**Behavior**:
- Creates JSON structure
- Adds all machines from config array
- Writes pretty-formatted JSON
- Closes file

**Example**:
```c
MachinesConfigList config = {0};
config_add_machine(&config, 1, "192.168.1.100");
config_add_machine(&config, 2, "192.168.1.101");
config_save("machine_config.json", &config);
```

---

#### `int config_add_machine(MachinesConfigList *config, int id, const char *ip)`

**Purpose**: Add machine to in-memory configuration

**Parameters**:
- `config`: Pointer to MachinesConfigList struct
- `id`: Machine ID (1-10)
- `ip`: IP address string

**Returns**:
- `0` on success
- `-1` on error (config full or NULL pointer)

**Behavior**:
- Adds machine to next available slot
- Increments config->count
- Stores ID and IP

**Example**:
```c
MachinesConfigList config = {0};
config_add_machine(&config, 1, "192.168.1.100");
config_add_machine(&config, 2, "192.168.1.101");
config_add_machine(&config, 3, "192.168.1.102");
config_save("machine_config.json", &config);
```

---

#### `const char* config_get_machine_ip(MachinesConfigList *config, int id)`

**Purpose**: Look up machine IP by ID

**Parameters**:
- `config`: Pointer to MachinesConfigList struct
- `id`: Machine ID to find

**Returns**:
- Pointer to IP string on success
- NULL if machine ID not found

**Example**:
```c
MachinesConfigList config = {0};
config_load("machine_config.json", &config);

const char *ip = config_get_machine_ip(&config, 1);
if (ip) {
    printf("Machine 1 is at %s\n", ip);
} else {
    printf("Machine 1 not configured\n");
}
```

---

## Global Variables

### WebSocket State

```c
extern SystemStateWS global_state_ws;
extern pthread_mutex_t ws_state_mutex;
extern int websocket_conectado;
```

**Access Pattern**:
```c
// Always protect access with mutex
pthread_mutex_lock(&ws_state_mutex);

// Check if machine is connected
if (global_state_ws.maquinas[0].connected) {
    float x = global_state_ws.maquinas[0].pos_x;
    printf("Machine 1 X position: %.3f\n", x);
}

pthread_mutex_unlock(&ws_state_mutex);
```

---

## Constants

```c
// From websocket_service.h
#define MAX_MAQUINAS 10          // Maximum machines
#define MAX_IP_LEN 16            // "192.168.1.100" + null
#define WS_PORT 81               // FluidNC WebSocket port

// From fluidnc_formatter.h
#define FLUIDNC_CMD_MAX 128      // Command buffer size
```

---

## Usage Examples

### Example 1: Send a jog command

```c
#include "websocket/websocket_service.h"
#include "websocket/fluidnc_formatter.h"

void jog_machine(int machine_id, char axis, float distance) {
    // Check if connected
    if (!global_state_ws.maquinas[machine_id - 1].connected) {
        printf("Machine %d not connected\n", machine_id);
        return;
    }

    // Format command
    char cmd[128];
    fluidnc_format_jog(axis, distance, 1000, cmd);

    // Send
    websocket_send_command(machine_id, cmd);
}
```

### Example 2: Add machine at runtime

```c
#include "websocket/websocket_service.h"

void add_machine_runtime(int id, const char *ip) {
    websocket_set_machine_ip(id, ip);
    websocket_connect_machine(id);
    websocket_save_config("machine_config.json");
}
```

### Example 3: Monitor machine status

```c
#include "websocket/websocket_service.h"

void print_machine_status(int id) {
    pthread_mutex_lock(&ws_state_mutex);

    MaquinaDataWS *m = &global_state_ws.maquinas[id - 1];
    
    printf("Machine %d:\n", id);
    printf("  IP: %s\n", m->ip_address);
    printf("  Connected: %s\n", m->connected ? "Yes" : "No");
    printf("  Status: %s\n", m->estado);
    printf("  Position: X=%.3f Y=%.3f Z=%.3f\n", m->pos_x, m->pos_y, m->pos_z);

    pthread_mutex_unlock(&ws_state_mutex);
}
```

### Example 4: Emergency stop all machines

```c
#include "websocket/websocket_service.h"
#include "websocket/fluidnc_formatter.h"

void emergency_stop_all(void) {
    char cmd[128];
    fluidnc_format_stop(cmd);

    for (int i = 1; i <= MAX_MAQUINAS; i++) {
        if (global_state_ws.maquinas[i - 1].connected) {
            websocket_send_command(i, cmd);
        }
    }
}
```

---

## Error Handling Patterns

### Pattern 1: Check connection before sending

```c
if (global_state_ws.maquinas[machine_id - 1].connected) {
    websocket_send_command(machine_id, cmd);
} else {
    printf("Machine %d is offline\n", machine_id);
    // Optionally trigger reconnect
    websocket_connect_machine(machine_id);
}
```

### Pattern 2: Wait for position update

```c
// Send status request
char cmd[128];
fluidnc_format_status(cmd);
websocket_send_command(machine_id, cmd);

// Wait for response (with timeout)
int timeout = 1000;  // 1 second
while (timeout-- > 0) {
    pthread_mutex_lock(&ws_state_mutex);
    float x = global_state_ws.maquinas[machine_id - 1].pos_x;
    pthread_mutex_unlock(&ws_state_mutex);
    
    if (x > 0) break;  // Got update
    usleep(1000);      // 1ms sleep
}
```

---

## Thread Safety Notes

1. **Always use mutex when accessing `global_state_ws`**
2. **Callback functions run in WebSocket thread context**
3. **UI thread accesses state with mutex protection**
4. **File I/O functions are blocking but thread-safe**

---

## Return Value Summary

| Function | Success | Error | Notes |
|----------|---------|-------|-------|
| `websocket_connect_machine()` | 0 | -1 | Async operation |
| `websocket_disconnect_machine()` | 0 | -1 | Sync close |
| `websocket_set_machine_ip()` | 0 | -1 | Mutex protected |
| `websocket_load_config()` | 0 | -1 | File I/O |
| `websocket_save_config()` | 0 | -1 | File I/O |
| `fluidnc_format_jog()` | len | - | Always success |
| `fluidnc_format_home()` | len | - | Always success |
| `fluidnc_format_stop()` | len | - | Always success |
| `fluidnc_format_status()` | len | - | Always success |
| `fluidnc_parse_mpos()` | 0 | -1 | Parse error |
| `config_load()` | 0 | -1 | File/parse error |
| `config_save()` | 0 | -1 | File write error |
| `config_add_machine()` | 0 | -1 | Array full |

---

## Performance Considerations

- **Command send**: Non-blocking, ~1ms per write
- **Response parsing**: Synchronous in callback, <1ms
- **Config load**: Blocking, ~10ms for 10 machines
- **Auto-reconnect**: Every 6 seconds, non-blocking
- **Memory**: ~2KB per machine * 10 = ~20KB total

---

This API reference provides complete documentation for integrating WebSocket + FluidNC functionality into the CNC HMI system.
