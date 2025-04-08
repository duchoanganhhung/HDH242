
/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c
 */

 #include "string.h"
 #include "mm.h"
 #include "syscall.h"
 #include "libmem.h"
 #include <stdlib.h>
 #include <stdio.h>
 #include <pthread.h>

#define MAX_PHYS_PAGES 1024
#define PAGING_PAGE_SIZE 4096
#define PAGING_VPN(addr)   ((addr) / PAGING_PAGE_SIZE)  // Virtual Page Number
#define PAGING_PGN(addr)   PAGING_VPN(addr)             // Alias if you're using PGN
#define PAGING_FPN(pte)    ((pte) & 0xFFFFF)             // Extract Frame Page Number (low 20 bits if 32-bit PTE)
#define PAGING_PTE(fpn)    ((fpn) & 0xFFFFF)             // Make a PTE from frame page number
#define PAGING_PAGE_ALIGNSZ(sz) (((sz) + PAGING_PAGE_SIZE - 1) / PAGING_PAGE_SIZE * PAGING_PAGE_SIZE)

 
 static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;
 static uint8_t page_bitmap[MAX_PHYS_PAGES] = {0}; // 0: free, 1: used

int alloc_page() {
    for (int i = 0; i < MAX_PHYS_PAGES; ++i) {
        if (page_bitmap[i] == 0) {
            page_bitmap[i] = 1;
            return i;  
        }
    }
    return -1; 
}

 /*enlist_vm_freerg_list - add new rg to freerg_list
  *@mm: memory region
  *@rg_elmt: new region
  *
  */
 int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
 {
   struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;
 
   if (rg_elmt->rg_start >= rg_elmt->rg_end)
     return -1;
 
   if (rg_node != NULL)
     rg_elmt->rg_next = rg_node;
 
   /* Enlist the new region */
   mm->mmap->vm_freerg_list = rg_elmt;
 
   return 0;
 }
 
 /*get_symrg_byid - get mem region by region ID
  *@mm: memory region
  *@rgid: region ID act as symbol index of variable
  *
  */
 struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
 {
   if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
     return NULL;
 
   return &mm->symrgtbl[rgid];
 }
 
 /*__alloc - allocate a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *@alloc_addr: address of allocated memory region
  *
  */
 int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
 {
   /*Allocate at the toproof */
   struct vm_rg_struct rgnode;
 
   // Lock the memory mutex at the beginning
   pthread_mutex_lock(&mmvm_lock);
 
   /* TODO: commit the vmaid */
 
   if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
   {
     caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
     caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
     *alloc_addr = rgnode.rg_start;
 
     pthread_mutex_unlock(&mmvm_lock);
     return 0;
   }
 
   /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/
 
   /* TODO retrive current vma if needed, current comment out due to compiler redundant warning*/
   /*Attempt to increate limit to get space */
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   if (cur_vma == NULL)
   {
     pthread_mutex_unlock(&mmvm_lock);
     return -1; // Invalid VMA
   }
 
   int inc_sz = PAGING_PAGE_ALIGNSZ(size);
   int inc_limit_ret;
 
   /* TODO retrive old_sbrk if needed, current comment out due to compiler redundant warning*/
   int old_sbrk = cur_vma->sbrk;
 
   /* TODO INCREASE THE LIMIT as inovking systemcall
    * sys_memap with SYSMEM_INC_OP
    */
   struct sc_regs regs;
   regs.a1 = vmaid;
   regs.a2 = inc_sz;
   regs.flags = SYSMEM_INC_OP;
 
   /* SYSCALL 17 sys_memmap */
   inc_limit_ret = syscall(caller, 17, &regs);
 
   // Failed to increase limit
   if (inc_limit_ret < 0)
   {
     pthread_mutex_unlock(&mmvm_lock);
     return -1;
   }
 
   /* TODO: commit the limit increment */
   cur_vma->sbrk += inc_sz;
 
   /* TODO: commit the allocation address
   // *alloc_addr = ...
   */
   *alloc_addr = old_sbrk;
    
   uint32_t vaddr = *alloc_addr;
uint32_t aligned_size = PAGING_PAGE_ALIGNSZ(size);
uint32_t num_pages = aligned_size / PAGING_PAGE_SIZE;

for (int i = 0; i < num_pages; i++) {
    uint32_t vpn = PAGING_VPN(vaddr + i * PAGING_PAGE_SIZE);
    uint32_t ppn = alloc_page();  // ← Make sure this function returns a physical frame
    caller->mm->pgd[vpn] = PAGING_PTE(ppn);  // ← Encodes into a valid PTE

    // Optional debug log
    printf("Mapped VPN %d -> PPN %d\n", vpn, ppn);
}
   // Update symbol region table
   caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
   caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size - 1;
 
   pthread_mutex_unlock(&mmvm_lock);
   return 0;
 }
 
 /*__free - remove a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __free(struct pcb_t *caller, int vmaid, int rgid)
 {
   // struct vm_rg_struct rgnode;
 
   // Dummy initialization for avoding compiler dummay warning
   // in incompleted TODO code rgnode will overwrite through implementing
   // the manipulation of rgid later
 
   if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
     return -1;
 
   /* TODO: Manage the collect freed region to freerg_list */
   // Lock the memory mutex at the beginning
   pthread_mutex_lock(&mmvm_lock);
 
   // Get the symbol region to be freed
   struct vm_rg_struct *rgnode = get_symrg_byid(caller->mm, rgid);
   if (rgnode == NULL || rgnode->rg_start == 0 || rgnode->rg_end == 0)
   {
     pthread_mutex_unlock(&mmvm_lock);
     return -1; // Invalid region
   }
 
   // Get the VMA where this region belongs
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   if (cur_vma == NULL)
   {
     pthread_mutex_unlock(&mmvm_lock);
     return -1; // Invalid VMA
   }
 
   // Create a new free region node to add to the free list
   struct vm_rg_struct *free_rg = malloc(sizeof(struct vm_rg_struct));
   if (free_rg == NULL)
   {
     pthread_mutex_unlock(&mmvm_lock);
     return -1; // Out of memory for metadata
   }
 
   // Copy the region information
   free_rg->rg_start = rgnode->rg_start;
   free_rg->rg_end = rgnode->rg_end;
   free_rg->rg_next = NULL;
 
   /*enlist the obsoleted memory region */
   enlist_vm_freerg_list(caller->mm, free_rg);
 
   // Clear the symbol table entry
   rgnode->rg_start = 0;
   rgnode->rg_end = 0;
 
   // Unlock memory mutex
   pthread_mutex_unlock(&mmvm_lock);
 
   return 0;
 }
 
 /*liballoc - PAGING-based allocate a region memory
  *@proc:  Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
 {
   /* TODO Implement allocation on vm area 0 */
   int addr;
 
   int alloc_ret = __alloc(proc, 0, reg_index, size, &addr);
   proc->regs[0] = addr;
   printf("===== PHYSICAL MEMORY AFTER ALLOCATING =====\n");
   printf("PID=%d - Region=%d - Address=%08X - Size=%d byte\n", proc->pid, reg_index, proc->regs[0], size);
 
   struct vm_area_struct *cur_vma = get_vma_by_num(proc->mm, 0);
 
   uint32_t start = proc->regs[0];
