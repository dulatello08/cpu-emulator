//
// Created by Dulat S on 10/29/24.
//
#include "main.h"

#define MAX_LINE_LENGTH 256

// Helper function to trim leading/trailing whitespace
char *trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

// Helper function to parse page type from string
PageType parse_page_type(const char *type_str) {
    if (strcmp(type_str, "boot_sector") == 0) return BOOT_SECTOR;
    if (strcmp(type_str, "usable_memory") == 0) return USABLE_MEMORY;
    if (strcmp(type_str, "mmio_page") == 0) return MMIO_PAGE;
    if (strcmp(type_str, "flash") == 0) return FLASH;
    return UNKNOWN_TYPE;
}

// Main INI file parser function
int parse_ini_file(const char *filename, MemoryConfig *config) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    MemorySection *current_section = NULL;

    while (fgets(line, sizeof(line), file)) {
        char *trimmed_line = trim_whitespace(line);

        if (trimmed_line[0] == ';' || trimmed_line[0] == '#' || trimmed_line[0] == '\0') {
            // Skip comments and empty lines
            continue;
        }

        if (trimmed_line[0] == '[') {
            // Section header
            char *end = strchr(trimmed_line, ']');
            if (!end) {
                fprintf(stderr, "Malformed section header: %s\n", trimmed_line);
                fclose(file);
                return -1;
            }
            *end = '\0';
            current_section = &config->sections[config->section_count++];
            strncpy(current_section->section_name, trimmed_line + 1, sizeof(current_section->section_name) - 1);
            current_section->section_name[sizeof(current_section->section_name) - 1] = '\0';
            current_section->type = UNKNOWN_TYPE;
            current_section->page_count = 0;
            current_section->device[0] = '\0';
        } else if (current_section) {
            // Key-value pair within a section
            char *equals = strchr(trimmed_line, '=');
            if (!equals) {
                fprintf(stderr, "Malformed key-value pair: %s\n", trimmed_line);
                fclose(file);
                return -1;
            }
            *equals = '\0';
            char *key = trim_whitespace(trimmed_line);
            char *value = trim_whitespace(equals + 1);

            if (strcmp(key, "type") == 0) {
                current_section->type = parse_page_type(value);
            } else if (strcmp(key, "start_address") == 0) {
                current_section->start_address = strtoul(value, NULL, 0);
            } else if (strcmp(key, "page_count") == 0) {
                current_section->page_count = strtoul(value, NULL, 0);
            } else if (strcmp(key, "device") == 0) {
                strncpy(current_section->device, value, sizeof(current_section->device) - 1);
                current_section->device[sizeof(current_section->device) - 1] = '\0';
            } else {
                fprintf(stderr, "Unknown key: %s\n", key);
            }
        }
    }

    fclose(file);
    return 0;
}
