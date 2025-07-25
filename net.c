#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n (len) bytes from fd; returns true on success and false on failure. 
It may need to call the system call "read" multiple times to reach the given size len. 
*/
static bool nread(int fd, int len, uint8_t *buf) {
  //holds output of read call (-1 for fail)
  int read_action;
  //hold how much has been read 
  int already_read = 0;

  
  //until the entire length is read, continue to call read function, checking for failure, and updating how much has been read
  while(already_read< len){
    
    read_action = read(fd, buf + already_read, len- already_read); 
    
    if(read_action <= 0){ 
      return false;
    }
    already_read += read_action;
  }
  
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on failure 
It may need to call the system call "write" multiple times to reach the size len.
*/
static bool nwrite(int fd, int len, uint8_t *buf) {
  //holds output of write call (-1 for fail)
  int write_action;
  //hold how much has been wrote 
  int already_wrote = 0;
  
  
  //until the entire length is wrote, continue to call write function, checking for failure, and updating how much has been wrote
  while(already_wrote< len){

    write_action = write(fd, buf + already_wrote, len- already_wrote); 
    
    if(write_action <= 0){ 
      return false;
    }
    already_wrote += write_action;
    
  }
  
  return true;
}

/* Through this function call the client attempts to receive a packet from sd 
(i.e., receiving a response from the server.). It happens after the client previously 
forwarded a jbod operation call via a request message to the server.  
It returns true on success and false on failure. 
The values of the parameters (including op, ret, block) will be returned to the caller of this function: 

op - the address to store the jbod "opcode"  
ret - the address to store the return value of the server side calling the corresponding jbod_operation function.
block - holds the received block content if existing (e.g., when the op command is JBOD_READ_BLOCK)

In your implementation, you can read the packet header first (i.e., read HEADER_LEN bytes first), 
and then use the length field in the header to determine whether it is needed to read 
a block of data from the server. You may use the above nread function here.  
*/
static bool recv_packet(int sd, uint32_t *op, uint16_t *ret, uint8_t *block) {
  
  
  uint16_t new_len;
  //creates a buf to read into
  uint8_t buf[HEADER_LEN];

  bool read_action = nread(sd, HEADER_LEN, buf);

  //ensure read was successful
  if (read_action == false){
    return false;
  }

  //extract information from the buf for len, op and ret and store in variables
  memcpy(&new_len, buf, 2);
  memcpy(op, buf +2, 4);
  memcpy(ret, buf +6, 2);

  //convert the variable and update the pointer to op and update the lenth holder variable
  *op = ntohl(*op);
  new_len = ntohs(new_len);

  //if legnth is 264, then it also has a block attached, so read that as well if needed
  if(new_len == 264){
    
    
    if((!(nread(sd, JBOD_BLOCK_SIZE, block)))){ 
      return false;
    }

  }
  return true;
  

}



/* The client attempts to send a jbod request packet to sd (i.e., the server socket here); 
returns true on success and false on failure. 

op - the opcode. 
block- when the command is JBOD_WRITE_BLOCK, the block will contain data to write to the server jbod system;
otherwise it is NULL.

The above information (when applicable) has to be wrapped into a jbod request packet (format specified in readme).
You may call the above nwrite function to do the actual sending.  
*/
static bool send_packet(int sd, uint32_t op, uint8_t *block) {

  //extract what the command is
  jbod_cmd_t cmnd_val = (op >> 14);

  //holder variables for the converted len, ret, and op to be sent
  uint16_t new_len;
  uint16_t new_ret;
  uint32_t new_op;

  //holds how big the packet is (264 or 8 depending on if it is write or not)
  int block_len = 0;
  
  if(cmnd_val == JBOD_WRITE_BLOCK){
    block_len = 264;
    new_len = htons(264);
  }else{
    block_len = 8;
    new_len = htons(8);
  }

  //create a buf to store info
  uint8_t buf[block_len]; 

  //convert op and ret
  new_op = htonl(op);
  new_ret = htons(0);
  
  //create packet, but with addition of a 256 block at the end is it is a write command
  if(cmnd_val == JBOD_WRITE_BLOCK){
    memcpy(buf, &new_len, 2);
    memcpy(buf + 2, &new_op, 4);
    memcpy(buf + 6, &new_ret, 2);
    memcpy(buf + 8, block, JBOD_BLOCK_SIZE);
  }else{
    memcpy(buf, &new_len, 2);
    memcpy(buf + 2, &new_op, 4);
    memcpy(buf + 6, &new_ret, 2);
  }
  
  //write the information and return whether or not the write was successful
  bool write_attempt = nwrite(sd, block_len, buf);
  if(write_attempt ==false){
    return false;
  }else{
    return true;
  }
  
}



/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. 
 * this function will be invoked by tester to connect to the server at given ip and port.
 * you will not call it in mdadm.c
*/
bool jbod_connect(const char *ip, uint16_t port) {
  struct sockaddr_in caddr;
  // Setup the address information
  caddr.sin_family = AF_INET;
  caddr.sin_port = htons(port);

  //if statements to ensure connection is successfuly if fails at any point, return fail, if not, return true

  //converts a IPv4 address into the UNIX structure used for processing:
  if (inet_aton(ip, &caddr.sin_addr) == 0 ) {
    return(false);
  }

  //creates a file handle for use in communication
  cli_sd = socket(PF_INET, SOCK_STREAM, 0);
  if (cli_sd == -1) {
  return(false);
  }

  //connects the socket file descriptor to the specified address
  if ( connect(cli_sd, (const struct sockaddr *)&caddr, sizeof(caddr)) == -1 ) {
        return(false);
    } 
  
  return(true);
}




/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
  //from slides, closes connection
  close(cli_sd);
  cli_sd = -1;
}



/* sends the JBOD operation to the server (use the send_packet function) and receives 
(use the recv_packet function) and processes the response. 

The meaning of each parameter is the same as in the original jbod_operation function. 
return: 0 means success, -1 means failure.
*/
int jbod_client_operation(uint32_t op, uint8_t *block) {

  uint32_t new_op;
  uint16_t ret_val;


  //sends packet and checks if it was successful
  if(!send_packet(cli_sd, op, block)) {
    return -1;
  }

  //recieves packet and checks if it was successful
  if(!recv_packet(cli_sd, &new_op, &ret_val, block)) {
    return -1;
  }
  
  return ret_val;
}
