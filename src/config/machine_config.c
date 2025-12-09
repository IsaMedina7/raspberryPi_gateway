#include "machine_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>

int config_load(const char *filename, MachinesConfigList *config)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("[CONFIG] File not found: %s\n", filename);
        config->count = 0;
        return -1;
    }

    // Read file content
    char buffer[4096];
    size_t size = fread(buffer, 1, sizeof(buffer) - 1, f);
    fclose(f);
    
    if (size == 0) {
        config->count = 0;
        return -1;
    }

    buffer[size] = '\0';

    // Parse JSON
    struct json_object *parsed_json = json_tokener_parse(buffer);
    if (!parsed_json) {
        printf("[CONFIG] Failed to parse JSON\n");
        config->count = 0;
        return -1;
    }

    config->count = 0;
    struct json_object *machines_array;
    
    if (json_object_object_get_ex(parsed_json, "machines", &machines_array)) {
        int array_len = json_object_array_length(machines_array);
        
        for (int i = 0; i < array_len && i < MAX_MACHINES; i++) {
            struct json_object *machine_obj = json_object_array_get_idx(machines_array, i);
            struct json_object *id_obj, *ip_obj;
            
            if (json_object_object_get_ex(machine_obj, "id", &id_obj) &&
                json_object_object_get_ex(machine_obj, "ip", &ip_obj)) {
                
                int id = json_object_get_int(id_obj);
                const char *ip = json_object_get_string(ip_obj);
                
                config->machines[config->count].id = id;
                strncpy(config->machines[config->count].ip_address, ip, MAX_IP_LEN - 1);
                config->machines[config->count].ip_address[MAX_IP_LEN - 1] = '\0';
                
                config->count++;
                printf("[CONFIG] Loaded M%d: %s\n", id, ip);
            }
        }
    }

    json_object_put(parsed_json);
    return 0;
}

int config_save(const char *filename, MachinesConfigList *config)
{
    struct json_object *root = json_object_new_object();
    struct json_object *machines_array = json_object_new_array();

    for (int i = 0; i < config->count; i++) {
        struct json_object *machine_obj = json_object_new_object();
        json_object_object_add(machine_obj, "id", json_object_new_int(config->machines[i].id));
        json_object_object_add(machine_obj, "ip", json_object_new_string(config->machines[i].ip_address));
        json_object_array_add(machines_array, machine_obj);
    }

    json_object_object_add(root, "machines", machines_array);

    FILE *f = fopen(filename, "w");
    if (f) {
        fprintf(f, "%s\n", json_object_to_json_string_ext(root, JSON_C_TO_STRING_PRETTY));
        fclose(f);
        printf("[CONFIG] Saved to %s\n", filename);
    } else {
        printf("[CONFIG] Failed to open file for writing\n");
        json_object_put(root);
        return -1;
    }

    json_object_put(root);
    return 0;
}

int config_add_machine(MachinesConfigList *config, int id, const char *ip)
{
    if (!config || !ip || config->count >= MAX_MACHINES) {
        return -1;
    }

    config->machines[config->count].id = id;
    strncpy(config->machines[config->count].ip_address, ip, MAX_IP_LEN - 1);
    config->machines[config->count].ip_address[MAX_IP_LEN - 1] = '\0';
    config->count++;

    return 0;
}

const char* config_get_machine_ip(MachinesConfigList *config, int id)
{
    if (!config) return NULL;

    for (int i = 0; i < config->count; i++) {
        if (config->machines[i].id == id) {
            return config->machines[i].ip_address;
        }
    }

    return NULL;
}
