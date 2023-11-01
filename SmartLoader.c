#include "loader.h"
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
void *virtual_mem;

void loader_cleanup() {
    free(ehdr);
    free(phdr);
    close(fd);
    if (virtual_mem != MAP_FAILED) {
        munmap(virtual_mem, phdr->p_memsz);
    }
}

void my_handler(int sig, siginfo_t *info, void *context) {
    if (sig == SIGSEGV) {
        printf("Segmentation fault caught!\n");
        void *faulty = info->si_addr;

        for (int i = 0; i < ehdr->e_phnum; i++) {
            Elf32_Phdr *segment = (Elf32_Phdr *)((char *)ehdr + ehdr->e_phoff + i * ehdr->e_phentsize);
            if (faulty >= (void *)segment->p_vaddr && faulty < (void *)(segment->p_vaddr + segment->p_memsz)) {
                size_t calc_mem = ((segment->p_memsz + 4096 - 1) / 4096) * 4096;
                virtual_mem = mmap(NULL, calc_mem, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (virtual_mem == MAP_FAILED) {
                    perror("Error allocating memory for segment");
                    loader_cleanup();
                    exit(1);
                }
                return;
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

    // Read ELF header
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
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

    // Calculate the offset of the program headers
    unsigned int p_off = ehdr->e_phoff;

    // Calculate the number of program headers
    unsigned short p_num = ehdr->e_phnum;

    // Calculate the size of each program header
    unsigned short p_size = ehdr->e_phentsize;

    // Find the PT_LOAD segment with entry point
    phdr = (Elf32_Phdr *)malloc(p_size);
    if (phdr == NULL) {
        perror("Error allocating memory for program header");
        loader_cleanup();
        return;
    }

    lseek(fd, p_off, SEEK_SET);

    void *actual = ehdr->e_entry;

    typedef int (*StartFunc)();
    StartFunc _start = (StartFunc)actual;

    virtual_mem = NULL;  // Initialize virtual_mem to NULL

    if (signal(SIGSEGV, my_handler) == SIG_ERR) {
        perror("Error handling SIGSEGV");
        loader_cleanup();
        return;
    }

    int result = _start();

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
