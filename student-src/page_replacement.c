#include "types.h"
#include "pagesim.h"
#include "paging.h"
#include "swapops.h"
#include "stats.h"
#include "util.h"

pfn_t select_victim_frame(void);


/*  --------------------------------- PROBLEM 7 --------------------------------------
    Checkout PDF section 7 for this problem

    Make a free frame for the system to use.

    You will first call the page replacement algorithm to identify an
    "available" frame in the system.

    In some cases, the replacement algorithm will return a frame that
    is in use by another page mapping. In these cases, you must "evict"
    the frame by using the frame table to find the original mapping and
    setting it to invalid. If the frame is dirty, write its data to swap
    and update the current process' diskmap.
 * ----------------------------------------------------------------------------------
 */
pfn_t free_frame(void) {
    pfn_t victim_pfn;

    /* Call your function to find a frame to use, either one that is
       unused or has been selected as a "victim" to take from another
       mapping. */
    victim_pfn = select_victim_frame();

    /*
     * If victim frame is currently mapped, we must evict it:
     *
     * 1) Look up the corresponding page table entry
     * 2) If the entry is dirty, write it to swap with swap_write()
     * 3) Mark the original page table entry as invalid
     * 4) Unmap the corresponding frame table entry
     *
     */
    if (frame_table[victim_pfn].mapped) {
        vpn_t victim_vpn = frame_table[victim_pfn].vpn;
        pcb_t *victim_pcb = frame_table[victim_pfn].process;
        pte_t *victim_pte = (pte_t*)(mem + victim_pcb -> saved_ptbr * PAGE_SIZE) + victim_vpn;
        if (victim_pte -> dirty == 1) {
            swap_write(victim_pte, mem + victim_pfn * PAGE_SIZE);
            stats.writebacks++;
        }
        victim_pte -> valid = 0;
        victim_pte -> dirty = 0;
        frame_table[victim_pfn].mapped = 0;
    }

    /* Return the pfn */
    return victim_pfn;
}



/*  --------------------------------- PROBLEM 9 --------------------------------------
    Checkout PDF section 7, 9, and 11 for this problem

    Finds a free physical frame. If none are available, uses either a FIFO,
    randomized or clocksweep algorithm to find a used frame for
    eviction.

    Return:
        The physical frame number of a free (or evictable) frame.

    HINTS: Use the global variables MEM_SIZE and PAGE_SIZE to calculate
    the number of entries in the frame table.
    ----------------------------------------------------------------------------------
*/
pfn_t fifo_pointer = 0;
pfn_t clocksweep_pointer = 0;
pfn_t select_victim_frame() {
    /* See if there are any free frames first */
    size_t num_entries = MEM_SIZE / PAGE_SIZE;
    for (size_t i = 0; i < num_entries; i++) {
        if (!frame_table[i].protected && !frame_table[i].mapped) {
            return i;
        }
    }

    if (replacement == RANDOM) {
        /* Play Russian Roulette to decide which frame to evict */
        pfn_t last_unprotected = NUM_FRAMES;
        for (pfn_t i = 0; i < num_entries; i++) {
            if (!frame_table[i].protected) {
                last_unprotected = i;
                if (prng_rand() % 2) {
                    return i;
                }
            }
        }
        /* If no victim found yet take the last unprotected frame
           seen */
        if (last_unprotected < NUM_FRAMES) {
            return last_unprotected;
        }
    } else if (replacement == CLOCKSWEEP) {
        /* Implement a clocksweep page replacement algorithm here */
        //pfn_t clocksweep_pointer = 0;
        for (pfn_t i = clocksweep_pointer; i < NUM_FRAMES; i++) {
            if (frame_table[i].protected == 0) {
                if (frame_table[i].referenced == 0) {
                    if (i == NUM_FRAMES - 1) {
                         clocksweep_pointer = 0;
                    } else {
                         clocksweep_pointer = i + 1;
                    }
                    return i;
                } else {
                    frame_table[i].referenced = 0;
                }
            }
            if (i == NUM_FRAMES - 1) {
                 i = 0;
            }
        }
    } else if (replacement == FIFO) {
        /* Implement a FIFO algorithm here */
        pfn_t returnVal = 0;
        pfn_t i = 0;
        while (i <= num_entries) {
            if (frame_table[fifo_pointer].protected == 0) {
                returnVal = fifo_pointer;
                fifo_pointer = (size_t) (fifo_pointer + 1) % num_entries;
                break;
            }
            fifo_pointer = (size_t) (fifo_pointer + 1) % num_entries;
            i++;
        }
        return returnVal;

    }

    /* If every frame is protected, give up. This should never happen
       on the traces we provide you. */
    panic("System ran out of memory\n");
    exit(1);
}
