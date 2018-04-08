/**
 *  @file       ipk-mtrip.cc
 *  @author     Andrej Nano (xnanoa00)
 *  @date       2018-04-09
 *  @version    1.0
 * 
 *  @brief IPK 2018, 2nd project - Bandwidth Measurement (Ryšavý). MTrip Main source file.
 *  
 *  @section Description
 *  
 *  This program is a school project for Computer Communications and Networks 
 *  course at Faculty of Information Technology of Brno University of 
 *  Technology. Main goal of the project is to use a proper communication to
 *  measure the bandwidth between 2 hosts on the Internet. Use of UDP protocol 
 *  is required for measurment.
 *  
 *  The project consists of 2 parts. A reflector and a meter.
 *  Both are just runtime modes for the same executable.
 *  Measurement will be done by sending UDP packets from the
 *  meter to the reflector, reflector will then respond.
 *  The meter must implement prober algorithm to measure the max
 *  bandwidth at which there are no lost packets.
 * 
 *  @section Usage
 *  
 *  * ./ipk-mtrip reflect -p port 
 *  * ./ipk-mtrip meter -h vzdáleny_host -p vzdálený_port - s velikost_sondy -t doba_mereni
 *  
 */

// std libraries
#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <csignal>
#include <sys/wait.h>
#include <chrono>
#include <ctime>
#include <thread>
#include <cstring>
#include <memory>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

// commonly used std objects.. really no need to be careful about poluting namespace
using std::cout;
using std::cerr;
using std::endl;
using std::string;

using namespace std::literals::chrono_literals; // seconds as => '1s' 
using Clock = std::chrono::high_resolution_clock;

// sockets API + networking libraries
#include <sys/socket.h>
#include <netdb.h>              

// mtrip configurations + control/argument parse/interrupt handling
#include "ipk-mtrip.h"

// socket abstraction
#include "ipk-socket.h"

/*****************************************************************************/

/**
 *  @brief Main entry point, handles common routine and then delegates to runtime modes. 
 *  
 *  @param argc number of string arguments pointed to by argv
 *  @param argv vector of string arguments passed to the program
 *  @return exit code as an int
 */
int main(int argc, char **argv)
{
  // function to handle interrupt
  signal(SIGINT, interrupt_handler);
  
  // create new runtime mtrip configuration
  std::unique_ptr<MTripConfiguration> mtrip = argument_parser(argc, argv);
  
  if(!mtrip)
    return EXIT_FAILURE;

  // ENTRY POINT
  mtrip->init();

  return EXIT_SUCCESS;
}

/*****************************************************************************/

#define SAVE_CONNECTION true

/**
 * @brief Main routine of the reflector
 * 
 * @desc Initializes the reflector mode routine 
 * as the active configuration
 */
void Reflector::init()
{
  cout << "UDP BANDWIDTH MEASUREMENT\n" << endl;
  cout << "[REFLECTOR]: " << CL_GREEN << "started\n" << RESET << endl;

  // create new socket object
  std::shared_ptr<SocketEntity> socket = std::make_shared<SocketEntity>();

  // prepare the server
  socket->setup_server(m_port);
  cout << " [INFO]: Socket setup completed." << endl;

  /* ------------------------------------------ */
      // WAIT FOR REQUEST
  /* ------------------------------------------ */

  int probe_size { 0 };
  int total_time { 0 };
  int bytes_recv;

  while (true)
  {
    // RECEIVE -> probe size
    bytes_recv = socket->recv_message(reinterpret_cast<char*>(&probe_size), sizeof(probe_size), SAVE_CONNECTION);
    if (bytes_recv == -1) { cerr << "ERROR: ." << endl; continue;}
    if (probe_size <= 0) { cerr << "ERROR: probe size is less than 0. " << endl; continue; }

    // RECEIVE -> total time
    bytes_recv = socket->recv_message(reinterpret_cast<char*>(&total_time), sizeof(total_time));
    if (bytes_recv == -1) { cerr << "ERROR: ." << endl; continue;}

    // from now on, recv should be used with 'probe_size' value
    char probe_buffer[probe_size];
    long packets_recv { 0 };
    int current_round { 0 };
    
    // send RESPONSE
    probe_buffer[0] = 'O';
    probe_buffer[1] = 'K';
    socket->send_message(probe_buffer, probe_size);


    while (current_round < total_time)
    {
      // first RTT, just reflect
      socket->recv_message(probe_buffer, probe_size);
      socket->send_message(probe_buffer, probe_size);

      // then Bandwidth, collect/count then respond with number that arrived
      packets_recv = recv_packet_group(socket, probe_size);
      cout << "packets received: " << packets_recv << endl;
      // respond
      socket->send_message(reinterpret_cast<char*>(&packets_recv), sizeof(packets_recv));

      current_round++;
    }
    cout << " END ........ " << endl;
  }

}

