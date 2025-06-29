/*
 *  RPLIDAR
 *  Ultra Simple Data Grabber Demo App
 *
 *  Copyright (c) 2009 - 2014 RoboPeak Team
 *  http://www.robopeak.com
 *  Copyright (c) 2014 - 2019 Shanghai Slamtec Co., Ltd.
 *  http://www.slamtec.com
 *
 */
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <netdb.h>
#include <iostream>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <mutex>
#include <vector>

#include "network_to_serial_bridge/arduino_comms_serial.hpp"

void Terminate(const int &server_fd, const int &new_socket)
{
    close(new_socket);
    shutdown(server_fd, SHUT_RDWR);
}

bool InitServer(int &server_fd, int &new_socket, int port, std::string label)
{
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
  
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("socket failed");
        return false;
    }
  
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    {
        perror("setsockopt");
        return false;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
  
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) 
    {
        perror("bind failed");
        return false;
    }
    
    std::cout << label << " bind successfully" << std::endl;
    
    if (listen(server_fd, 3) < 0) 
    {
        perror("listen");
        return false;
    }
    std::cout << label << " listen successfully" << std::endl;

    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) 
    {
        perror("accept");
        return false;
    }
    std::cout << label << " accept successfully" << std::endl;

    return true;
}

std::vector<std::string> SplitBuffer(std::string s, std::string delimiter)
{
	std::vector<std::string> v;
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) 
    {
		token = s.substr(0, pos);
		v.push_back(token);
		s.erase(0, pos + delimiter.length());
	}

	if(!v.empty())
	{
		v.push_back(s);
	}

	return v;
}

bool ReadNetworkMessage(const int &socket, std::string &msg)
{
    try
    {
        char buffer[1024];
        bzero(buffer, 1024);
        int n = read(socket, buffer, 1024);
        printf("received %s\n", buffer);
        
        if(n<=0)
        {
            printf("error2\n");
            return false;
        }
        msg = buffer;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }

    return true;
}

bool SendNetworkMessage(const int &socket, const std::string &msg)
{
    try
    {
        int n = send(socket, msg.c_str(), msg.length(), MSG_NOSIGNAL);
        if (n < 0)
        {
            printf("error1\n");
            return false;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        return 0;
    }

    std::string network_port_number_s(argv[1]);
    std::string device(argv[2]);
    std::string baud_rate_s(argv[3]);
    int network_port_number = std::stoi(network_port_number_s);
    int baud_rate = std::stoi(baud_rate_s);
    ArduinoComms comms_;
    comms_.connect(device, baud_rate, 1000);

    std::cout << "network port: " << network_port_number << std::endl;
    std::cout << "device : " << device << std::endl;
    std::cout << "baud_rate : " << baud_rate << std::endl;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // for(int i = 0; i < 10; i++)
    // {
    //     std::stringstream ss2;
    //     ss2 << "u " << 1.0 << ":" << 0.0 << ":" << 0.0 << ":" << 10.0 << "\r";
    //     comms_.send_msg(ss2.str());
    //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // }
    
    //test
    // while(1)
    // {
    //     // std::stringstream ss1;
    //     // ss1 << "u " << 0.0 << ":" << 0.0 << ":" << 0.0 << ":" << 10.0 << "\r";
    //     // comms_.send_msg(ss1.str());
    //     // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //     std::stringstream ss;
    //     ss << "m " << 0 << " " << 40 << "\r";
    //     comms_.send_msg(ss.str());
    //     std::this_thread::sleep_for(std::chrono::milliseconds(33));
    //     // std::string response = comms_.send_msg_get_response("f\r", true);
    //     // std::this_thread::sleep_for(std::chrono::milliseconds(15));
    //     // // response = comms_.send_msg_get_response("g\r", true);
    //     // // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // }

    while(1)
    {
        int server_fd, socket;
        InitServer(server_fd, socket, network_port_number, "");

        while(1)
        {
            std::string msg;
            if(!ReadNetworkMessage(socket, msg))
            {
                printf("error3\n");
                break;
            }

            if(msg.empty()==true)
            {
                printf("empty message\n");
                continue;
            }

            std::vector<std::string> v = SplitBuffer(msg, "|");        
            if(v[0].compare("send")==0)
            {
                comms_.send_msg(v[1]);
            }
            else if(v[0].compare("send_get_response")==0)
            {
                std::string response = comms_.send_msg_get_response(v[1], true);
                if(!SendNetworkMessage(socket, response))
                {
                    printf("error4\n");
                    break;
                }
            }
        }

        Terminate(server_fd, socket);
    }

    return 0;
}
