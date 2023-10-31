// test code from:
// * https://www.geeksforgeeks.org/socket-programming-cc/
// * https://monoxid.net/c/socket-connection-timeout/
#include <arpa/inet.h>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <iostream> 

#define USE_HYPERDECK_LEFT true
#define USE_HYPERDECK_RIGHT true
#define USE_KINEMATICS false

// enum for types of device
enum syncDeviceType{
	HyperDeck,
	Kinematics,	
	Other
};

// 
class SyncDevice{	
private:
	int socket_fd, status, res, opt, valread;
	struct sockaddr_in address; 
	std::string ip_address;
	syncDeviceType type;
	bool readyToConnect = false;
	bool socketInitialized = false;
	bool deviceConfigured = false;
	unsigned int port;
	char buffer[1024] = {0}; // doesn't need delete[] in destructor because it is allocated here
	struct timeval timeout;
	char recordCommand[10] = "ping\n";
public:
	SyncDevice(std::string arg_ip, int arg_port, syncDeviceType arg_type){
		ip_address = arg_ip;
		port = arg_port;
		type = arg_type;
		readyToConnect = true;
		Init();
	}
	~SyncDevice(){
		close(socket_fd);
		// nothing to do here
	};
	void PrintInfo(void){
		std::cout << "SyncDevice Object" << std::endl;
		std::cout << "> IP Address: " << ip_address << std::endl;
		std::cout << "> Port: " << port << std::endl;
		return;
	}
	int Init(void){
		
		// make sure we're ready to connect
		if(!readyToConnect){
			std::cout << "Not ready to connect! Provide valid parameters." << std::
			endl;
			return -1;
		}
		
		// set timeout
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;
		
		// Creating socket file descriptor 
		std::cout << "creating socket FD" << std::endl;
		if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
		{ 
			perror("socket failed"); 
			exit(EXIT_FAILURE); 
		} 
		
		std::cout << "setting socket options" << std::endl;
		// get socket flags
		if ((opt = fcntl (socket_fd, F_GETFL, NULL)) < 0) {
			return -1;
		}
		if (fcntl (socket_fd, F_SETFL, opt | O_NONBLOCK) < 0) {
			return -1;
		}
		setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);
		
		address.sin_family = AF_INET; 
		address.sin_port = htons( port );
		
		
		// configure address
		std::cout << "configuring server address" << std::endl;
		if( inet_pton(AF_INET, "192.168.10.50", &address.sin_addr) <=0){
			printf("INVALID ADDRESS\n");
			return -1;
		} 

		// connect to server
		std::cout << "connecting to server" << std::endl;
		if(( status = connect(socket_fd, (struct sockaddr*)&address, sizeof(address))) < 0){
			if(errno == EINPROGRESS){
				std::cout << "Connection was in progress, so we will wait..." << std::endl;
				fd_set wait_set;

				// make file descriptor set with socket
				FD_ZERO (&wait_set);
				FD_SET (socket_fd, &wait_set);

				// wait (BLOCKING) for socket to be writable; return after given timeout
				res = select (socket_fd + 1, NULL, &wait_set, NULL, &timeout);
			}
		} else {
			res = 1;
		}

		// reset socket flags
		if (fcntl (socket_fd, F_SETFL, opt) < 0) {
			return -1;
		}

		// an error occured in connect or select
		if (res < 0) {
			std::cout << "could not connect to server" << std::endl;
			return -1;
		}
		// select timed out
		else if (res == 0) {
			errno = ETIMEDOUT;
			return 1;
		}
		// almost finished...
		else {
			socklen_t len = sizeof (opt);

			// check for errors in socket layer
			if (getsockopt (socket_fd, SOL_SOCKET, SO_ERROR, &opt, &len) < 0) {
				return -1;
			}

			// there was an error
			if (opt) {
				errno = opt;
				return -1;
			}
		}
		
		// done
		socketInitialized = true;
		return 0;
	}
	int Configure(void){
		
		if(!socketInitialized){
			std::cout << "Error: socket not initialized" << std::endl;
			return -1;
		}
		
		std::cout << "reading value" << std::endl;
		valread = read(socket_fd,buffer,1024);
		printf("valread: %d response: <%s>\n",valread,buffer);
		memset(buffer,0,1024);

		for( unsigned int i = 0; i<3; ++i){
			// send record message
			send(socket_fd,recordCommand,strlen(recordCommand),0);
			printf("send record message\n");
			
			// capture and display response
			valread = read(socket_fd,buffer,1024);
			printf("valread: %d response: <%s>\n",valread,buffer);
			memset(buffer,0,1024);
		}
		return 0;
	}
	int Start(void){
		return 0;
	}
	int Close(){
		close(socket_fd);
		return 0;
	}
};

// main
int main(int argc, char const *argv[]) 
{ 
	
	SyncDevice* hyperdeck_left = new SyncDevice("192.168.10.50",9993,HyperDeck);
	hyperdeck_left->Configure();
	
	SyncDevice* hyperdeck_right = new SyncDevice("192.168.10.60",9993,HyperDeck);
	hyperdeck_right->Configure();
	
	SyncDevice* kin_pc = new SyncDevice("192.168.10.70",9993,Kinematics);
	kin_pc->Configure();
	
	
	// close all sockets
	hyperdeck_left->Close();
	hyperdeck_right->Close();
	kin_pc->Close();
	
	return 0; 
}