#include "loader.h"
#include <signal.h>
#include <sys/stat.h>

struct sigaction sa;

// Global variables to store elf header and program header
Elf32_Ehdr *ehdr;
Elf32_Phdr phdr;
int fd;

// Initializing the variables to keep track of it.
int faultPages_ptr = 0;
int allocPages_ptr = 0;
long fragmnt_ptr = 0;
unsigned char *allocated_pages = NULL; // Track page allocations
unsigned long allocMem_ptr = 0;
unsigned long memory_ptr = 0;

// Release memory & other cleanups
void loader_cleanup() {
    free(ehdr);
    //free(&phdr);
    close(fd);
}

void my_handler(int sig, siginfo_t *info, void *context){
    // handle segmentation fault
    if (sig == SIGSEGV) {
        void *faulty = info->si_addr;
        unsigned int page_size = 4096
        // printf("Page size %d\n",page_size);
        void *page_start = (void *)((uintptr_t)faulty & ~(page_size - 1));
        // printf("fault address %p\n", faulty);
        if (allocated_pages[(uintptr_t)page_start / page_size] == 0) {
            // Calculate the offset of the program headers
            unsigned int p_off = ehdr->e_phoff;

            // Calculate the number of program headers
            unsigned short p_num = ehdr->e_phnum;

            // Calculate the size of each program header
            unsigned short p_size = ehdr->e_phentsize;

            int i = 0; // Initialize the loop counter
            while (i < p_num) {
                // Calculate the offset for the i-th Program Header entry
                lseek(fd, p_off + i * p_size, SEEK_SET);
                read(fd, &phdr, p_size);

                if (phdr.p_type == PT_LOAD &&
                    page_start >= phdr.p_vaddr &&
                    page_start < phdr.p_vaddr + phdr.p_memsz) {
                    printf("p starting address %x\n", phdr.p_vaddr);

                    // Calculate the size to mmap (round up to the page size)
                    int calc_mem = ((phdr.p_memsz + page_size - 1) / page_size) * page_size;
                    faultPages_ptr += calc_mem / 4096;

                    printf("segment size %d\n", phdr.p_memsz);
                    printf("map size %d\n", calc_mem);

                    void *virtual_mem = mmap(page_start, calc_mem, PROT_READ | PROT_WRITE | PROT_EXEC,
                                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

                    if (virtual_mem == MAP_FAILED) {
                        perror("Error mapping memory for segment");
                        exit(1);
                    }
                    read(fd, virtual_mem, phdr.p_filesz);
                    int fragmentation = calc_mem - phdr.p_memsz;
                    fragmnt_ptr += fragmentation;
                    printf("total internal fragmentation %ld\n", fragmnt_ptr);

                    // Track allocated memory
                    allocMem_ptr += calc_mem;

                    // Update the memory_ptr
                    memory_ptr += fragmentation;
                    printf("Result allocMem_ptr: %ld\n", allocMem_ptr);

                    // Copy segment contents
                    lseek(fd, phdr.p_offset, SEEK_SET);

                    if (read(fd, virtual_mem, phdr.p_filesz) == -1) {
                        perror("error read");
                        exit(1);
                    }

                    allocPages_ptr++;
                    allocated_pages[(uintptr_t)page_start / page_size] = 1;
                    printf("Result allocated_pages: %hhn\n", allocated_pages);
                    printf("page allocations: %d\n", allocPages_ptr);
                    printf("..............................................\n");

                    //munmap(virtual_mem,calc_mem);
                }

                // Increment the loop counter
                i++;
            }
        }
    }
}

void load_and_run_elf(char **exe) {
    fd = open(exe, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (ehdr == NULL) {
        perror("Failed to allocate memory for ELF header");
        loader_cleanup();
        exit(1);
    }

    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) == -1) {
        perror("Error reading ELF header");
        loader_cleanup();
        exit(1);
    }

    // Calculate the offset of the program headers
    unsigned int p_off = ehdr->e_phoff;

    // Calculate the number of program headers
    unsigned short p_num = ehdr->e_phnum;

    // Calculate the size of each program header
    unsigned short p_size = ehdr->e_phentsize;

    // Calculate the maximum address space size
    unsigned int max_address = 0;

    int i = 0;
    while (i < p_num) {
        // Calculate the offset for the i-th Program Header entry
        lseek(fd, p_off + i * p_size, SEEK_SET);

        // Read the Program Header entry into phdr
        if (read(fd, &phdr, p_size) == -1) {
            perror("error reading");
            exit(1);
        }

        if (phdr.p_type == PT_LOAD) {
            if (phdr.p_vaddr + phdr.p_memsz > max_address) {
                max_address = phdr.p_vaddr + phdr.p_memsz;
            }
        }

        i++;
    }

    // Calculate the number of pages required for the address space
    int page_count = (int)(max_address / getpagesize()) + 1;

    // Allocate memory for tracking allocated pages
    allocated_pages = (unsigned char *)calloc(page_count, sizeof(unsigned char));

    if (allocated_pages == NULL) {
        perror("Error allocating memory for tracking pages");
        exit(1);
    }

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = my_handler;
    sigaction(SIGSEGV, &sa, NULL);

    void *actual = (void *)(ehdr->e_entry);

    // Define function pointer type for _start and use it to typecast function pointer properly
    typedef int (*Start_Func)();
    Start_Func _start = (Start_Func)actual;

    int result = _start();

    loader_cleanup();
    printf("Result from _start: %d\n", result);
    printf("Page Faults: %d\n", faultPages_ptr);
    printf("Page Allocations: %d\n", allocPages_ptr);
    printf("Internal Fragmentation: %.2f KB\n", (float)fragmnt_ptr / 1024);
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