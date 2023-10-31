#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * Release memory and other cleanups
 */
void loader_cleanup() {
    free(ehdr);
    free(phdr);
    close(fd);
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char **exe) {
    // Open the ELF file
    fd = open(exe[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening ELF file");
        exit(1);
    }

    // Get the file size
    off_t fileSize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);  // Reset file offset to the beginning

    // Allocate memory to store the entire ELF file
    ehdr = (Elf32_Ehdr *)malloc(fileSize);
    if (ehdr == NULL) {
        perror("Error allocating memory for ELF header");
        close(fd);
        exit(1);
    }

    if (read(fd, ehdr, fileSize) != fileSize) {
        perror("Error reading ELF header");
        free(ehdr);
        close(fd);
        exit(1);
    }

    // Calculate the offset of the program headers
    unsigned int p_off = ehdr->e_phoff;

    // Calculate the number of program headers
    unsigned short p_num = ehdr->e_phnum;

    // Allocate memory to store program headers
    phdr = (Elf32_Phdr *)malloc(p_num * sizeof(Elf32_Phdr));
    if (phdr == NULL) {
        perror("Error allocating memory for program headers");
        free(ehdr);
        close(fd);
        exit(1);
    }

    // Read program headers
    if (lseek(fd, p_off, SEEK_SET) == -1) {
        perror("Error seeking to program headers");
        free(ehdr);
        free(phdr);
        close(fd);
        exit(1);
    }
    if (read(fd, phdr, p_num * sizeof(Elf32_Phdr)) != p_num * sizeof(Elf32_Phdr)) {
        perror("Error reading program headers");
        free(ehdr);
        free(phdr);
        close(fd);
        exit(1);
    }

    // Find the entry point segment
    unsigned int entry_point = ehdr->e_entry;
    unsigned int address = 0;
    int offset = 0;

    for (int i = 0; i < p_num; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            if (entry_point >= phdr[i].p_vaddr && entry_point < phdr[i].p_vaddr + phdr[i].p_memsz) {
                address = phdr[i].p_vaddr;
                offset = phdr[i].p_offset;
                break;
            }
        }
    }

    // Allocate memory for the entry point segment
    void *virtual_mem = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    if (virtual_mem == MAP_FAILED) {
        perror("Error allocating memory for the entry point segment");
        free(ehdr);
        free(phdr);
        close(fd);
        exit(1);
    }

    void *actual = (void *)((char *)ehdr + entry_point - (address + offset));

    // Copy the segment of PT_LOAD
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking to the entry point segment");
        free(ehdr);
        free(phdr);
        close(fd);
        munmap(virtual_mem, phdr->p_memsz);
        exit(1);
    }

    if (read(fd, virtual_mem, phdr->p_filesz) != phdr->p_filesz) {
        perror("Error reading the entry point segment");
        free(ehdr);
        free(phdr);
        close(fd);
        munmap(virtual_mem, phdr->p_memsz);
        exit(1);
    }

    // Define and call the _start function
    typedef int (*StartFunc)();
    StartFunc _start = (StartFunc)actual;
    int result = _start();

    printf("User _start return value = %d\n", result);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }
    // 1. Carry out necessary checks on the input ELF file
    // 2. Pass it to the loader for carrying out the loading/execution
    load_and_run_elf(argv);
    // 3. Invoke the cleanup routine inside the loader
    loader_cleanup(ehdr, phdr);
    return 0;
}
