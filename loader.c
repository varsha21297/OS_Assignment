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
  ehdr= (Elf32_Ehdr*) store_elf;
  uint8_t *store_pdr = (uint8_t*)malloc(fileSize);
  phdr= (Elf32_Phdr*) store_pdr;
  int address;
  int offset;
  for (int i=0; i< ehdr->e_phnum; i++){
    if (phdr[i].p_type =="PT_LOAD"){
      address= phdr[i].p_vaddr;
      offset= phdr[i].p_offset;
      break;
    }
  }

  void* virtual_mem= mmap(NULL, phdr->p_memsz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
  int actual= ehdr->e_entry-(address+offset);
  memcpy(virtual_mem, actual, sizeof(actual));
  int actual1= (*int)actual;
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
