#include <arpa/inet.h>

#include <cstring>

#include "nethelper.h"

// Function to convert 64-bit integer from host to big-endian byte order
uint64_t htobe64(uint64_t host_64) {
    uint64_t result = ((uint64_t)htonl(host_64 & 0xFFFFFFFF) << 32) | htonl(host_64 >> 32);
    return result;
}

// Function to convert 64-bit integer from big-endian to host byte order
uint64_t be64toh(uint64_t net_64) {
    uint64_t result = ((uint64_t)ntohl(net_64 & 0xFFFFFFFF) << 32) | ntohl(net_64 >> 32);
    return result;
}

// Function to convert a double to network byte order
double htond(double host_double) {
  union {
      double d;
      uint64_t i;
  } conv;

  conv.d = host_double;
  conv.i = htobe64(conv.i);

  double net_double;
  memcpy(&net_double, &conv.i, sizeof(double));
  return net_double;
}

// Function to convert a double from network byte order to host byte order
double ntohd(double net_double) {
  union {
      double d;
      uint64_t i;
  } conv;

  memcpy(&conv.i, &net_double, sizeof(double));
  conv.i = be64toh(conv.i);

  return conv.d;
}