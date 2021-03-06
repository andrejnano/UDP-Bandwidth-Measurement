/**
 *  @file       ipk-mtrip.h
 *  @author     Andrej Nano (xnanoa00)
 *  @date       2018-04-09
 *  @version    1.0
 * 
 *  @brief IPK 2018, 2nd project - Bandwidth Measurement (Ryšavý). MTrip Header file.
 *  
 *  @section Description
 *  
 *  Since the application requires 2 modes of execution, I decided to abstract them
 *  and use each mode as a "configuration object" with some things same for both.
 *  This header file declares such object classes.
 *  
 *  Also, some basic functions required for this application such as argument parsing
 *  and signal handler are declared inside this header file.
 */

#ifndef IPK_MTRIP_H_
#define IPK_MTRIP_H_

  #include <memory>
  #include <string>
  #include <vector>
  #include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

  // socket abstraction
  #include "ipk-socket.h"


  // terminal output ANSI colors
  #define CL_RED     "\x1b[31m"
  #define CL_GREEN   "\033[32;1m"
  #define CL_YELLOW  "\x1b[33m"
  #define CL_BLUE    "\x1b[34m"
  #define CL_MAGENTA "\x1b[35m"
  #define CL_CYAN    "\x1b[36m"
  #define BOLD       "\033[4;1m"
  #define RESET      "\x1b[0m"


  /**
   *  @brief Abstract MTrip runtime configuration
   *  
   *  @desc The main idea is to create better abstraction of the program runtime modes.
   */
  class MTripConfiguration
  {
    public:

      enum mtrip_mode_t 
      {
        REFLECT_MODE = 0,
        METER_MODE   = 1
      };

      virtual mtrip_mode_t get_mode() = 0; // return the mode
      virtual void init() = 0;             // start the proper computation

      virtual ~MTripConfiguration() {};   // pure virtual destructor
  };


  /**
   *  @brief Specialized Reflector mode configuration
   *  
   *  @desc Reflector responds to incoming probe packets.
   *  It is used as './ipk-mtrip reflect -p port ' and only needs 1 value stored.
   */
  class Reflector : public MTripConfiguration
  {
    private:
      mtrip_mode_t mode;
      unsigned short m_port;
    
    public:
      // constructor
      Reflector() : mode {REFLECT_MODE} {}

      // usual constructor
      Reflector(unsigned short port) : m_port {port} {}

      // virtual destructor
      ~Reflector() override {};

      // initializes the reflecting mode routine 
      void init() override;

      // receive packets of fixed size for 1 second and count them
      long recv_packet_group(std::shared_ptr<SocketEntity> socket, int probe_size);

      // request mode of the current program runtime
      inline mtrip_mode_t get_mode() override { return mode; }
  };


  /**
   *  @brief Specialized Meter mode configuration
   *  
   *  @desc Meter sends probe packets to reflector and measures the maximum bandwidth.
   *  It is used as:
   *  './ipk-mtrip meter -h vzdáleny_host -p vzdálený_port - s velikost_sondy -t doba_mereni' 
   *  and needs to store 4 values.
   */
  class Meter : public MTripConfiguration
  {
    private:
      mtrip_mode_t mode;
      std::string m_host_name;
      unsigned short m_port;
      int m_probe_size;
      int m_measurment_time;

    public:

      // basic constructor
      Meter() : mode {METER_MODE} {}

      // usual constructor
      Meter(std::string host_name, unsigned short port, int probe_size, int measurment_time)
        : mode {METER_MODE}, 
          m_host_name{ host_name }, 
          m_port {port}, 
          m_probe_size {probe_size}, 
          m_measurment_time {measurment_time}
      {}

      // virtual destructor
      ~Meter() override {}

      // initializes the measurement mode routine 
      void init() override;

      // get RoundTripTime 
      double RTT(std::shared_ptr<SocketEntity> socket, size_t buffer_size);
      
      // send group of packets at a 'packet_rate' for 1 second
      long send_packet_group(std::shared_ptr<SocketEntity> socket, long long packet_rate, int probe_size);

      // request mode of the current program runtime
      inline mtrip_mode_t get_mode() override { return mode;}
  };


  /**
   *  @brief Properly handles interrupt, such as CTRL+C
   * 
   *  @param signum number of the signal caught
   *  @return void
   */
  void interrupt_handler(int signum);


  /**
   *  @brief Parses arguments passed to the program and checks for their validity
   * 
   *  @param signum number of the signal caught
   *  @return program configuration distinct for each mode
   */
  std::unique_ptr<MTripConfiguration> argument_parser(int argc, char **argv);


  /**
   * @brief Print startup informations
   * 
   */
  void print_start_info(string host_name, unsigned short port, int measurment_time, int probe_size);


  /**
   * @brief Print results information
   * 
   */
  void print_result_info(int probe_size, int measurement_time, long packets_sent, long packets_recv, std::vector<double> speed_list, std::vector<double> rtt_list);


#endif // IPK_MTRIP_H
