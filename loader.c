#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char* exe[]) { 
  
  fd = open(exe[1], O_RDONLY);
  off_t fileSize = lseek(fd, 0, SEEK_END);   //off_t basically used for file offsets
  lseek(fd, 0, SEEK_SET); // pointer at the beginning
  uint8_t *store_elf = (uint8_t*)malloc(fileSize); // allocate memory
  ssize_t bytes_read = read(fd, store_elf, fileSize);
  void *mapped_memory = mmap(NULL, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_memory == MAP_FAILED) {
        perror("mmap");
        close(fd);
    }

    // Access the ELF header
    ehdr = (Elf32_Ehdr *)mapped_memory;

    // Iterate through program headers
    phdr = (Elf32_Phdr *)(mapped_memory + ehdr->e_phoff);
    int found = 0;

    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *programHeader = phdr[i];

        if (programHeader->p_type == 1 /* PT_LOAD */) {
            if (ehdr->e_entry >= programHeader->p_vaddr &&
                ehdr->e_entry < programHeader->p_vaddr + programHeader->p_memsz) {
                found = 1;
                break;
            }
        }
    }
    void *segmentMemory = mmap(
        NULL,
        phdr.p_memsz,
        PROT_READ,
        MAP_PRIVATE | MAP_ANONYMOUS,
        0, 0
    );
    if (pread(fd, segmentMemory, phdr.p_memsz, phdr.p_offset) == -1) {
        perror("Error reading segment content");
        close(fd);
        munmap(segmentMemory, programHeader.p_memsz);
    }
    // Calculate the offset within the segment to the entry point
    uintptr_t entryOffset = ehdr.e_entry - programHeader.p_vaddr;

    // Navigate to the entry point within the loaded segment
    void (*entryPoint)() = (void (*)())((uintptr_t)segmentMemory + entryOffset);
    // Typecast the address to a function pointer
    void (*typeentry)() = (_start *)(uintptr_t)(segmentMemory + entryOffset);
    // Call the entry point function
    typeentry();



  // 1. Load entire binary content into the memory from the ELF file.
  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c
  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  // 6. Call the "_start" method and print the value returned from the "_start"
  int result = _start();
  printf("User _start return value = %d\n",result);
}

int main(int argc, char* argv[]) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}
