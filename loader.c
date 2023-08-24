#include "loader.h"

Elf32_Ehdr *ehdr;  // Global variables to store ELF header and program header
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
void load_and_run_elf(char** exe) {
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
    ehdr = (Elf32_Ehdr*)malloc(fileSize);
    read(fd, ehdr, fileSize);
    if (ehdr == NULL) {
        perror("Error allocating memory");
        close(fd);
        exit(1);
    }


    // Calculate the offset of the program headers
    unsigned int p_off = ehdr->e_phoff;

    // Calculate the number of program headers
    unsigned short p_num = ehdr->e_phnum;

    // Calculate the size of each program header
    unsigned short p_size = ehdr->e_phentsize;

    unsigned int entry_point = ehdr->e_entry;

    // Allocate memory to store program headers
    phdr = (Elf32_Phdr*)malloc(p_size);
    lseek(fd, ehdr->e_phoff, SEEK_SET); // pointer to program header


  unsigned int address;
  int offset;

   

  /*for (int i=0; i< ehdr->e_phnum; i++){
    if (phdr[i].p_type =="PT_LOAD"){
      address= phdr[i].p_vaddr;
      offset= phdr[i].p_offset;
      break;
    }
  }*/
  for (int i=0 ; i<p_num; i++ ) { 
    read(fd,phdr,p_num); //reading the program header table
      unsigned int type = phdr->p_type;
      if (type==PT_LOAD) { 
        if ((entry_point >= phdr->p_vaddr) && (entry_point <= phdr->p_vaddr + phdr->p_memsz)) {
          address= phdr->p_vaddr;
          offset= phdr->p_offset;
          //printf("%x\n",address);
          break;
            //found segment 
        }
      }
    } 
     

  void *virtual_mem= mmap(NULL, phdr->p_memsz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1 , 0);

  void* actual= ehdr->e_entry-(address+offset);
  unsigned int *ptr = &actual;
  memcpy(virtual_mem, ptr, sizeof(actual));


  typedef int (*StartFunc)();
    StartFunc _start = (StartFunc)actual;
  


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


int main(int argc, char** argv) {
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup(ehdr , phdr);
  return 0;
}
