/* Author:    
   Date:
    */
    
    
    
/***
 *      ______ .___  ___. .______     _______.  ______              ____    __   __  
 *     /      ||   \/   | |   _  \   /       | /      |            |___ \  /_ | /_ | 
 *    |  ,----'|  \  /  | |  |_)  | |   (----`|  ,----'              __) |  | |  | | 
 *    |  |     |  |\/|  | |   ___/   \   \    |  |                  |__ <   | |  | | 
 *    |  `----.|  |  |  | |  |   .----)   |   |  `----.             ___) |  | |  | | 
 *     \______||__|  |__| | _|   |_______/     \______|            |____/   |_|  |_| 
 *                                                                                   
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cache.h"
#include "mdadm.h"
#include "util.h"
#include "jbod.h"
#include "net.h"

 int is_mounted = 0; // set to 0 if disks unmounted, set to 1 if disks mounted

//helper function to shift bytes to properly run JBOD operation
uint32_t byte_shifter(uint32_t a, uint32_t b, uint32_t c, uint32_t d){
  uint32_t retval = 0x0, tempa, tempb, tempc, tempd;

  tempa = a&0xff;
  tempb = (b&0xff) << 14;
  tempc = (c&0xff) << 20;
  tempd = (d&0xff) << 28;
  retval = tempa | tempb | tempc | tempd;


  return retval;
}

int mdadm_mount(void) {
  /* YOUR CODE */

  // create a 32 bit int and shift the value for mount to bits 14-19 (bits for command)
  uint32_t mnt = JBOD_MOUNT << 14;
  if(is_mounted == 0){ //only mount if disks are currently unmounted
    jbod_client_operation(mnt, 0); 
    is_mounted = 1;
    return 1;
  }
  else{
    //printf("hello1");
    return -1;
  }
}

