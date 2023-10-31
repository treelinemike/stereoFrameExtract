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
#include <cxxopts.hpp>  // https://www.github.com/jarro2783/cxxopts
#include <yaml-cpp/yaml.h>

#define USE_HYPERDECK_LEFT true
#define USE_HYPERDECK_RIGHT true
#define USE_KINEMATICS false

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
	int socket_fd, status, res, opt, valread;
	struct sockaddr_in address; 
	std::string ip_address, dev_name;
	syncDeviceType type;
	bool readyToConnect = false;
	bool deviceInitialized = false;
	unsigned int port;
	struct timeval timeout;
	char buffer[1024] = {0}; // doesn't need delete[] in destructor because it is allocated here
	char recordCommand[512] = {0};
public:
	SyncDevice(std::string arg_dev_name, std::string arg_ip, int arg_port, syncDeviceType arg_type){
		dev_name = arg_dev_name;
		ip_address = arg_ip;
		port = arg_port;
		type = arg_type;
		readyToConnect = true;
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

			// there was an error
			if (opt) {
				errno = opt;
				std::cout << "error(" << errno << ")" << std::endl;
				return -1;
			}
		}
		
		// SOCKET CONNECTION IS CONFIGURED AND READY TO GO
		// NOW WE NEED TO CONFIGURE FOR THE SPECIFIC DEVICE TYPE
		char dev_command[1024] = {0};
		char dev_resp[1024] = {0};
		switch(type){
		case HyperDeck:
			// set record command for this device type
			strcpy(recordCommand,"record\n");	
			
			// read anything in buffer from initial connection
			// for HyperDeck this will be system info
			valread = read(socket_fd,buffer,1024);
			//printf("valread: %d response: <%s>\n",valread,buffer);
			memset(buffer,0,1024);  // reset buffer

			// send ping message
			strcpy(dev_command,"ping\n");
			send(socket_fd,dev_command,strlen(dev_command),0);
			valread = read(socket_fd,buffer,1024);			
			if(valread < 6){
				std::cout << "ERROR: not enough response chars received: <" << buffer << ">" << std::endl;
				return -1;
			}
			memset(dev_resp,0,1024);
			strncpy(dev_resp,buffer,6);
			if(strcmp(dev_resp,"200 ok") != 0){
				std::cout << "ERROR: bad response from device to ping: " << buffer << std::endl;
				return -1;
			}
			memset(buffer,0,1024);  // reset buffer
			
		case Kinematics:
			// set record command for this device type
			strcpy(recordCommand,"r\n");
			
			// read anything in buffer from initial connection
			// for Kinematics don't expect to find anything, this will time out
			valread = read(socket_fd,buffer,1024);
			
			break;
		case Other:
			strcpy(recordCommand,"record\n");
			break;
		default:
			std::cout << "Undefined initialization behavior" << std::endl;
			return -1;
		}
		
		// done with initialization
		deviceInitialized = true;
		return 0;
	}
	int Start(void){
		return 0;
	}
	int Close(void){
		close(socket_fd);
		return 0;
	}
	std::string getName(void){
		return dev_name;
	}
	void printRecordCommand(void){
		std::cout << recordCommand << std::endl;
	}
};

// main
int main(int argc, char const *argv[]) 
{ 

	// variables
	cxxopts::Options options("sync_start", "LAN-based device synchronization");
	YAML::Node config;
	std::string yamlfile_name, device_name, device_type_str, device_ip;
	unsigned int device_port;
	bool manual_start_flag, device_active;
	std::vector<SyncDevice> devices;
	syncDeviceType device_type;
	int retval;

	// PARSE COMMAND LINE OPTIONS
	try
	{
		options.add_options()
		("y,yamlconfig", "name of config YAML file (REQUIRED)", cxxopts::value<std::string>());
		auto cxxopts_result = options.parse(argc, argv);
		if (cxxopts_result.count("yamlconfig") != 1)
		{
			std::cout << options.help() << std::endl;
			return -1;
		}
		yamlfile_name = cxxopts_result["yamlconfig"].as<std::string>();
	}
	catch (const cxxopts::exceptions::exception& e)
	{
		std::cout << "Error parsing options: " << e.what() << std::endl;
		return -1;
	}
	
	// LOAD CONFIGURATION FILE
	try {
		// get input file
		config = YAML::LoadFile(yamlfile_name);

		// get manual start flag
		if (config["manual_start"]) {
			manual_start_flag = config["manual_start"].as<bool>();
		}
		else {
			manual_start_flag = true;
			std::cout << "YAML: no 'manual_start' key in file, assuming manual start" << std::endl;
		}

		// parse each clip definition
		if (!config["devices"]) {
			std::cout << "ERROR: no 'devices' key found in YAML config file" << std::endl;
			return -1;
		}
		YAML::Node yamldevices = config["devices"];
		for (YAML::const_iterator it = yamldevices.begin(); it != yamldevices.end(); ++it) {
			device_name = it->first.as<std::string>();
			
			// get the YAML node for this device description
			YAML::Node device_details = it->second;
			
			// skip this device if we're not intersted in synchronizing it (i.e. if active == false)
			device_active = device_details["active"].as<bool>();
			if(!device_active){
				continue;
			}
			
			// extract device info
			// TODO: add error checking
			device_type_str = device_details["type"].as<std::string>();
			device_ip = device_details["ip"].as<std::string>();
			device_port = device_details["port"].as<unsigned int>();
			
			// map device_type_str from std::string to enum
			// TODO: is there a better way to bind enum?
			if(device_type_str.compare("HyperDeck") == 0){
				device_type = HyperDeck;
			} else if(device_type_str.compare("Kinematics") == 0){
				device_type = Kinematics;
			} else if(device_type_str.compare("Other") == 0){
				device_type = Other;
			} else {
				std::cout << "Invalid device type: " << device_type_str << std::endl;
				return -1;
			}
			
			// create a device
			// constructor: SyncDevice(std::string arg_dev_name, std::string arg_ip, int arg_port, syncDeviceType arg_type)
			std::cout << "Ading " << device_ip << ":" << device_port << " as " << device_name << std::endl;
			SyncDevice single_device(device_name,device_ip,device_port,device_type); // on stack (not heap), will be automatically freed 
			devices.push_back(single_device); // object is *copied* into vector
		}

		// make sure we have some clip definitions in our vector
		if (!devices.size()) {
			std::cout << "ERROR: no valid devices found in YAML config file" << std::endl;
			return -1;
		}
	}
	catch (const YAML::Exception& e)
	{
		std::cout << "ERROR PARSING YAML CONFIGURATION: " << e.what() << std::endl;
		return -1;
	}

	// initialize each device
	std::cout << std::flush;
	for( auto &it : devices ){
		std::cout << "Initializing: " << it.getName() << "... " << std::flush;
		retval = it.Init();
		if( retval ){
			std::cout << "Error initializing " << it.getName() << std::endl;
			return -1;
		}
		std::cout << " done." << std::endl;
	}
	
	// wait for keypress if desired
	if(manual_start_flag){
		std::cout << "Press ENTER to synchronize devices" << std::endl;
		std::cin.get();
	}
	
	// send sync pulse(s)
	for( auto &it : devices ){
		std::cout << it.getName() << ": ";
		it.printRecordCommand();
	}
	
	/*
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
	*/
	
	// done
	std::cout << "done" << std::endl;
	return 0; 
}