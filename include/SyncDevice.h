// SyncDevice class to manage each LAN device to be synchronized
// includes socket control info and provides a method for quickly
// issuing a TCP command for starting the remote device
//
// uses snippits of socket test code from:
// * https://www.geeksforgeeks.org/socket-programming-cc/
// * https://monoxid.net/c/socket-connection-timeout/
//
// Author: Mike Kokko
// Updated: 02-Nov-2023

#ifndef SYNCDEVICE_H
#define SYNCDEVICE_H

#include <unistd.h> 
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h> 
#include <netinet/in.h> 
#include <string.h>
#include <stdio>
#include <iostream> 

// enum for types of device
enum syncDeviceType{
	HyperDeck,
	Kinematics,	
	Other
};

// class for synchronizable devices
// TODO: should have a copy constructor to avoid issues in std::vector
class SyncDevice{	
private:
	std::string dev_name;
	std::string ip_address;
	unsigned int port;
	syncDeviceType type;
	bool readyToConnect = false;
	int socket_fd, status, res, opt, valread;
	struct sockaddr_in address; 
	bool deviceInitialized = false;
	struct timeval timeout;
	char buffer[1024] = {0}; // doesn't need delete[] in destructor because it is allocated here
	char recordCommand[512] = {0};
	char dev_command[1024] = {0};
	char dev_resp[1024] = {0};
	unsigned int recordCommandLength;
public:
	SyncDevice(std::string arg_dev_name, std::string arg_ip, unsigned int arg_port, syncDeviceType arg_type):
		dev_name{arg_dev_name},
		ip_address{arg_ip},
		port{arg_port},
		type{arg_type},
		readyToConnect{true}{}
	~SyncDevice(){
		close(socket_fd);
	};
	int Close(void);
	std::string getName(void);
	void PrintInfo(void);
	int sendRecordCommand(void);
	int SendHyperDeckConfigCmd(const char * msg);
	int Init(void);
};

#endif