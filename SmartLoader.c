#include "loader.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

void loader_cleanup() {
    free(ehdr);
    free(phdr);
    close(fd);
}

void my_handler(int sig, siginfo_t *info, void *context) {
    if (sig == SIGSEGV) {
        printf("Segmentation fault caught!\n");
        void *faulty = info->si_addr;

        for (int i = 0; i < ehdr->e_phnum; i++) {
            Elf32_Phdr *segment = (Elf32_Phdr *)((char *)ehdr + ehdr->e_phoff + i * ehdr->e_phentsize);
            if (faulty >= (void *)segment->p_vaddr && faulty < (void *)(segment->p_vaddr + segment->p_memsz)) {
                size_t calc_mem = ((segment->p_memsz + 4096 - 1) / 4096) * 4096;
                void *virtual_mem = mmap(NULL, calc_mem, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (virtual_mem == MAP_FAILED) {
                    perror("Error allocating memory for segment");
                    loader_cleanup();
                    exit(1);
                }
                memcpy(virtual_mem, (void *)segment->p_offset, segment->p_filesz);

                // Cast the virtual_mem as a function pointer and execute it
                int (*func)() = virtual_mem;
                int result = func();
                printf("User _start return value = %d\n", result);

                // Cleanup the memory
                munmap(virtual_mem, calc_mem);
                return;
            }
        }
    }
}

void load_and_run_elf(char **exe) {
    fd = open(exe[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr);
    if (ehdr == NULL) {
        perror("Error allocating memory for ELF header");
        close(fd);
        return;
    }

    if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        perror("Error reading ELF header");
        loader_cleanup();
        return;
    }

    unsigned int entry_point = ehdr->e_entry;

    // Set up a signal handler for SIGSEGV
    struct sigaction sa;
    sa.sa_sigaction = my_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, NULL);

    // Call the entry point function
    int (*func)() = (int (*)())entry_point;
    int result = func();
    printf("User _start return value = %d\n", result);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }
    load_and_run_elf(argv);
    loader_cleanup();
    return 0;
}
