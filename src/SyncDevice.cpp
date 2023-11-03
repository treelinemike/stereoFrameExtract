#include "SyncDevice.h"

int SyncDevice::Close(void){
	close(socket_fd);
	return 0;
}

std::string SyncDevice::getName(void){
	return dev_name;
}

void SyncDevice::PrintInfo(void){
	std::cout << "SyncDevice Object" << std::endl;
	std::cout << "> IP Address: " << ip_address << std::endl;
	std::cout << "> Port: " << port << std::endl;
	return;
}
	
int SyncDevice::sendRecordCommand(void){
	if(deviceInitialized){
		send(socket_fd,recordCommand,recordCommandLength,0);
	} else {
		std::cout << "device not initialized." << std:: endl;
		return -1;
	}
	return 0;
}

int SyncDevice::SendHyperDeckConfigCmd(const char * msg){
	
	// send message
	send(socket_fd,msg,strlen(msg),0);
	valread = read(socket_fd,buffer,1024);			
	if(valread < 6){
		std::cout << "ERROR: not enough response chars received: <" << buffer << ">" << std::endl;
		return -1;
	}
	std::string response(buffer);
	if(response.substr(0,6).compare("200 ok") != 0){
		std::cout << "ERROR: bad response from device: " << buffer << std::endl;
		return -1;
	}
	memset(buffer,0,1024);  // reset buffer	
	return 0;
}

int SyncDevice::Init(void){
	
	// make sure we're ready to connect
	if(!readyToConnect){
		std::cout << "Not ready to connect! Provide valid parameters." << std::
		endl;
		return -1;
	}
	
	// set timeout
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	
	// Creating socket file descriptor 
	//std::cout << "creating socket FD" << std::endl;
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		perror("failed to create socket"); 
		return -1; 
	} 
	
	// get socket flags
	//std::cout << "setting socket options" << std::endl;
	if ((opt = fcntl (socket_fd, F_GETFL, NULL)) < 0) {
		std::cout << "could not get socket options" << std::endl;
		return -1;
	}
	if (fcntl (socket_fd, F_SETFL, opt | O_NONBLOCK) < 0) {
		std::cout << "could not set socket to nonblocking" << std::endl;
		return -1;
	}
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof timeout);		
	address.sin_family = AF_INET; 
	address.sin_port = htons( port );
	
	// configure address
	//std::cout << "configuring server address" << std::endl;
	if( inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr) <=0){
		printf("invalid address\n");
		return -1;
	} 
	// connect to server
	//std::cout << "connecting to server" << std::endl;
	if(( status = connect(socket_fd, (struct sockaddr*)&address, sizeof(address))) < 0){
		if(errno == EINPROGRESS){
			//std::cout << "Connection was in progress, so we will wait..." << std::endl;
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
		std::cout << "could not reset socket flags" << std::endl;
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
		std::cout << "select timed out" << std::endl;
		return 1;
	}
	// almost finished...
	else {
		socklen_t len = sizeof (opt);
		// check for errors in socket layer
		if (getsockopt (socket_fd, SOL_SOCKET, SO_ERROR, &opt, &len) < 0) {
			std::cout << "socket layer error" << std::endl;
			return -1;
		}
		// there was an different error
		if (opt) {
			errno = opt;
			std::cout << "error connecting to server (" << errno << ")" << std::endl;
			return -1;
		}
	}
		
	// SOCKET CONNECTION IS CONFIGURED AND READY TO GO
	// NOW WE NEED TO CONFIGURE FOR THE SPECIFIC DEVICE TYPE
	switch(type){
	case HyperDeck:
		// set record command for this device type
		strcpy(recordCommand,"record\n");
		recordCommandLength = strlen(recordCommand);			
		
		// read anything in buffer from initial connection
		// for HyperDeck this will be system info
		valread = read(socket_fd,buffer,1024);
		//printf("valread: %d response: <%s>\n",valread,buffer);
		memset(buffer,0,1024);  // reset buffer
		
		// send ping message
		if( SendHyperDeckConfigCmd("ping\n") != 0){
			std::cout << "ERROR: failed ping." << std::endl;
			return -1;
		}
		
		// enable remote
		if( SendHyperDeckConfigCmd("remote: enable: true\n") != 0){
			std::cout << "ERROR: QUITTING" << std::endl;
			return -1;
		}
		
		// select slot 1
		if( SendHyperDeckConfigCmd("slot select: slot id: 1\n") != 0){
			std::cout << "ERROR: QUITTING" << std::endl;
			return -1;
		}
		break;
		
	case Kinematics:
		// set record command for this device type
		strcpy(recordCommand,"r\n");
		recordCommandLength = strlen(recordCommand);
		break;
	
	case Other:
		strcpy(recordCommand,"record\n");
		recordCommandLength = strlen(recordCommand);
		break;
	
	default:
		std::cout << "Undefined initialization behavior" << std::endl;
		return -1;
	}
	
	// done with initialization
	deviceInitialized = true;
	return 0;
}
