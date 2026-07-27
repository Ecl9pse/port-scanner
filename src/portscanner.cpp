#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <algorithm>

void scan_port_range(int, int, const char *, std::vector<int> &, std::mutex &);
void menu();
void thread_manager(int start, int end);
int optimize_max_threads(int);
void display_ports(std::vector<int> &);

int main(int argc, char *argv[])
{

    int choice;
    int start_port;
    int end_port;

    while ((choice = getopt(argc, argv, "srpa")) != -1)
    {
        switch (choice)
        {
        case 's':
            start_port = 1;
            end_port = 1023;
            thread_manager(start_port, end_port);
            break;
        case 'r':
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
            start_port = 1;
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

// Menu Function
// Displays a meny of flags to specify which ports to scan when running the program
void menu()
{
    std::cout << "Enter flag to choose ports to scan:\n"
              << "\t'-s' - Scan System Ports\n"
              << "\t'-r' - Scan Registered Ports\n"
              << "\t'-p' - Scan Process Ports\n"
              << "\t'-a' - Scan All Ports\n";
}

// Thread Manager Function
// Enables multithreading by adding scan_port_range threads where the number of threads is limited based on user hardware
void thread_manager(int start, int end)
{

    std::vector<int> open_ports;      // vector to hold all found open ports
    std::vector<std::thread> threads; // vector that holds all threads that will scan ports
    std::mutex mtx;                   // mutex to keep integrity of open_ports vector while multithreading

    int total_ports = end - start + 1; // make port range inclusive

    // determine max threads to be used
    int max_threads = optimize_max_threads(total_ports);

    const char *ip_to_check = "127.0.0.1"; // will check local ip

    // establish chunk size
    int interval_size = std::max(1, (total_ports / max_threads));

    // iterable starting point for chunk of ports
    int thread_start = start;

    // populate a vector of threads that each contain a scan_port_range function that scans a certain range of ports
    for (int thread_num = 0; thread_num < max_threads; thread_num++)
    {

        int thread_end;

        if (thread_num == max_threads - 1)
        {
            thread_end = end;
        }
        else
        {
            thread_end = thread_start + interval_size - 1;
        }

        threads.push_back(std::thread(scan_port_range, thread_start, thread_end, ip_to_check, std::ref(open_ports), std::ref(mtx)));

        thread_start = thread_end + 1;
    }

    // wait for all threads to finish
    for (std::thread &t : threads)
    {
        t.join();
    }

    // message if no open ports found
    if (open_ports.size() == 0)
    {
        std::cout << "No open ports on " << ip_to_check << "\n";
    }

    // list open ports
    else
    {
        sort(open_ports.begin(), open_ports.end());
        std::cout << "Open ports on " << ip_to_check << ":\n";

        for (int i = 0; i < open_ports.size(); i++)
        {
            std::cout << open_ports[i] << "\n";
        }
    }
}

// Optimize Max Threads Function
//
int optimize_max_threads(int total_ports)
{

    // set number of threads to number of available threads
    int num_threads = std::thread::hardware_concurrency();

    // set minimum number of threads for incompatability with hardware_concurrency()
    if (num_threads == 0)
    {
        num_threads = 16;
    }
    // set maximum number of threads to prevent overload
    if (num_threads > 100)
    {
        num_threads = 100;
    }
    // handle case if more threads than ports to scan
    if (num_threads > total_ports)
    {
        num_threads = total_ports;
    }

    return num_threads;
}

void scan_port_range(int start, int end, const char *ip, std::vector<int> &open_ports, std::mutex &mtx)
{

    // loop through each port
    for (int port = start; port <= end; port++)
    {

        int sock = socket(AF_INET, SOCK_STREAM, 0); // create a TCP socket using IPv4

        // skip port if sock fails
        if (sock < 0)
        {
            continue;
        }

        struct sockaddr_in target;              // create target address of type sockaddr_in (struct that holds IP address, port, and address type)
        target.sin_addr.s_addr = inet_addr(ip); // set target IP address to local IP
        target.sin_family = AF_INET;            // specify target has IPv4 address
        target.sin_port = htons(port);          // set port to i passed from the calling for loop

        // set a default timeout interval to 200 ms
        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000;

        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

        // connect returns 0 if successful and -1 if failed
        int result = connect(sock, (sockaddr *)&target, sizeof(target));

        // if successful connection, lock the open ports vector and add the open port to it then unlock for other threads afterwards
        if (result == 0)
        {
            mtx.lock();
            open_ports.push_back(port);
            mtx.unlock();
        }

        close(sock); // close sock file descriptor
    }
}