// receive packets of fixed size for 1 second and count them
long Reflector::recv_packet_group(std::shared_ptr<SocketEntity> socket, int probe_size)
{
  // timeout when stuck.. 
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  setsockopt(socket->get_fd(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  char probe_buffer[probe_size];

  long packets_recv { 0 };
  int bytes_recv {0};
  
  socket->recv_message(probe_buffer, probe_size);
  packets_recv++;

  auto t1 = Clock::now();
  auto t2 = Clock::now();

  while (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() < 1000)
  {
    bytes_recv = socket->recv_message(probe_buffer, probe_size);

    if (bytes_recv == probe_size)
      packets_recv++;

    t2 = Clock::now();
  }

  // set back to default
  timeout.tv_sec = 0;
  setsockopt(socket->get_fd(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  return packets_recv;
}



/*****************************************************************************/

/**
 * @brief Main routine of the meter
 * 
 * @desc Initializes the measurement mode routine 
 * as the active configuration
 */
void Meter::init()
{
  cout << "UDP BANDWIDTH MEASUREMENT\n" << endl;
  cout << "[METER]: " << CL_GREEN << "started\n" << RESET << endl;

  // create new socket object
  std::shared_ptr<SocketEntity> socket = std::make_shared<SocketEntity>();
  
  // prepare the remote address
  socket->setup_connection(m_host_name.c_str(), m_port);
  cout << "\t[INFO]: Socket setup completed.\n" << endl;

  print_start_info(m_host_name, m_port, m_measurment_time, m_probe_size);

  /* ------------------------------------------ */
      // PREPARE MEASUREMENT
  /* ------------------------------------------ */

  long total_packets_sent { 0 };
  long total_packets_recv { 0 };
  std::vector<double> speed_list, rtt_list;

  long packets_recv { 0 };
  long packets_sent { 0 };

  char probe_buffer[m_probe_size];
  
  socket->send_message(reinterpret_cast<char*>(&m_probe_size), sizeof(m_probe_size));
  std::this_thread::sleep_for(1ms);

  socket->send_message(reinterpret_cast<char*>(&m_measurment_time), sizeof(m_measurment_time));

  socket->recv_message(probe_buffer, m_probe_size);

  if (probe_buffer[0] != 'O' || probe_buffer[1] != 'K')
  {
    cerr << "Reflector disagrees" << endl;
    exit(EXIT_FAILURE);
  }
  // otherwise ok.. measurement can start


  /* ------------------------------------------ */
      // MEASUREMENT
  /* ------------------------------------------ */


  int current_round {0};
  long long packet_rate {10000}; // 1s = 1 000 000 us ... -> packet send gap will be 1 000 000 / packet_rate us 
  long long old_rate {100};

  double rtt {0.0};

  while (current_round < m_measurment_time)
  {
    cout << " [" << current_round << ". round] ";
    
    // calculate RTT
    rtt = RTT(socket, m_probe_size);
    rtt_list.push_back(rtt);
    cout << "RTT: " << rtt << "ms" << endl;

    // send group @ rate
    packets_sent = send_packet_group(socket, packet_rate, m_probe_size);

    cout << "got here" << endl;

    // get response how many were received
    socket->recv_message(reinterpret_cast<char*>(&packets_recv), sizeof(packets_recv));
    cout << "packets received: " << packets_recv << endl;
    
    // calculate the speed in Mbits
    double speed = packets_recv * m_probe_size * 8 / 1000 / 1000;
    speed_list.push_back(speed);

    cout << "upload speed: " << speed << " Mb/s" << endl;
    
    cout << "old rate: " << packet_rate << endl;

    // adjust rate
    if (packets_recv < packets_sent) // packets were lost
    {
      packet_rate = (old_rate + packet_rate ) / 2;
    }
    else // no packets lost, increase the rate
    {
      old_rate = packet_rate;
      packet_rate = packet_rate * 2;
    }

    cout << "new rate: " << packet_rate << endl;

    total_packets_sent += packets_sent;
    total_packets_recv += packets_recv;

    current_round++;
  }

  /* ------------------------------------------ */
      // RESULTS
  /* ------------------------------------------ */

  print_result_info(m_probe_size, m_measurment_time, total_packets_sent, total_packets_recv, speed_list, rtt_list);
}



// send group of packets at a 'packet_rate' for 1 second
long Meter::send_packet_group(std::shared_ptr<SocketEntity> socket, long long packet_rate, int probe_size)
{
  char probe_buffer[probe_size];

  long packets_sent { 0 };
  auto send_gap = std::chrono::microseconds(1'000'000 / packet_rate);
  
  auto t1 = Clock::now();
  auto t2 = Clock::now();

  while (std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() < 1000)
  {
    socket->send_message(probe_buffer, probe_size);
    packets_sent++;
    std::this_thread::sleep_for(send_gap);
    t2 = Clock::now();
  }

  return packets_sent;
}


/**
 * @brief Print startup informations
 * 
 */
void print_start_info(string host_name, unsigned short port, int measurment_time, int probe_size)
{
  cout << "-----------------------------------" << endl;
  cout << "~ Host: "<< BOLD << host_name << RESET << endl;
  cout << "~ Port: " << BOLD << port << RESET << endl;
  cout << "~ Measurement time: " << BOLD << measurment_time << " seconds" << RESET << endl;
  cout << "~ Probe packet size: " << BOLD << probe_size << " Bytes" << RESET << endl;
  cout << "-----------------------------------" << endl;
}

/**
 * @brief Print results information
 * 
 */
void print_result_info(int probe_size, int measurement_time, long packets_sent, long packets_recv, std::vector<double> speed_list, std::vector<double> rtt_list)
{
  cout << "\n\n--------------------------------------------------------------------------------" << endl;
  cout << "  " << BOLD << "FINAL RESULTS" << RESET << endl;
  cout << "--------------------------------------------------------------------------------\n" << endl;

  cout << "   " << CL_BLUE << "PACKETS & DATA\n" << RESET << endl;
  cout << "\tPACKETS TRANSFERRED: " << packets_recv << "/" << packets_sent << " (received/sent)" << endl;
  cout << "\tPACKETS LOST: ~ " << 100 - (static_cast<long double>(packets_recv)/packets_sent) * 100 << "% loss" << endl;
  cout << "\tDATA TRANSFERED: " << packets_sent * probe_size / 1000 / 1000 << " MB SENT / " << packets_recv * probe_size / 1000 / 1000 << " MB RECEIVED\n" << endl;
  
  cout << "   " << CL_RED << "RTT\n " << RESET << endl;
  cout << "\tMAX RTT: "<< *std::max_element(rtt_list.begin(), rtt_list.end()) << " ms" << endl;
  cout << "\tMIN RTT: "<< *std::min_element(rtt_list.begin(), rtt_list.end()) << " ms" << endl;

  // avg rtt and standard deviation
  double rtt_sum = std::accumulate(rtt_list.begin(), rtt_list.end(), 0.0);
  double rtt_mean = rtt_sum / rtt_list.size();
  
  cout << "\tAVG RTT: "<< rtt_mean << " ms" << endl;

  std::vector<double> rtt_diff(rtt_list.size());
  std::transform(rtt_list.begin(), rtt_list.end(), rtt_diff.begin(), [rtt_mean](double d) { return d - rtt_mean;});

  double rtt_sum_sq = std::inner_product(rtt_diff.begin(), rtt_diff.end(), rtt_diff.begin(), 0.0);
  double rtt_std_dev = std::sqrt(rtt_sum_sq / rtt_list.size());

  cout << "\tSTD DEV: "<< rtt_std_dev << " ms\n" << endl;

  cout << "   " << CL_YELLOW << "AVAILABLE BANDWIDTH\n " << RESET << endl;
  cout << "\tMAX SPEED: "<< *std::max_element(speed_list.begin(), speed_list.end()) << " Mb/s" << endl;
  cout << "\tMIN SPEED: "<< *std::min_element(speed_list.begin(), speed_list.end()) << " Mb/s" << endl;

  // avg rtt and standard deviation
  double speed_sum = std::accumulate(speed_list.begin(), speed_list.end(), 0.0);
  double speed_mean = speed_sum / speed_list.size();
  
  cout << "\tAVG SPEED: "<< speed_mean << " Mb/s" << endl;

  std::vector<double> speed_diff(speed_list.size());
  std::transform(speed_list.begin(), speed_list.end(), speed_diff.begin(), [speed_mean](double d) { return d - speed_mean;});

  double speed_sum_sq = std::inner_product(speed_diff.begin(), speed_diff.end(), speed_diff.begin(), 0.0);
  double speed_std_dev = std::sqrt(speed_sum_sq / speed_list.size());

  cout << "\tSTD DEV: "<< speed_std_dev << " Mb/s\n\n" << endl;

}


/**
 * @brief Round trip time calculation
 * 
 * @desc Calculates roundtrip time (Single test)
 * For average value, function needs to be called multiple times
 * @param socket socket to be used for RTT calc.
 * @param buffer buffer to be used for sending/
 */
double Meter::RTT(std::shared_ptr<SocketEntity> socket, size_t buffer_size)
{ 
  char buffer[buffer_size];
  memset(buffer, 'R', buffer_size);

  auto start = std::chrono::system_clock::now();
  socket->send_message(buffer, buffer_size);
  socket->recv_message(buffer, buffer_size);
  auto end = std::chrono::system_clock::now();
  
  std::chrono::duration<double> duration = end - start;

  return duration.count() * 1000.0; // in ms
}




/*****************************************************************************/

/**
 *  @brief Properly handles interrupt, such as CTRL+C
 *  @param signum number of the signal caught
 *  @return void
 */
void interrupt_handler(int signum)
{
  cout << "\n\n[!!!] Caught signal(" << signum << "). Ending the program." << endl;
  exit(EXIT_SUCCESS);
}


/**
 *  @brief Parses arguments, checks their validity and returns a new MTrip Configuration object
 * 
 *  @param argc number of string arguments pointed to by argv
 *  @param argv vector of string arguments passed to the program
 *  @return program runtime configuration distinct for each mode
 */
std::unique_ptr<MTripConfiguration> argument_parser(int argc, char **argv)
{
  if (argc < 4)
  {
    cerr << "Wrong number of arguments." << endl;
    return nullptr;
  }

  char c; // help
  opterr = 0; // turn off getopt errors

  // METER MODE
  if (string(argv[optind]) == "meter")
  {
    optind++;

    // argument options
    bool h_flag = false, p_flag = false, s_flag = false, t_flag = false;
    
    // argument values
    string host_name;
    unsigned short port;
    size_t probe_size;
    float measurment_time;

    while ((c = getopt(argc, argv, "h:p:s:t:")) != -1)
    {
      switch (c)
      {
        case 'h':
          h_flag = true;
          host_name = optarg;
          break;
        case 'p':
          p_flag = true;
          port = static_cast<unsigned int>(atoi(optarg));
          break;
        case 's':
          s_flag = true;
          probe_size = static_cast<size_t>(atoi(optarg));
          break;
        case 't':
          t_flag = true;
          measurment_time = atoi(optarg);
          break;
        case '?':
          if (optopt == 'h' || optopt == 'p' || optopt == 's' || optopt == 't')
              cerr << "Option -" << static_cast<char>(optopt) << " requires an argument." << endl;
          else if (isprint(optopt))
              cerr << "Uknown option '-" << static_cast<char>(optopt) << "'" << endl;
          else
              cerr << "Unknown option character. " << endl;
          exit(1);
        default:
          cerr << "uknown getopt() error" << endl;
          exit(1);
          break;
      }
    }

    // everything OK -> create new configuration
    if (h_flag && p_flag && s_flag && t_flag)
    {
      return std::make_unique<Meter>(host_name, port, probe_size, measurment_time);
    }
    else
    {
      cerr << "Not all argument options passed in." << endl;
      return nullptr;
    }
  }
  else
  // REFLECT MODE
  if (string(argv[optind]) == "reflect")
  {
    optind++;

    // argument option + value
    bool p_flag = false;
    unsigned int port;

    while ((c = getopt(argc, argv, "p:")) != -1)
    {
      switch (c)
      {
        case 'p':
          p_flag = true;
          port = static_cast<unsigned int>(atoi(optarg));
          break;
        case '?':
          if (optopt == 'p')
              cerr << "Option -p requires an argument." << endl;
          else if (isprint(optopt))
              cerr << "Uknown option '-" << static_cast<char>(optopt) << "'" << endl;
          else
              cerr << "Unknown option character. " << endl;
          exit(1);
        default:
          cerr << "uknown getopt() error" << endl;
          exit(1);
          break;
      }
    }
    
    // everything OK -> create new configuration
    if (p_flag)
    {
      return std::make_unique<Reflector>(port);
    }
    else
    {
      cerr << "Required option not passed in." << endl;
      return nullptr;
    }
  }
  else
  {
    cerr << "Undefined mode inside an argument passed to the application." << std::endl;
    return nullptr;
  }
  
}
