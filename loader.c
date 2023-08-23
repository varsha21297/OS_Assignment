#include "loader.h"

Elf32_Ehdr *ehdr; //struct format
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
void load_and_run_elf(char** exe) { 
  
  fd = open(exe, O_RDONLY);

  // 1. Load entire binary content into the memory from the ELF file.

  off_t fileSize = lseek(fd, 0, SEEK_END);   //off_t basically used for file offsets
  lseek(fd, 0, SEEK_SET); // pointer at the beginning
  uint8_t *store_elf = (uint8_t*)malloc(fileSize); // allocate memory
  ssize_t bytes_read = read(fd, store_elf, fileSize); 

  ehdr = (Elf32_Ehdr*)store_elf;  //typecasting

  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c

  unsigned int p_off = (ehdr -> e_phoff);
  unsigned short p_num = (ehdr -> e_phnum);
  unsigned short p_size = (ehdr -> e_phentsize);
  unsigned int entry_point = (ehdr -> e_entry); 

  unit8_t *store_ph = (uint8_t*)malloc(p_num*p_size);
  phdr = (Elf32_Phdr*)store_ph;  //typecasting


  for (int i=p_off ; i<p_num ;) {

    i = i + p_size;
    printf(i);
  }

  printf("entry point %x", (ehdr -> e_entry));





  
  
  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  // 6. Call the "_start" method and print the value returned from the "_start"

  int result = _start();
  printf("User _start return value = %d\n",result);
}

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv[1]);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}
