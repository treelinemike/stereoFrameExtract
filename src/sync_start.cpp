// test code from https://www.geeksforgeeks.org/socket-programming-cc/
#include <arpa/inet.h>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#define PORT 9993 
int main(int argc, char const *argv[]) 
{ 
    int status;
    int client_fd, valread; 
    struct sockaddr_in address; 
    char buffer[1024] = {0}; 
    char recordCommand[] = "ping\n";    
   
    // Creating socket file descriptor 
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    address.sin_family = AF_INET; 
    address.sin_port = htons( PORT ); 
	
    // configure address
    if( inet_pton(AF_INET, "192.168.10.50", &address.sin_addr) <=0){
      printf("INVALID ADDRESS\n");
      return -1;
    } 

	
    // connect to server
    if(( status = connect(client_fd, (struct sockaddr*)&address, sizeof(address))) < 0){
       printf("CONNECTION FAILED\n");
       return -1;
    } 

	valread = read(client_fd,buffer,1024);
    printf("valread: %d response: %s\n",valread,buffer);
	memset(buffer,0,1024);

	for( unsigned int i = 0; i<3; ++i){
	// send record message
    send(client_fd,recordCommand,strlen(recordCommand),0);
    printf("send record message\n");
    
    // capture and display response
    valread = read(client_fd,buffer,1024);
    printf("valread: %d response: %s\n",valread,buffer);
	memset(buffer,0,1024);
    }
    return 0; 
} 
