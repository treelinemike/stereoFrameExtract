#!/usr/bin/env python3
# modified from https://realpython.com/python-sockets/#echo-client-and-server

import socket

HOST = '127.0.0.1'  # Standard loopback interface address (localhost)
PORT = 9993         # Port to listen on (non-privileged ports are > 1023)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	s.bind((HOST, PORT))
    print('Waiting for sync connection...');
    s.listen()
    conn, addr = s.accept()
    continue_flag = True
    with conn:
        print('Connected by', addr)
        print('Waiting for sync command...');
        while continue_flag:
            data = conn.recv(1024)
            data_str = data.decode().rstrip() 
            if data_str == 'record':
                print('Received: ', data_str)
                continue_flag = False