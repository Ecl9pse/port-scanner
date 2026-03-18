#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>

void scan_port(int, const char*, std::vector<int>&, std::mutex&);

int main(int argc, char* argv[]){

    std::vector<int> open_ports; //vector to hold all found open ports
    std::vector<std::thread> threads; //vector that holds all threads that will scan ports
    std::mutex mtx; //mutex to keep integrity of open_ports vector while multithreading

    //local ip: 127.0.0.1
    //test ip: 8.8.8.8
    const char* ip_to_check = "8.8.8.8";

    //create a thread for each port 0-200 that will start scanning when it is created
    for (int i = 0; i <= 200; i++){
        threads.push_back(std::thread (scan_port, i, ip_to_check, std::ref(open_ports), std::ref(mtx)));
    }

    //wait for all threads to finish
    for (std::thread& t : threads){
        t.join();
    }

    //message if no open ports found
    if (open_ports.size() == 0){
        std::cout << "No open ports on " << ip_to_check << "\n";
    }

    //list open ports
    else{
        std::cout << "Open ports on " << ip_to_check << ":\n";

        for (int i = 0; i < open_ports.size(); i++){
            std::cout << open_ports[i] << "\n";
        }
    }

    return 0;
}


void scan_port(int port, const char* ip, std::vector<int>& open_ports, std::mutex& mtx){
    int sock = socket(AF_INET, SOCK_STREAM, 0); //create a TCP socket using IPv4

    sockaddr_in target{}; //create target address of type sockaddr_in (struct that holds IP address, port, and address type)
    target.sin_family = AF_INET; //specify target has IPv4 address

    if (sock < 0){
        std::cerr << "Failed to create socket\n";
        return;
    }


    target.sin_port = htons(port); //set port to i passed from the calling for loop
    inet_pton(AF_INET, ip, &target.sin_addr); //set IP address of target

    //set timeout interval to 200 ms
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    //connect returns 0 if successful and -1 if failed
    int result = connect(sock, (sockaddr*)&target, sizeof(target));

    //if successful connection, lock the open ports vector and add the open port to it then unlock for other threads afterwards
    if (result == 0){
        mtx.lock();
        open_ports.push_back(port);
        mtx.unlock();
    }

    close(sock); //close sock file descriptor
}