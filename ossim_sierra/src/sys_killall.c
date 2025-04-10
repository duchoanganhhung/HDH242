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
    char proc_name[100];
    uint32_t data;

    //hardcode for demo only
    uint32_t memrg = regs->a1;
    
    /* TODO: Get name of the target proc */
    //proc_name = libread..
    //kill_process("sc2");
    /*int i = 0;
    data = 0;
    while(i <= 99){
        libread(caller, memrg, i, &data);
        proc_name[i]= data;
        if(data == -1) proc_name[i]='\0';
        i++;
    }
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);
    kill_process(proc_name);*/
    /* TODO: Traverse proclist to terminate the proc
     *       stcmp to check the process match proc_name
     */
    //caller->running_list
    //caller->mlq_ready_queu

    /* TODO Maching and terminating 
     *       all processes with given
     *        name in var proc_name
     */

    return 0; 
}

/*int __sys_killall(struct pcb_t *caller, struct sc_regs* regs)
{
    printf("debug\n");
    char proc_name[100] = {0};
    uint32_t data;

    hardcode for demo only
    uint32_t memrg = regs->a1;
    printf("Invalid memrg: %u\n", memrg);
    
    /* TODO: Get name of the target proc *
    //proc_name = libread..
    printf("Reading from memregion %u\n", memrg);

    int i = 0;
    data = 0;
    while (i < 100) {
        if (__read(caller, 0, memrg, i, &data) != 0) {
            printf("Memory read failed at index %d\n", i);
            break;
        }
        printf("mem[%d] = %u ('%c')\n", i, data, (char)data);
        if (data == 0 || data == (uint32_t)-1) break;
        proc_name[i++] = (char)data;
    }
    proc_name[i] = '\0';
    printf("The procname retrieved from memregionid %d is \"%s\"\n", memrg, proc_name);

    kill_process(proc_name);
    /* TODO: Traverse proclist to terminate the proc
     *       stcmp to check the process match proc_name
     *
    

    /* TODO Maching and terminating 
     *       all processes with given
     *        name in var proc_name
     *

    return 0; 
}*/
