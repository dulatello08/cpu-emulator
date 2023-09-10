#include <stddef.h>
#include <inttypes.h>
#include "../main.h"

// Lookup table entry
typedef struct {
    const char *name;  // Field name
    size_t offset;     // Offset of the field in the structure
    size_t size;       // Size of the field
} FieldEntry;

// Lookup table for the fields of the memory_block structure
const FieldEntry memory_block_fields[] = {
        { "startAddress", offsetof(struct memory_block, startAddress), sizeof(uint16_t) },
        { "size", offsetof(struct memory_block, size), sizeof(uint16_t) },
};

// Lookup table for the fields of the MemoryMap structure
const FieldEntry memory_map_fields[] = {
        { "programMemory", offsetof(MemoryMap, programMemory), sizeof(struct memory_block) },
        { "usableMemory", offsetof(MemoryMap, usableMemory), sizeof(struct memory_block) },
        { "flagsBlock", offsetof(MemoryMap, flagsBlock), sizeof(struct memory_block) },
        { "stackMemory", offsetof(MemoryMap, stackMemory), sizeof(struct memory_block) },
        { "mmuControl", offsetof(MemoryMap, mmuControl), sizeof(struct memory_block) },
        { "peripheralControl", offsetof(MemoryMap, peripheralControl), sizeof(struct memory_block) },
        { "flashControl", offsetof(MemoryMap, flashControl), sizeof(struct memory_block) },
        { "currentFlashBlock", offsetof(MemoryMap, currentFlashBlock), sizeof(struct memory_block) },
};

// Lookup table for the fields of the CPUState structure
const FieldEntry cpu_state_fields[] = {
        { "mm", offsetof(CPUState, mm), sizeof(MemoryMap) },
        { "reg", offsetof(CPUState, reg), sizeof(uint8_t*) },
        { "pc", offsetof(CPUState, pc), sizeof(uint16_t*) },
        { "in_subroutine", offsetof(CPUState, in_subroutine), sizeof(uint8_t*) },
        { "memory", offsetof(CPUState, memory), sizeof(uint8_t*) },
        { "z_flag", offsetof(CPUState, z_flag), sizeof(bool) },
        { "v_flag", offsetof(CPUState, v_flag), sizeof(bool) },
        { "display", offsetof(CPUState, display), sizeof(char[LCD_WIDTH][LCD_HEIGHT]) },
};

void handle_command(int client_fd, const char *command, void *ptr, const FieldEntry *entry, const char *value);