int mdadm_unmount(void) {
  /* YOUR CODE */

  // create a 32 bit int and shift the value for unmount to bits 14-19 (bits for command)
  uint32_t mnt = JBOD_UNMOUNT << 14;
  if(is_mounted == 1){ //only unmount if disks are currently mounted
    jbod_client_operation(mnt, 0);
    is_mounted = 0;
    return 1;
  }
  else{
    //printf("hello2");
    return -1;
  }
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
    /* YOUR CODE */

  //tests to make sure that the function is not reading invalid parameters 
  if(is_mounted == 0){ //checks if disks are mounted
    //printf("hello3");
    return -1;
  }else if(len > 1024){ //ensures length of read is not greater than 1024 bytes
    //printf("hello4");
    return -1;
  }else if(addr >= (JBOD_NUM_DISKS*JBOD_DISK_SIZE)){ //ensures the start address is within the total scope of the disks
    //printf("hello5");
    return -1;
  }else if(addr + len >= (JBOD_NUM_DISKS*JBOD_DISK_SIZE)){ // ensures that the read won't end outside scopes of disks
    //printf("hello6");
    return -1;
  }else if(buf == NULL && len != 0){ //ensures that address is not NULL while length is non-zero
    //printf("hello7");
    return -1;
  }

  int place = addr; //keeps track of the current addr (updates as more is read)
  uint8_t temp_buf[256]; //creates a temperary buf array to read into
  int into_disk = 0; // holds the number of bytes into the current disk (past the start of the current disk)
  int first_block = 0; // reads either 0 or 1, keeps track if it is reading the first block of the address legnth
  int already_read = 0; // holds the number of bytes the have been read at any point in the program

  //loops through until the the program has read the entire input legnth
  while(place < (len + addr)){

  uint32_t current_disk = place / (JBOD_DISK_SIZE); //determines the current disk
  uint32_t disk_operation = byte_shifter(0, JBOD_SEEK_TO_DISK, 0, current_disk); //goes to the current disk   jbod_operation(disk_operation, NULL);
  into_disk = place % (JBOD_DISK_SIZE); //claculates the number of bytes past the start of the current disk
  int current_block = into_disk / (JBOD_BLOCK_SIZE); //calculates the current block
  int remainder = into_disk % (JBOD_BLOCK_SIZE); //calulcates the number of bytes into the current block the address is
  
// check if block being read is currently in cache
  int cache_index = -1;
  if (cache_enabled()) {
      cache_index = cache_lookup(current_disk, current_block, temp_buf);
  }
  //if the cache contains the block, then it copies the data from the cache into temp_buf
  if (cache_index == -1) {
    jbod_client_operation(disk_operation, NULL);
    uint32_t block_operation = byte_shifter(0, JBOD_SEEK_TO_BLOCK, current_block, 0); //goes to the current block
    jbod_client_operation(block_operation, NULL);
    uint32_t read_operation = byte_shifter(0, JBOD_READ_BLOCK, 0, 0);
    jbod_client_operation(read_operation, temp_buf);
    cache_insert(current_disk, current_block, temp_buf);
        }







  

  //need to increment place with how many bits were just read

  //tests to see if this is the first block of the reading 
  if (first_block == 0){
    //test for if the start address starts the the beginning or in the middle of the first block
    if (remainder == 0){
      //when starting at beginning of first block, now tests if the read will need to surpass the first block, or be within it
      //and simply reads the entire legnth of the block if so
      if((len) > JBOD_BLOCK_SIZE){
        memcpy(buf, &temp_buf[remainder], JBOD_BLOCK_SIZE);
        place = place + ((JBOD_BLOCK_SIZE));
        already_read = JBOD_BLOCK_SIZE;
      }else{
        memcpy(buf, &temp_buf[remainder], len);
        place = place + (len);
        already_read = (len);
      }
    }else{ 
      //if is within first block, caluclates the number of bytes left to rea din the block and reads those
      //tests two cases, the first is if it is reading to the end of the block
      if((len) > (JBOD_BLOCK_SIZE - remainder)){
      memcpy(buf, &temp_buf[remainder], (JBOD_BLOCK_SIZE)-remainder);
      place = place + ((JBOD_BLOCK_SIZE) - remainder);
      already_read = (JBOD_BLOCK_SIZE) - remainder;
      }else{ //the second is if it starts in the middle of the first blocka and also ends within the first block
      memcpy(buf, &temp_buf[remainder], len);
      place = place + (len);
      already_read = len;
      }
      
    }
    //sets first block to 1 to signify that any block after this is not the first block, and also meaning that all other block readings will start at the beginning of said block
    first_block = 1;
  }else{
      //now tests for 2 cases, if the read will be within the current block or exceeds that block
      if((len- already_read) > JBOD_BLOCK_SIZE){
        //again, simply reads the entire legnth of the block if it is going to go into another block
        memcpy(buf + already_read, &temp_buf[remainder], JBOD_BLOCK_SIZE);
        place = place + JBOD_BLOCK_SIZE; 
        already_read = already_read + JBOD_BLOCK_SIZE;
      }else{
        //if it will not go into another block, it reads the remaining bytes, finishing the loop
        memcpy(buf+ already_read, &temp_buf[remainder], len - already_read);
        place = place + (len - already_read);
        already_read = already_read + (len - already_read);
      }
  }
  }
  return len;
}


