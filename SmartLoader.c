#include "loader.h"
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
void* virtual_mem;
int total_page_faults = 0;
int total_page_allocations = 0;
size_t total_internal_fragmentation = 0;
size_t page_size = 4 * 1024;

void loader_cleanup() {
        free(ehdr);
        free(phdr);
        close(fd);
    
}

void my_handler(int sig, siginfo_t *info, void *context){ //function to handle SIGSEV signal
    if (sig == SIGSEGV) {
        printf("Segmentation fault caught!\n");
        void *faulty= info->sig_addr;

    // Calculate the starting address of the faulting page
    unsigned int page_start = (unsigned int)ehdr->e_entry & ~(page_size - 1);

    // Calculate the size of the page, rounding up to the nearest page_size
    size_t page_memory_size = (segment_size + page_size - 1) & ~(page_size - 1);

    // Allocate memory for the entire page using mmap
    void *page = mmap((void *)page_start, page_memory_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

    if (page == MAP_FAILED) {
        perror("Error mapping memory for page fault");
        loader_cleanup();
        exit(1);
    }

    total_page_faults++;
    total_page_allocations++;

    // Calculate internal fragmentation for this page allocation
    unsigned long allocated_size = page_size;
    if (page_size > allocated_size)
    {
        total_internal_fragmentation_kb += (page_size - allocated_size) / 1024;
    }


    //     ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    //     if (ehdr == NULL) {
    //         perror("Error allocating memory for ELF header");
    //         close(fd);
    //         return;
    //     }
        
    //     read(fd, ehdr, sizeof(Elf32_Ehdr));

    //     // Calculate the offset of the program headers
    //     unsigned int p_off = ehdr->e_phoff;

    //     // Calculate the number of program headers
    //     unsigned short p_num = ehdr->e_phnum;

    //     // Calculate the size of each program header
    //     unsigned short p_size = ehdr->e_phentsize;

    //     unsigned int entry_point = ehdr->e_entry;
    //     lseek(fd, p_off, SEEK_SET);
    //     for (int i = 0; i < p_num; i++) {
    //         read(fd, phdr, p_size);
    //         if ((faulty >= phdr->p_vaddr) && (faulty <= phdr->p_vaddr + phdr->p_memsz)) {
    //             size_t calc_mem= ((phdr->p_memsz +4096-1)/4096)*4096;
    //             void *virtual_mem = mmap(NULL, calc_mem, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //             int result = _start();
    //             printf("User _start return value = %d\n", result);
    //             // Cleanup whatever opened
    //             munmap(virtual_mem, phdr->p_memsz);
    //             if (virtual_mem == MAP_FAILED) {
    //                 perror("Error allocating memory for segment");
    //                 loader_cleanup();
    //                 return;
    //             }
    //             break;
    //             //found segment 
    //         }
        
    // }
    }
}

void load_and_run_elf(char **exe) {
    fd = open(exe, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    //Get the file size
    off_t fileSize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);  // Reset file offset to the beginning

    // Read ELF header
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (ehdr == NULL) {
        perror("Error allocating memory for ELF header");
        close(fd);
        return;
    }
    
    read(fd, ehdr, sizeof(Elf32_Ehdr));

    // Calculate the offset of the program headers
    unsigned int p_off = ehdr->e_phoff;

    // Calculate the number of program headers
    unsigned short p_num = ehdr->e_phnum;

    // Calculate the size of each program header
    unsigned short p_size = ehdr->e_phentsize;

    unsigned int entry_point = ehdr->e_entry;
  

    // Find the PT_LOAD segment with entrypoint
    phdr = (Elf32_Phdr *)malloc(p_size);   
    if (phdr == NULL) {
        perror("Error allocating memory for program header");
        loader_cleanup();
        return;
    }

    lseek(fd, p_off, SEEK_SET);

    unsigned int address;
    int offset;

    for (int i=0 ; i<p_num; i++ ) { 
    read(fd,phdr,p_num); //reading the program header table
    unsigned int type = phdr->p_type;
    if (type==PT_LOAD) { 
      if ((entry_point >= phdr->p_vaddr) && (entry_point <= phdr->p_vaddr + phdr->p_memsz)) {
        address= phdr->p_vaddr;
        offset= phdr->p_offset;
        //printf("%x\n",address);
        break;
          //found segment 
      }
    }
  } 

    // Calculate entrypoint address within the loaded segment
    void *actual = ehdr->e_entry;

    // Define function pointer type for _start and use it to typecast function pointer properly
    typedef int (*StartFunc)();
    StartFunc _start = (StartFunc)actual;

    // Call the "_start" method and print the value returned from "_start"
    int result = _start();
    if (signal(SIGSEGV, my_handler) == SIG_ERR) { //handle SIGSEGV signal
        perror("Error handling SIGSEGV");
        loader_cleanup();
        return;
    }
    printf("User _start return value = %d\n", result);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }
    load_and_run_elf(argv[1]);
    loader_cleanup();
    return 0;
}