// Function to handle a connection
void handle_connection(int client_fd, CPUState *state, uint8_t *shared_data_memory) {
    char buffer[1024];
    ssize_t numBytes;

    while((numBytes = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[numBytes] = '\0';
        char *trimmed_buffer = buffer;
        while (isspace(*trimmed_buffer)) {
            trimmed_buffer++;
        }
        size_t buffer_length = strlen(trimmed_buffer);
        while (buffer_length > 0 && isspace(trimmed_buffer[buffer_length - 1])) {
            trimmed_buffer[buffer_length - 1] = '\0';
            buffer_length--;
        }

        char *command = strtok(trimmed_buffer, " ");
        char *param = strtok(NULL, " ");
        char *value = strtok(NULL, " ");

        char *fieldNames[10];
        int numFieldNames = 0;
        char *fieldName = strtok(param, ".");
        while(fieldName != NULL && numFieldNames < 10) {
            fieldNames[numFieldNames++] = fieldName;
            fieldName = strtok(NULL, ".");
        }

        int indices[10];
        for(int i = 0; i < numFieldNames; i++) {
            int index = -1;
            char *bracketPos = strchr(fieldNames[i], '[');
            if (bracketPos != NULL) {
                *bracketPos = '\0';  // Terminate field name at bracket
                sscanf(bracketPos + 1, "%d", &index);  // Parse index
            }
            indices[i] = index;
        }

        const FieldEntry *entry = NULL;
        void *structPtr = state;
        const FieldEntry *tables[] = {cpu_state_fields, memory_map_fields, memory_block_fields};
        size_t tableSizes[] = {sizeof(cpu_state_fields), sizeof(memory_map_fields), sizeof(memory_block_fields)};

        for(int i = 0; i < numFieldNames; i++) {
            size_t numEntries = tableSizes[i] / sizeof(FieldEntry);

            entry = NULL;
            for(size_t j = 0; j < numEntries; j++) {
                if(strcmp(fieldNames[i], tables[i][j].name) == 0) {
                    entry = &tables[i][j];
                    break;
                }
            }

            if(entry == NULL) {
                sprintf(buffer, "Unknown field: %s", fieldNames[i]);
                write(client_fd, buffer, strlen(buffer));
                return;
            }
            printf("Struct ptr: %p\n", (void *)state);

            structPtr = (char*)structPtr + entry->offset;
            if (entry->size == sizeof(uint8_t*) && indices[i] != -1) {
                // special case for memory
                if (strcmp(entry->name, "memory") == 0) {
                    structPtr = shared_data_memory + indices[i];
                }
                // For something like registers:
                else {
                    printf("Start pointer: %p; Index: %d; Entry name: %s\n", (void *)(*((uint64_t *) structPtr)), indices[i], entry->name);

                    structPtr = &(((uint8_t *)(*((uint64_t *) structPtr)))[indices[i]]);

                    printf("Result pointer: %p\n", structPtr);
                }
            }
        }

        handle_command(client_fd, command, structPtr, entry, value);
    }
}

void handle_get_command(void *ptr, __attribute__((unused)) const FieldEntry *entry, char *buffer) {
    CPUState *state = (CPUState *)ptr;
    const char *cpu_state_format =
            "{\n"
            "  \"state\": \"%p\",\n"  // Use "state" as the key for the pointer to the state itself
            "  \"mm\": {\n"
            "    \"programMemory\": { \"startAddress\": %u, \"size\": %u },\n"
            "    \"usableMemory\": { \"startAddress\": %u, \"size\": %u },\n"
            "    \"flagsBlock\": { \"startAddress\": %u, \"size\": %u },\n"
            "    \"stackMemory\": { \"startAddress\": %u, \"size\": %u },\n"
            "    \"mmuControl\": { \"startAddress\": %u, \"size\": %u },\n"
            "    \"peripheralControl\": { \"startAddress\": %u, \"size\": %u },\n"
            "    \"flashControl\": { \"startAddress\": %u, \"size\": %u },\n"
            "    \"currentFlashBlock\": { \"startAddress\": %u, \"size\": %u }\n"
            "  },\n"
            "  \"reg\": \"%p\",\n"
            "  \"pc\": \"%04x\",\n"
            "  \"in_subroutine\": \"%02x\",\n"
            "  \"memory\": \"%p\",\n"
            "  \"z_flag\": %s,\n"
            "  \"v_flag\": %s,\n"
            "  \"display\": []\n"  // Display can be added if needed, left empty for simplicity
            "}";
    sprintf(buffer, cpu_state_format,
            // Use "state" as the key for the pointer to the state itself
            state,
            // MemoryMap fields
            state->mm.programMemory.startAddress, state->mm.programMemory.size,
            state->mm.usableMemory.startAddress, state->mm.usableMemory.size,
            state->mm.flagsBlock.startAddress, state->mm.flagsBlock.size,
            state->mm.stackMemory.startAddress, state->mm.stackMemory.size,
            state->mm.mmuControl.startAddress, state->mm.mmuControl.size,
            state->mm.peripheralControl.startAddress, state->mm.peripheralControl.size,
            state->mm.flashControl.startAddress, state->mm.flashControl.size,
            state->mm.currentFlashBlock.startAddress, state->mm.currentFlashBlock.size,
            // CPUState fields
            state->reg, *(state->pc), *(state->in_subroutine), state->memory,
            state->z_flag ? "true" : "false",
            state->v_flag ? "true" : "false");
}

void handle_set_command(void *ptr, const FieldEntry *entry, const char *value, char *buffer) {
    if(entry->size == sizeof(bool)) {
        bool val = (strcmp(value, "true") == 0);
        *((bool*)ptr) = val;
        sprintf(buffer, "Set boolean value to %s at memory address %p", val ? "true" : "false", ptr);
    } else if(entry->size == sizeof(uint8_t*)) {
        uint8_t val;
        char *endptr;
        val = strtoul(value, &endptr, 0);
        if(*endptr != '\0') {
            sprintf(buffer, "Error: invalid input \"%s\"", value);
        } else {
            *((uint8_t*)ptr) = val;
            sprintf(buffer, "Set %" PRIu8 " to memory address %p", val, ptr);
        }
    }
    // Add else if cases for other field sizes as needed

    // Default feedback message if none of the above conditions match
    if (strcmp(buffer, "") == 0) {
        strcpy(buffer, "Unable to set value.");
    }
    strcat(buffer, "\n"); // add trailing newline
}

void handle_command(int client_fd, const char *command, void *ptr, const FieldEntry *entry, const char *value) {
    char buffer[1024];

    if (command == NULL) {
        strcpy(buffer, "No command specified\n");
    } else if (strcmp(command, "GET") == 0) {
        printf("GET\n");
        handle_get_command(ptr, entry, buffer);
    } else if (strcmp(command, "SET") == 0) {
        if (value != NULL) {
            handle_set_command(ptr, entry, value, buffer);
        } else {
            strcpy(buffer, "Missing value for SET command\n");
        }
    } else {
        printf("%s\n", command);
        strcpy(buffer, "Invalid command\n");
    }

    write(client_fd, buffer, strlen(buffer));
}