int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {
  if(is_mounted == 0){ //checks if disks are mounted
    //printf("hello8");
    return -1;
  }else if(len > 1024){ //ensures length of read is not greater than 1024 bytes
//printf("hello9");
    return -1;
  }else if(addr >= (JBOD_NUM_DISKS*JBOD_DISK_SIZE)){ //ensures the start address is within the total scope of the disks
//printf("hello10");
    return -1;
  }else if(addr + len > (JBOD_NUM_DISKS*JBOD_DISK_SIZE)){ // ensures that the read won't end outside scopes of disks
    //printf("hello11");
    return -1;
  }else if(buf == NULL && len != 0){ //ensures that address is not NULL while length is non-zero
    //printf("hello12");
    return -1;
  }else if(addr == 0 && len == 0 && buf == NULL){ //ensures that address and len are not 0 while buf is NULL
    return 0;
  }


  int place = addr; //keeps track of the current addr (updates as more is read)
  uint8_t temp_buf[256]; //creates a temperary buf array to read into
  int into_disk = 0; // holds the number of bytes into the current disk (past the start of the current disk)
  int first_block = 0; // reads either 0 or 1, keeps track if it is reading the first block of the address legnth
  int already_read = 0; // holds the number of bytes the have been read at any point in the program

  //loops through until the the program has written the entire input legnth
  while(place < (len + addr)){

  uint32_t current_disk = place / (JBOD_DISK_SIZE); //determines the current disk
  into_disk = place % (JBOD_DISK_SIZE); //claculates the number of bytes past the start of the current disk
  int current_block = into_disk / (JBOD_BLOCK_SIZE); //calculates the current block
  int remainder = into_disk % (JBOD_BLOCK_SIZE); //calulcates the number of bytes into the current block the address is
  
  //loopup the block in the cache

  int cache_index = -1;
  if (cache_enabled()) {
      cache_index = cache_lookup(current_disk, current_block, temp_buf);
  }
  
  //if the cache contains the block, then it copies the data from the cache into temp_buf
  if (cache_index == -1) {
    uint32_t disk_operation = byte_shifter(0, JBOD_SEEK_TO_DISK, 0, current_disk); //goes to the current disk   jbod_operation(disk_operation, NULL);
    jbod_client_operation(disk_operation, NULL);
    uint32_t block_operation = byte_shifter(0, JBOD_SEEK_TO_BLOCK, current_block, 0); //goes to the current block
    jbod_client_operation(block_operation, NULL);
    uint32_t read_operation = byte_shifter(0, JBOD_READ_BLOCK, 0, 0);
    jbod_client_operation(read_operation, temp_buf);
    cache_insert(current_disk, current_block, temp_buf);
        }


  





 if (first_block == 0){

    //test for if the start address starts the the beginning or in the middle of the first block
    if (remainder == 0){
      //when starting at beginning of first block, now tests if the write will need to surpass the first block, or be within it
      //and simply writes the entire legnth of the block if so
      if((len) > JBOD_BLOCK_SIZE){
        memcpy(temp_buf+remainder, buf, JBOD_BLOCK_SIZE - remainder);
        //ensures to re seek to block to offset the automatic advancment of the current block after a read
        uint32_t disk_operation = byte_shifter(0, JBOD_SEEK_TO_DISK, 0, current_disk);
        jbod_client_operation(disk_operation, NULL);
        uint32_t block_operation = byte_shifter(0, JBOD_SEEK_TO_BLOCK, current_block, 0);
        jbod_client_operation(block_operation, NULL);
        uint32_t write_operation = byte_shifter(0, JBOD_WRITE_BLOCK, 0, 0);
        jbod_client_operation(write_operation, temp_buf);
        place = place + ((JBOD_BLOCK_SIZE-remainder));
        already_read = JBOD_BLOCK_SIZE;
      }else{
        memcpy(temp_buf+remainder, buf, len);
        //ensures to re seek to block to offset the automatic advancment of the current block after a read
        uint32_t disk_operation = byte_shifter(0, JBOD_SEEK_TO_DISK, 0, current_disk);
        jbod_client_operation(disk_operation, NULL);
        uint32_t block_operation = byte_shifter(0, JBOD_SEEK_TO_BLOCK, current_block, 0);
        jbod_client_operation(block_operation, NULL);
        uint32_t write_operation = byte_shifter(0, JBOD_WRITE_BLOCK, 0, 0);
        jbod_client_operation(write_operation, temp_buf);
        place = place + (len);
        already_read = (len);
      }
    }else{ 
      //if is within first block, caluclates the number of bytes left to write in the block and writes those
      //tests two cases, the first is if the write goes past the end of the current block
      if((len) > (JBOD_BLOCK_SIZE - remainder)){
      memcpy(temp_buf+remainder, buf, (JBOD_BLOCK_SIZE)-remainder);
      //ensures to re seek to block to offset the automatic advancment of the current block after a read
      uint32_t disk_operation = byte_shifter(0, JBOD_SEEK_TO_DISK, 0, current_disk);
        jbod_client_operation(disk_operation, NULL);
        uint32_t block_operation = byte_shifter(0, JBOD_SEEK_TO_BLOCK, current_block, 0);
        jbod_client_operation(block_operation, NULL);
      uint32_t write_operation = byte_shifter(0, JBOD_WRITE_BLOCK, 0, 0);
      jbod_client_operation(write_operation, temp_buf);
      place = place + ((JBOD_BLOCK_SIZE) - remainder);
      already_read = (JBOD_BLOCK_SIZE) - remainder;
      }else{ //the second is if it starts in the middle of the first blocka and also ends within the first block
      memcpy(temp_buf+remainder, buf, len);
      //ensures to re seek to block to offset the automatic advancment of the current block after a read
      uint32_t disk_operation = byte_shifter(0, JBOD_SEEK_TO_DISK, 0, current_disk);
        jbod_client_operation(disk_operation, NULL);
        uint32_t block_operation = byte_shifter(0, JBOD_SEEK_TO_BLOCK, current_block, 0);
        jbod_client_operation(block_operation, NULL);
      uint32_t write_operation = byte_shifter(0, JBOD_WRITE_BLOCK, 0, 0);
      jbod_client_operation(write_operation, temp_buf);
      place = place + (len);
      already_read = len;
      }
    }
    //sets first block to 1 to signify that any block after this is not the first block, and also meaning that all other block writing will start at the beginning of said block
    first_block = 1;
  }else{
      //now tests for 2 cases, if the read will be within the current block or exceeds that block
      if((len- already_read) > JBOD_BLOCK_SIZE){
        //again, simply writes the entire legnth of the block if it is going to go into another block
        memcpy(temp_buf, buf+ already_read, JBOD_BLOCK_SIZE);
        //ensures to re seek to block to offset the automatic advancment of the current block after a writes
        uint32_t disk_operation = byte_shifter(0, JBOD_SEEK_TO_DISK, 0, current_disk);
        jbod_client_operation(disk_operation, NULL);
        uint32_t block_operation = byte_shifter(0, JBOD_SEEK_TO_BLOCK, current_block, 0);
        jbod_client_operation(block_operation, NULL);
      uint32_t write_operation = byte_shifter(0, JBOD_WRITE_BLOCK, 0, 0);
      jbod_client_operation(write_operation, temp_buf);
        place = place + JBOD_BLOCK_SIZE; 
        already_read = already_read + JBOD_BLOCK_SIZE;
      }else{
        //if it will not go into another block, it writes the remaining bytes, finishing the loop
        memcpy(temp_buf, buf + already_read, len - already_read);
        //ensures to re seek to block to offset the automatic advancment of the current block after a read
        uint32_t disk_operation = byte_shifter(0, JBOD_SEEK_TO_DISK, 0, current_disk);
        jbod_client_operation(disk_operation, NULL);
        uint32_t block_operation = byte_shifter(0, JBOD_SEEK_TO_BLOCK, current_block, 0);
        jbod_client_operation(block_operation, NULL);
      uint32_t write_operation = byte_shifter(0, JBOD_WRITE_BLOCK, 0, 0);
      jbod_client_operation(write_operation, temp_buf);
        place = place + (len - already_read);
        already_read = already_read + (len - already_read);
      }
  }
  //updates the information of the block in the cache after the information was written to it
  if (cache_enabled()) {
                cache_update(current_disk, current_block, temp_buf);
        }
  }
  return len;
}