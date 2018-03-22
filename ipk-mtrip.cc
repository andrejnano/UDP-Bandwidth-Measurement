/**
 *  @file       ipk-mtrip.cc
 *  @author     Andrej Nano (xnanoa00)
 *  @date       2018-04-09
 *  @version    0.1
 * 
 *  @brief IPK 2018, 2nd project - Bandwidth Measurement (Ryšavý).
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
 *  @section Example use
 *  
 *  ./ipk-mtrip reflect -p port 
 *  ./ipk-mtrip meter -h vzdáleny_host -p vzdálený_port - s velikost_sondy -t doba_mereni
 *  
 */

// standard library
#include <iostream>
    using std::cout;
    using std::cerr;
    using std::endl;
#include <unistd.h>

// project header file
#include "ipk-mtrip.h"

int main()
{
    
}