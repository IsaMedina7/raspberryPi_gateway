#ifndef MACHINE_CONFIG_H
#define MACHINE_CONFIG_H

#define CONFIG_FILE "machine_config.json"
#define MAX_MACHINES 10
#define MAX_IP_LEN 16

typedef struct {
    int id;
    char ip_address[MAX_IP_LEN];
} MachineConfig;

typedef struct {
    MachineConfig machines[MAX_MACHINES];
    int count;
} MachinesConfigList;

// Load configuration from file
int config_load(const char *filename, MachinesConfigList *config);

// Save configuration to file
int config_save(const char *filename, MachinesConfigList *config);

// Add a machine to configuration
int config_add_machine(MachinesConfigList *config, int id, const char *ip);

// Get machine IP from configuration
const char* config_get_machine_ip(MachinesConfigList *config, int id);

#endif // MACHINE_CONFIG_H
