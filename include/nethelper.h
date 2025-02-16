#ifndef _NETHELPER_H_
#define _NETHELPER_H_

#include <cstdint>

// Function to convert 64-bit integer from host to big-endian byte order
uint64_t htobe64(uint64_t host_64);

// Function to convert 64-bit integer from big-endian to host byte order
uint64_t be64toh(uint64_t net_64);

// Function to convert a double to network byte order
double htond(double host_double);

// Function to convert a double from network byte order to host byte order
double ntohd(uint64_t net_double);

#endif // _NETHELPER_H_
