// test code from https://www.geeksforgeeks.org/socket-programming-cc/
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#define PORT 9993 
int main(int argc, char const *argv[]) 
{ 
    int server_fd, new_socket, valread; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    char buffer[1024] = {0}; 
    char *hello = "Hello from server"; 
    char recordCommand[] = "record";    
   
    // Creating socket file descriptor 
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    address.sin_family = AF_INET; 
    address.sin_port = htons( PORT ); 
    
    if( inet_pton(AF_INET, "192.168.10.50",

    // Forcefully attaching socket to the port 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
     

    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }


 
    if (listen(server_fd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 


    printf("Waiting for TCP connection on port %d...\r\n",PORT);
    
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  
                       (socklen_t*)&addrlen))<0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 
   printf("Connected. Waiting for 'record' command...\r\n");

   valread = read( new_socket , buffer, 1024); 
   buffer[strcspn(buffer,"\r\n")] = '\0';  // stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
   while( strcmp(buffer,recordCommand) != 0 ){
     printf("buffer: <<%s>> key: <<%s>> 0x%02X\r\n",buffer,recordCommand,buffer[strlen(buffer)-1]);//buffer[strlen(buffer)-1]); 
     
    // not sure why this times out and returns zero after the first read() call, but we can manage that...
     do{
        valread = read( new_socket , buffer, 1024);
     } while(valread <= 0); 
     buffer[strcspn(buffer,"\r\n")] = '\0';  // stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input...
 
   } 
   printf("Accepted: %s\r\n",buffer ); 
   printf("Command accepted!\r\n");
      
    return 0; 
} 