uint32_t end = start + size;
printf("DEBUG: cur_vma->vm_start = %d, vm_end = %d\n", cur_vma->vm_start, cur_vma->vm_end);

print_pgtbl(proc, start, end);

 
   /* By default using vmaid = 0 */
 
   return alloc_ret;
 }
 
 /*libfree - PAGING-based free a region memory
  *@proc: Process executing the instruction
  *@size: allocated size
  *@reg_index: memory region ID (used to identify variable in symbole table)
  */
 
 int libfree(struct pcb_t *proc, uint32_t reg_index)
 {
   /* TODO Implement free region */
   printf("===== PHYSICAL MEMORY AFTER DEALLOCATING =====\n");
   printf("PID=%d - Region=%d\n", proc->pid, reg_index);
   /* By default using vmaid = 0 */
   return __free(proc, 0, reg_index);
 }
 
 /*pg_getpage - get the page in ram
  *@mm: memory region
  *@pagenum: PGN
  *@framenum: return FPN
  *@caller: caller
  *
  */
 int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
 {
   uint32_t pte = mm->pgd[pgn];
 
   if (!PAGING_PAGE_PRESENT(pte))
   { /* Page is not online, make it actively living */
     int vicpgn, swpfpn;
     int vicfpn;
     uint32_t vicpte;
 
     // int tgtfpn = PAGING_PTE_SWP(pte);//the target frame storing our variable
 
     /* TODO: Play with your paging theory here */
     /* Find victim page */
     find_victim_page(caller->mm, &vicpgn);
 
     /* Get free frame in MEMSWP */
     MEMPHY_get_freefp(caller->active_mswp, &swpfpn);
 
     /* TODO: Implement swap frame from MEMRAM to MEMSWP and vice versa*/
     uint32_t *victim_pte = &mm->pgd[vicpgn];
     vicfpn = PAGING_FPN(*victim_pte);
 
     /* TODO copy victim frame to swap
      * SWP(vicfpn <--> swpfpn)
      * SYSCALL 17 sys_memmap
      * with operation SYSMEM_SWP_OP
      */
     struct sc_regs regs;
     regs.a1 = vicfpn;
     regs.a2 = swpfpn;
     regs.flags = SYSMEM_SWP_OP;
 
     /* SYSCALL 17 sys_memmap */
     syscall(caller, 17, &regs);
 
     /* TODO copy target frame form swap to mem
      * SWP(tgtfpn <--> vicfpn)
      * SYSCALL 17 sys_memmap
      * with operation SYSMEM_SWP_OP
      */
     /* TODO copy target frame form swap to mem
     //regs.a1 =...
     //regs.a2 =...
     //regs.a3 =..
     */
 
     int tgtfpn = PAGING_PTE_SWP(pte);
     regs.a1 = tgtfpn;
     regs.a2 = vicfpn;
     regs.flags = SYSMEM_SWP_OP;
 
     /* SYSCALL 17 sys_memmap */
     syscall(caller, 17, &regs);
 
     /* Update page table */
     // Update victim's page table entry
     pte_set_swap(victim_pte, 0, swpfpn);
     CLRBIT(*victim_pte, PAGING_PTE_PRESENT_MASK);
 
     /* Update its online status of the target page */
     // Update requested page table entry
     pte_set_fpn(&mm->pgd[pgn], vicfpn);
     PAGING_PTE_SET_PRESENT(mm->pgd[pgn]); // Mark it as present in memory
 
     // Update FIFO list
     enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
   }
 
   // Retrieve the frame number after ensuring it's in RAM
   *fpn = PAGING_FPN(mm->pgd[pgn]);
 
   return 0;
 }
 
 /*pg_getval - read value at given offset
  *@mm: memory region
  *@addr: virtual address to acess
  *@value: value
  *
  */
 int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
 {
   int pgn = PAGING_PGN(addr);
   int off = PAGING_OFFST(addr);
   int fpn;
 
   /* Get the page to MEMRAM, swap from MEMSWAP if needed */
   if (pg_getpage(mm, pgn, &fpn, caller) != 0)
     return -1; /* invalid page access */
 
   /* TODO
    *  MEMPHY_read(caller->mram, phyaddr, data);
    *  MEMPHY READ
    *  SYSCALL 17 sys_memmap with SYSMEM_IO_READ
    */
   // Compute physical address
   int phyaddr = (fpn * PAGING_PAGESZ) + off;
 
   struct sc_regs regs;
   regs.a1 = caller->mram;
   regs.a2 = data;
   regs.flags = SYSMEM_IO_READ;
 
   /* SYSCALL 17 sys_memmap */
   syscall(caller, 17, &regs);
 
   // Update data (? deprecated????)
   // data = (BYTE)
 
   return 0;
 }
 
 /*pg_setval - write value to given offset
  *@mm: memory region
  *@addr: virtual address to acess
  *@value: value
  *
  */
 int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
 {
   int pgn = PAGING_PGN(addr);
   int off = PAGING_OFFST(addr);
   int fpn;
 
   /* Get the page to MEMRAM, swap from MEMSWAP if needed */
   if (pg_getpage(mm, pgn, &fpn, caller) != 0)
     return -1; /* invalid page access */
 
   /* TODO
    *  MEMPHY_write(caller->mram, phyaddr, value);
    *  MEMPHY WRITE
    *  SYSCALL 17 sys_memmap with SYSMEM_IO_WRITE
    */
   int phyaddr = (fpn * PAGING_PAGESZ) + off;
 
   struct sc_regs regs;
   regs.a1 = caller->mram;
   regs.a2 = phyaddr;
   regs.a3 = &value;
   regs.flags = SYSMEM_IO_WRITE;
 
   /* SYSCALL 17 sys_memmap */
   syscall(caller, 17, &regs);
 
   // Update data (? deprecated????)
   // data = (BYTE)
 
   return 0;
 }
 
 /*__read - read value in region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@offset: offset to acess in memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
 {
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
     return -1;
 
   pg_getval(caller->mm, currg->rg_start + offset, data, caller);
 
   return 0;
 }
 
 /*libread - PAGING-based read a region memory */
 int libread(
     struct pcb_t *proc, // Process executing the instruction
     uint32_t source,    // Index of source register
     uint32_t offset,    // Source address = [source] + [offset]
     uint32_t *destination)
 {
   BYTE data;
   int val = __read(proc, 0, source, offset, &data);
 
   /* TODO update result of reading action*/
   *destination = (uint32_t)data;
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER READING =====\n");
   printf("read region=%d offset=%d value=%d\n", source, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   MEMPHY_dump(proc->mram);
 #endif
 
   return val;
 }
 
 /*__write - write a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@offset: offset to acess in memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *
  */
 int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
 {
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
 
   if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
     return -1;
 
   pg_setval(caller->mm, currg->rg_start + offset, value, caller);
 
   return 0;
 }
 
 /*libwrite - PAGING-based write a region memory */
 int libwrite(
     struct pcb_t *proc,   // Process executing the instruction
     BYTE data,            // Data to be wrttien into memory
     uint32_t destination, // Index of destination register
     uint32_t offset)
 {
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
   printf("write region=%d offset=%d value=%d\n", destination, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   MEMPHY_dump(proc->mram);
 #endif
 
   return __write(proc, 0, destination, offset, data);
 }
 
 /*free_pcb_memphy - collect all memphy of pcb
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@incpgnum: number of page
  */
 int free_pcb_memph(struct pcb_t *caller)
 {
   int pagenum, fpn;
   uint32_t pte;
 
   for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++) {
    pte = caller->mm->pgd[pagenum];
  
    if (PAGING_PAGE_PRESENT(pte)) {
      // Page is in RAM
      fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else if (pte & PAGING_PTE_SWAPPED_MASK) {
      // Page is swapped out
      fpn = PAGING_PTE_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);
    }
  }
 
   return 0;
 }
 
 /*find_victim_page - find victim page
  *@caller: caller
  *@pgn: return page number
  *
  */
 int find_victim_page(struct mm_struct *mm, int *retpgn)
 {
   struct pgn_t *pg = mm->fifo_pgn;
   if (pg == NULL)
   {
     return -1; // FIFO list empty
   }
 
   /* TODO: Implement the theorical mechanism to find the victim page */
   *retpgn = pg->pgn;
 
   // Update the FIFO to pop the first element
   mm->fifo_pgn = pg->pg_next;
 
   // Free the victim page (? WHY)
   free(pg);
 
   return 0;
 }
 
 /*get_free_vmrg_area - get a free vm region
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@size: allocated size
  *
  */
 int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
 {
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   /*if (!cur_vma) {
    printf("NULL CON ME NO ROI\n");
    fflush(stdout);
    return -1;
  }
  else {
    printf("ko null bro\n");
    fflush(stdout);
  }*/
   struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
 
   if (rgit == NULL)
     return -1;
 
   /* Probe unintialized newrg */
   newrg->rg_start = newrg->rg_end = -1;
 
   /* TODO Traverse on list of free vm region to find a fit space */
   while (rgit != NULL)
   {
     int region_size = rgit->rg_end - rgit->rg_start;
 
     // Check if size is enough
     if (region_size >= size)
     {
       // Update the region boundaries
       newrg->rg_start = rgit->rg_start;
       newrg->rg_end = newrg->rg_start + size;
 
       // Update free region's start
       rgit->rg_start = newrg->rg_end;
 
       return 0;
     }
 
     rgit = rgit->rg_next;
   }
 
   return -1;
 }
 
 // #endif 