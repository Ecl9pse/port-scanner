#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>

void scan_port_range(int, int, const char*, std::vector<int>&, std::mutex&);
void menu();
void thread_manager(int start, int end);
int optimize_max_threads(int, int);


int main(int argc, char* argv[]){

    int choice;
    int start_port;
    int end_port;



    while ((choice = getopt(argc, argv, "supa")) != -1){
        switch(choice){
            case 's':
                start_port = 0;
                end_port = 1023;
                thread_manager(start_port, end_port);
                break;
            case 'u':
                start_port = 1024;
                end_port = 49151;
                thread_manager(start_port, end_port);
                break;
            case 'p':
                start_port = 49152;
                end_port = 65535;
                thread_manager(start_port, end_port);
                break;
            case 'a':
                start_port = 0;
                end_port = 65535;
                thread_manager(start_port, end_port);
                break;
            default:
                menu();
                break;

        }
    }

    return 0;
}


void scan_port_range(int start, int end, const char* ip, std::vector<int>& open_ports, std::mutex& mtx){

    for (int port = start; port <= end; port++){
        int sock = socket(AF_INET, SOCK_STREAM, 0); //create a TCP socket using IPv4

        if (sock < 0){
            continue;
        }

        struct sockaddr_in target; //create target address of type sockaddr_in (struct that holds IP address, port, and address type)
        target.sin_addr.s_addr = inet_addr(ip); //set target IP address to local IP
        target.sin_family = AF_INET; //specify target has IPv4 address
        target.sin_port = htons(port); //set port to i passed from the calling for loop


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
}

void menu(){
    std::cout << "Enter flag to choose ports to scan:\n" <<
    "\t'-s' - Scan System Ports\n" <<
    "\t'-u' - Scan User Ports\n" <<
    "\t'-p' - Scan Process Ports\n" <<
    "\t'-a' - Scan All Ports\n";
}

int optimize_max_threads(int max_threads, int total_ports){



    if (max_threads == 0){
        max_threads = 16;
    }

    if (max_threads > 100){
        max_threads = 100;
    }

    if (max_threads > total_ports){
        max_threads = total_ports;
    }

    return max_threads;
}

void thread_manager(int start, int end){

    std::vector<int> open_ports; //vector to hold all found open ports
    std::vector<std::thread> threads; //vector that holds all threads that will scan ports
    std::mutex mtx; //mutex to keep integrity of open_ports vector while multithreading

    int total_ports = end - start + 1;

    int max_threads = std::thread::hardware_concurrency() * 4;
    max_threads = optimize_max_threads(max_threads, total_ports);

    const char* ip_to_check = "127.0.0.1";



    int interval_size = std::max(1, (total_ports / max_threads));

    int thread_start = start;

    //create a thread for each port 0-max_threads that will start scanning when it is created
    for (int thread_num = 0; thread_num < max_threads; thread_num++){

        int thread_end;

        if (thread_num == max_threads - 1){
            thread_end = end;
        }
        else{
            thread_end = thread_start + interval_size - 1;
        }

        threads.push_back(std::thread (scan_port_range, thread_start, thread_end, ip_to_check, std::ref(open_ports), std::ref(mtx)));

        thread_start = thread_end + 1;
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


}