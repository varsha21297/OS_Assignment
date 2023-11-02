#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <elf.h>
#include <signal.h>
#include <string.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr; 

int fd;
int page_faults = 0;
int page_allocations = 0;
long internal_fragmentation = 0;
unsigned char *allocated_pages = NULL; // Track page allocations
unsigned long total_allocated_memory = 0;
unsigned long total_memory_allocated = 0;

void loader_cleanup() {
    if (allocated_pages != NULL) {
        free(allocated_pages);
    }
    if (ehdr != NULL) {
        free(ehdr);
    }
    if (fd != -1) {
        close(fd);
    }
}

void my_handler(int sig, siginfo_t *info, void *context) {
    if (sig == SIGSEGV) {
        printf("Segmentation fault caught!\n");
        void *faulty = info->si_addr;
        unsigned int page_size = getpagesize();
        void *page_start = (void *)((uintptr_t)fault_addr & ~(page_size - 1));

        if (allocated_pages[(uintptr_t)page_start / page_size] == 0) {
            // This is a page fault

            // Calculate the offset of the program headers
            unsigned int p_off = ehdr->e_phoff;

            // Calculate the number of program headers
            unsigned short p_num = ehdr->e_phnum;

            // Calculate the size of each program header
            unsigned short p_size = ehdr->e_phentsize;

            // Calculate the segment offset
            for (int i = 0; i < p_num; i++) {
                lseek(fd, p_off + i * p_size, SEEK_SET);
                read(fd, phdr, p_size);

                if (phdr->p_type == PT_LOAD &&
                    (uintptr_t)page_start >= phdr->p_vaddr &&
                    (uintptr_t)page_start < phdr->p_vaddr + phdr->p_memsz) {

                
                    size_t calc_mem = ((phdr->p_memsz + page_size - 1) / page_size) * page_size;
                    void *virtual_mem = mmap(page_start, calc_mem, PROT_READ | PROT_WRITE | PROT_EXEC,
                                                MAP_PRIVATE | MAP_ANONYMOUS , -1, 0);

                    if (virtual_mem == MAP_FAILED) {
                        perror("Error mapping memory for segment");
                        exit(1);
                    }

                    // Track allocated memory
                    total_allocated_memory += calc_size;

                    // Copy segment contents
                    lseek(fd, phdr->p_offset, SEEK_SET);
                    read(fd, segment_memory, phdr->p_filesz);

                    page_faults++;
                    page_allocations++;
                    allocated_pages[(uintptr_t)page_start / page_size] = 1;
                }
            }
        }
    }
}

void load_and_run_elf(char *exe) {
    fd = open(exe, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (ehdr == NULL) {
        perror("Error allocating memory for ELF header");
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
    for (int i = 0; i < p_num; i++) {
        lseek(fd, poff + i * p_size, SEEK_SET);
        read(fd, phdr, p_size);

        if (phdr->p_type == PT_LOAD) {
            if (phdr->p_vaddr + phdr->p_memsz > max_address) {
                max_address = phdr->p_vaddr + phdr->p_memsz;
            }
        }
    }

    // Calculate the number of pages required for the address space
    size_t page_count = (size_t)(max_address / getpagesize()) + 1;

    // Allocate memory for tracking allocated pages
    allocated_pages = (unsigned char *)calloc(page_count, sizeof(unsigned char));

    if (allocated_pages == NULL) {
        perror("Error allocating memory for tracking pages");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = my_handler;
    sigaction(SIGSEGV, &sa, NULL);

    void *entrypoint = (void *)(ehdr->e_entry);

    typedef int (*StartFunc)();
    StartFunc _start = (StartFunc)entrypoint;

    int result = _start();

    // Calculate internal fragmentation based on allocated and used memory
    internal_fragmentation = total_allocated_memory - total_memory_allocated;

    loader_cleanup();

    printf("User _start return value = %d\n", result);
    printf("Page Faults: %d\n", page_faults);
    printf("Page Allocations: %d\n", page_allocations);
    printf("total allocated memory: %lu\n", total_allocated_memory);
    printf("total memory allocated: %lu\n",total_memory_allocated);
    printf("Internal Fragmentation: %.2f KB\n", (float)internal_fragmentation / 1024.0);
} 

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(argv[1]);

    return 0;
}