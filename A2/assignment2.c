#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> /*For mmap() function*/
#include <string.h> /*For memcpy function*/
#include <fcntl.h> /*For file descriptors*/

#define BUFFER_SIZE 10
#define OFFSET_BITS 8
#define OFFSET_MASK 255
#define PAGE_COUNT 256
#define PAGE_SIZE 256 // Size of a page in bytes == Size of a frame in bytes
#define FRAME_COUNT 128
#define MEMORY_SIZE PAGE_SIZE * PAGE_COUNT
#define PHYSICAL_MEMORY_SIZE PAGE_SIZE * FRAME_COUNT
#define TLB_SIZE 16

// struct for TLB entry
typedef struct pair {
  int page_number;
  int frame_number;
} TLBentry;

// struct for TLB
// idea from: https://medium.com/@charlesdobson/how-to-implement-a-simple-circular-buffer-in-c-34b7e945d30e
typedef struct circular_array {
  TLBentry array[TLB_SIZE];
  int write_index;
} TLB;

// search the TLB for an entry corresponding to a page number
int search_TLB(TLB* tlb, int page_number) {
  for (int i = 0; i < TLB_SIZE; i++) {
    TLBentry entry = tlb->array[i];
    // found
    if (entry.page_number == page_number) {
      return i;
    }
  }
  // not found
  return -1;
}

/* To add an entry to the TLB. Since TLB uses the FIFO policy, if
 * TLB is full, any entry added to the TLB replaces the oldest entry
 */
void TLB_Add(TLB* tlb, TLBentry tlbEntry) {
  tlb->array[tlb->write_index] = tlbEntry;
  (tlb->write_index)++;
  (tlb->write_index) %= TLB_SIZE;
}

/* removing this page’s entry from the TLB, 
 * shuffling the TLB contents to maintain the FIFO order, 
 * and adding the new page entry at the top of the TLB
 */
void TLB_Update(TLB* tlb, int page_number) {
  int oldEntryInd = search_TLB(tlb, page_number);
  for (int i = oldEntryInd; i < tlb->write_index; i++)
    tlb->array[i] = tlb->array[i + 1];
  TLBentry emptyEntry = {.page_number = -1, .frame_number = -1};
  tlb->array[tlb->write_index] = emptyEntry;
  (tlb->write_index)--;
  (tlb->write_index) %= TLB_SIZE;
}

int main() {
  // statistic variables
  int page_faults = 0;
  int tlb_hits = 0;
  int total_addresses = 0;

  // initialize page table
  int page_table[PAGE_COUNT];
  for (int i = 0; i < PAGE_COUNT; i++) {
    page_table[i] = -1;
  }

  // initialize TLB
  TLB *tlb;
  tlb = malloc(sizeof(TLB));
  for (int i = 0; i < TLB_SIZE; i++) {
    TLBentry emptyEntry = {.page_number = -1, .frame_number = -1};
    tlb->array[i] = emptyEntry;
  }
  tlb->write_index = 0;

  // map backing store to a memory region
  signed char *mmapfptr;
  int mmapfile_fd = open("BACKING_STORE.bin", O_RDONLY);
  mmapfptr = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, mmapfile_fd, 0);

  // initialize physical memory
  signed char *physical_memory_ptr = malloc(PHYSICAL_MEMORY_SIZE);
  int write_index = 0;

  // maintain a hashtable (array) to map frame num to page num for replacement in physical memory
  int frameNum_to_pageNum[FRAME_COUNT];
  for (int i = 0; i < FRAME_COUNT; i++) {
    frameNum_to_pageNum[i] = -1;
  }

  // open file
  FILE *fptr = fopen("addresses.txt", "r");
  char buff[BUFFER_SIZE];
  while (fgets(buff, BUFFER_SIZE, fptr) != NULL) {
    // compute page number and offset
    int logical_address = atoi(buff);
    int page_number = logical_address >> OFFSET_BITS;
    int offset = logical_address & OFFSET_MASK;
    printf("Virtual address: %d ", logical_address);
    total_addresses += 1;
    
    // search TLB first
    int entry_index = search_TLB(tlb, page_number);
    int frame_number, physical_address;
    
    if (entry_index != -1) {     
        /* found entry in TLB */
        tlb_hits++;
        frame_number = (tlb->array[entry_index]).frame_number;
    } else {
      /* NOT found entry in TLB */
      frame_number = page_table[page_number];
      if (frame_number == -1) {        
        /* page fault */
        page_faults++;
        // copy a 256-byte pages from the memory mapped file and copy it to an available 
        //    frame in phsical memory.
        // Note: conside pointer arithmetic
        memcpy(physical_memory_ptr + PAGE_SIZE*write_index, mmapfptr + PAGE_SIZE*page_number, PAGE_SIZE);

        /*
        Handling Page Faults 4:
        Note: when you replace a page in memory you will need to update two entries of the page table; 
          one corresponding to the page being replaced, in which case the entry is -1, 
          and the other corresponding to the new page that is brought into memory.
        */
        // update the entry for page being replaced
        int old_page_number = frameNum_to_pageNum[write_index];
        if (old_page_number != -1) { // if an empty frame does not exist
          // update page_table
          page_table[old_page_number] = -1;
          // update TLB
          if (search_TLB(tlb, old_page_number) != -1) {
            TLB_Update(tlb, old_page_number);
          }
        }
        // update the entry for new page
        frame_number = write_index;
        page_table[page_number] = frame_number;
        frameNum_to_pageNum[frame_number] = page_number;
        write_index++;
        write_index %= FRAME_COUNT;
      }
      // add TLB when TLB is not hit
      TLBentry newEntry = {.page_number = page_number, .frame_number = frame_number};
      TLB_Add(tlb, newEntry);
    }
    // print physical address and value
    physical_address = (frame_number << OFFSET_BITS) | offset;
    printf("Physical address = %d ", physical_address);
    signed char *value = (signed char *) (physical_memory_ptr + physical_address);
    printf("Value=%d \n", *value);
  }

  // output statistic result
  printf("Total addresses = %d \n", total_addresses);
  printf("Page_faults = %d \n", page_faults);
  printf("TLB Hits = %d \n", tlb_hits);
  
  // clear
  fclose(fptr);
  munmap(mmapfptr, MEMORY_SIZE);
  free(physical_memory_ptr);
  return 0;
}