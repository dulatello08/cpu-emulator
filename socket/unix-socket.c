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
        { "inSubroutine", offsetof(CPUState, inSubroutine), sizeof(uint8_t*) },
        { "memory", offsetof(CPUState, memory), sizeof(uint8_t*) },
        { "z_flag", offsetof(CPUState, z_flag), sizeof(bool) },
        { "v_flag", offsetof(CPUState, v_flag), sizeof(bool) },
        { "display", offsetof(CPUState, display), sizeof(char[LCD_WIDTH][LCD_HEIGHT]) },
};

// Function to handle a connection
void handle_connection(int client_fd, CPUState *state) {
    char buffer[1024];
    ssize_t numBytes;

    // Loop to handle requests from the client
    while((numBytes = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[numBytes] = '\0'; // Null-terminate the string
        char *command = strtok(buffer, " ");
        char *param = strtok(NULL, " ");
        char *value = strtok(NULL, " ");

        // Split the parameter into field names
        char *fieldNames[10];
        int numFieldNames = 0;
        char *fieldName = strtok(param, ".");
        while(fieldName != NULL && numFieldNames < 10) {
            fieldNames[numFieldNames++] = fieldName;
            fieldName = strtok(NULL, ".");
        }

        // Look for the field in the lookup tables
        const FieldEntry *entry = NULL;
        void *structPtr = state;
        for(int i = 0; i < numFieldNames; i++) {
            // Determine which lookup table to use
            const FieldEntry *table;
            size_t numEntries;
            if(i == 0) {
                table = cpu_state_fields;
                numEntries = sizeof(cpu_state_fields) / sizeof(FieldEntry);
            } else if(i == 1) {
                table = memory_map_fields;
                numEntries = sizeof(memory_map_fields) / sizeof(FieldEntry);
            } else {
                table = memory_block_fields;
                numEntries = sizeof(memory_block_fields) / sizeof(FieldEntry);
            }

            // Search the lookup table for the field name
            entry = NULL;
            for(size_t j = 0; j < numEntries; j++) {
                if(strcmp(fieldNames[i], table[j].name) == 0) {
                    entry = &table[j];
                    break;
                }
            }

            // If the field was not found, send an error message and break out of the loop
            if(entry == NULL) {
                sprintf(buffer, "Unknown field: %s", fieldNames[i]);
                break;
            }

            // Update the pointer to the structure for the next level of fields
            structPtr = (char*)structPtr + entry->offset;
        }

        // If the field was found, handle the command
        if(entry != NULL) {
            if(strcmp(command, "GET") == 0) {
                // Get the value of the field and format it as a string
                if(entry->size == sizeof(uint16_t)) {
                    sprintf(buffer, "%u", *(uint16_t*)structPtr);
                } else if(entry->size == sizeof(struct memory_block)) {
                    struct memory_block *mb = (struct memory_block*)structPtr;
                    sprintf(buffer, "{ \"startAddress\": %u, \"size\": %u }", mb->startAddress, mb->size);
                }
                // Add else if cases for other field sizes as needed

            } else if(strcmp(command, "SET") == 0 && value != NULL) {
                // Parse the value and set the field
                if(entry->size == sizeof(uint16_t)) {
                    // Support both decimal and hexadecimal input
                    if(strncmp(value, "0x", 2) == 0) {
                        sscanf(value, "%" SCNx16, (uint16_t*)structPtr);
                    } else {
                        sscanf(value, "%" SCNu16, (uint16_t*)structPtr);
                    }
                } else if(entry->size == sizeof(struct memory_block)) {
                    // Parse the value as a memory_block struct (this is just an example and may need to be adjusted depending on the actual format of the values)
                    struct memory_block *mb = (struct memory_block*)structPtr;
                    sscanf(value, "{ \"startAddress\": %" SCNu16 ", \"size\": %" SCNu16 " }", &mb->startAddress, &mb->size);
                }
                // Add else if cases for other field sizes as needed

                strcpy(buffer, "OK");
            } else {
                strcpy(buffer, "Missing value for SET command");
            }
        }

        // Send the response back to the client
        write(client_fd, buffer, strlen(buffer));
    }
}