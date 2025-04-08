#include <stdio.h>
#include<stdlib.h>
#include "common.h"
#include "syscall.h"
#include "string.h"
#include "libmem.h"

extern struct pcb_t *current_process;
extern int __sys_killall(struct pcb_t *caller, struct sc_regs* regs);


int main() {
    //printf("BRUH\n");
    if (!current_process) {
        printf("[INFO] No active process. Creating a test process...\n");
        current_process = malloc(sizeof(struct pcb_t));
        
        memset(current_process, 0, sizeof(struct pcb_t));
        
        if (!current_process) {
            printf("[ERROR] Failed to allocate memory for current_process!\n");
            return 1;
        }
        else {
            printf("Process created\n");
        }
        current_process->pid = 1;  
    }

    struct sc_regs fake_regs;  

    //printf("BRUH5mak\n");
    uint32_t mem_region = liballoc(current_process, 10, 1);  
    printf("BRUH3\n");
    if (mem_region == 0) {  
        printf("[ERROR] Memory allocation failed!\n");
        return 1;
    }

    libwrite(current_process, mem_region, 0, 'm');
    libwrite(current_process, mem_region, 1, 'i');
    libwrite(current_process, mem_region, 2, 'n');
    libwrite(current_process, mem_region, 3, 't');
    libwrite(current_process, mem_region, 4, 't');
    libwrite(current_process, mem_region, 5, 'y');
    libwrite(current_process, mem_region, 6, '\0');  

    fake_regs.a1 = mem_region;

    printf("[INFO] Attempting to kill 'mintty'...\n");

    int result = __sys_killall(current_process, &fake_regs);

    if (result == 0) {
        printf("[SUCCESS] 'mintty' processes terminated.\n");
    } else {
        printf("[ERROR] Failed to kill 'mintty'.\n");
    }

    return 0;
}
