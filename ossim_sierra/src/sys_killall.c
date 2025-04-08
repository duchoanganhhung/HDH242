/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "libmem.h"
#include "string.h"

struct pcb_t *current_process = NULL;  

int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    printf("debug\n");
    char proc_name[100] = {0};
    uint32_t data;

    //hardcode for demo only
    uint32_t memrg = regs->a1;
    //printf("Invalid memrg: %u\n", memrg);
    
    /* TODO: Get name of the target proc */
    //proc_name = libread..
    int i = 0;
    data = 0;
    while (i <= 99){
        libread(caller, memrg, i, &data);
        if (data == -1 || data == 0) {
            break;
        }
        proc_name[i++] = (char)data;
        //if(data == -1) proc_name[i]='\0';
    }
    proc_name[i]='\0';
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    /* TODO: Traverse proclist to terminate the proc
     *       stcmp to check the process match proc_name
     */
    

    /* TODO Maching and terminating 
     *       all processes with given
     *        name in var proc_name
     */

    return 0; 
}
