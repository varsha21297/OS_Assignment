#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

void loader_cleanup() {
        free(ehdr);
        free(phdr);
        close(fd);
    
}

void my_handler(){ //function to handle SIGSEV signal
    printf("Segmentation fault\n");
    loader_cleanup();
    exit(0);
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
    phdr = (Elf32_Phdr *)malloc(p_size);   //CHANGED
    if (phdr == NULL) {
        perror("Error allocating memory for program header");
        loader_cleanup();
        return;
    }

    lseek(fd, p_off, SEEK_SET);

    unsigned int address;
    int offset;
    
    /*for (int i = 0; i < p_num; i++) {
    read(fd, phdr, p_size);
    unsigned int type = phdr -> p_type;
        if (type==PT_LOAD) { 
        if ((entry_point >= phdr->p_vaddr) && (entry_point <= phdr->p_vaddr + phdr->p_memsz)) {
          break;
            //found segment 
        }
      }
    }

    // Allocate memory using mmap
    void *virtual_mem = mmap(NULL, phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (virtual_mem == MAP_FAILED) {
    perror("Error mapping memory for segment");
    free(ehdr);
    free(phdr);
    close(fd);
    exit(1);
}

    // Copy segment content
    lseek(fd, phdr->p_offset, SEEK_SET);
    read(fd, virtual_mem, phdr->p_filesz);*/

    // Calculate entrypoint address within the loaded segment
    void *actual = /*virtual_mem +*/ (ehdr->e_entry /*- phdr->p_vaddr*/);

    // Define function pointer type for _start and use it to typecast function pointer properly
    typedef int (*StartFunc)();
    StartFunc _start = (StartFunc)actual;

    // Call the "_start" method and print the value returned from "_start"
    int result = _start();
    printf("User _start return value = %d\n", result);

    // Cleanup whatever opened
    //munmap(virtual_mem, phdr->p_memsz);
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