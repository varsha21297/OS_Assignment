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
long total_internal_fragmentation = 0;
unsigned char *allocated_pages = NULL; // Track page allocations
unsigned long total_allocated_memory = 0;
unsigned long total_memory_allocated = 0;

void loader_cleanup() {
        free(ehdr);
        free(phdr);
        close(fd);
    
}

void my_handler(int sig, siginfo_t *info, void *context){ //function to handle SIGSEV signal
    if (sig == SIGSEGV) {
        printf("Segmentation fault caught!\n");
        void *faulty= info->si_addr;
        unsigned int page_size = getpagesize();

    // Calculate the starting address of the faulting page
    unsigned int page_start = (unsigned int)ehdr->e_entry & ~(page_size - 1);

    if (allocated_pages[(uintptr_t)page_start / page_size] == 0) { 

        // Calculate the offset of the program headers
        unsigned int p_off = ehdr->e_phoff;

        // Calculate the number of program headers
        unsigned short p_num = ehdr->e_phnum;

        // Calculate the size of each program header
        unsigned short p_size = ehdr->e_phentsize;


        // Calculate the segment offset
        for (int i = 0; i < p_num; i++) {
            lseek(fd, ehdr->e_phoff + i * ehdr->e_phentsize, SEEK_SET);
            read(fd, phdr, p_size);
            if ((page_start >= phdr->p_vaddr) && (page_start <= phdr->p_vaddr + phdr->p_memsz)) {

                size_t calc_mem= ((phdr->p_memsz +page_size-1) / page_size) * page_size;
                
                void *virtual_mem = mmap(page_start, calc_mem, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
                // int result = _start();
                // printf("User _start return value = %d\n", result);
                // // Cleanup whatever opened
                // munmap(virtual_mem, phdr->p_memsz);
                if (virtual_mem == MAP_FAILED) {
                    perror("Error allocating memory for segment");
                    exit(1);
                    //loader_cleanup();
                }

                // Track allocated memory
                total_allocated_memory += map_size;

                 // Read the segment content
                lseek(fd, phdr->p_offset, SEEK_SET);
                read(fd, virtual_mem, phdr->p_filesz);

                //found segment 
                total_page_faults++;
                total_page_allocations++;
                allocated_pages[(uintptr_t)page_start / page_size] = 1;

                return;
            }       
        }
    }
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
    // phdr = (Elf32_Phdr *)malloc(p_size);   
    // if (phdr == NULL) {
    //     perror("Error allocating memory for program header");
    //     loader_cleanup();
    //     return;
    // }

    // Install segmentation fault handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = my_handler;
    sigaction(SIGSEGV, &sa, NULL);


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

    printf("Page Faults: %d\n", total_page_faults);
    printf("Page Allocations: %d\n", total_page_allocations);
    printf("Internal Fragmentation: %d KB\n", total_internal_fragmentation / 1024);

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