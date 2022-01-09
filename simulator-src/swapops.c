#include <stdlib.h>

#include "swapops.h"
#include "util.h"

swap_queue_t swap_queue;

void swap_read(pte_t *pte, void *dst) {

    swap_info_t *info = swap_queue_find(&swap_queue, pte->swap);
    if (!info) {
        panic("Attempted to read an invalid swap entry.\nHINT: How do you check if a swap entry exists, and if it does not, what should you put in memory instead?");
    }
    memcpy(dst, info->page_data, PAGE_SIZE);
}

void swap_write(pte_t *pte, void *src) {

    swap_info_t *info = swap_queue_find(&swap_queue, pte->swap);
    if (!info) {
        info = create_entry();
        swap_queue_enqueue(&swap_queue, info);
        pte->swap = info->token;
    }
    memcpy(info->page_data, src, PAGE_SIZE);
}

void swap_free(pte_t *pte) {
    swap_entry_t swp_entry = pte->swap;
    if (!swap_queue_find(&swap_queue, swp_entry)) {
        panic("Attempted to free an invalid swap entry!");
    }
    swap_queue_dequeue(&swap_queue, pte->swap);
    pte->swap = 0;
}

void diskmap_init(pcb_t* process) {
    uint64_t *dm = (uint64_t *)calloc((size_t) NUM_PAGES, sizeof(swap_entry_t));

    if(dm == NULL)
    {
        panic("Disk map allocation failed due to memory issues\n");
    }

    process->diskmap = dm;
}

void diskmap_read(vpn_t vpn __attribute__((unused)), uint64_t* diskmap __attribute__((unused)), void* dst) {
    // Read out of the filesystem to get the program data (simulated by filling with 0s)
    memset(dst, (uint8_t)0, (size_t)PAGE_SIZE);
}

