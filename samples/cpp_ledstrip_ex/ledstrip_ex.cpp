/*
  Basic example of how to send RGB data to an ESP32 device sporting NightDriver code and controlling an LED Strip.
  
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include <endian.h>

#define SERVER_IP "10.130.21.6"  // IP address of the server
#define SERVER_PORT 49152        // Port to connect to
#define NUM_LEDS 60              // Number of LEDs
#define BUFFER_SIZE 1024         // Buffer size for receiving data
#define SENDS_PER_SECOND 3       // Number of sends per second

struct SocketResponse
{
    uint32_t    size;              // 4
    uint64_t    sequence;          // 8 
    uint32_t    flashVersion;      // 4
    double      currentClock;      // 8
    double      oldestPacket;      // 8
    double      newestPacket;      // 8
    double      brightness;        // 8
    double      wifiSignal;        // 8
    uint32_t    bufferSize;        // 4
    uint32_t    bufferPos;         // 4
    uint32_t    fpsDrawing;        // 4
    uint32_t    watts;             // 4
} __attribute__((packed));

struct NightDriverHeader {
    union {
        struct {
            uint16_t command;
            uint16_t channel;
        };
        uint32_t magic_word; // Dave's magic word
    };
    uint32_t length;    // length of payload
    uint64_t seconds;   // timestamp in seconds
    uint64_t millis;    // timestamp in milliseconds
    uint8_t data[];     // flexible array member for payload data
};

// Function to generate random color values
void generate_random_colors(uint8_t *data, int num_leds) {
    for (int i = 0; i < num_leds * 3; i++) {
        data[i] = rand() % 256; // Generate a random byte (0-255)
    }
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

void print_socket_response(struct SocketResponse response) {
    printf("Size: %u\n", ntohl(response.size));
    printf("Sequence: %" PRIu64 "\n", be64toh(response.sequence));
    printf("Flash Version: %u\n", ntohl(response.flashVersion));
    printf("Current Clock: %.6f\n", ntohd(response.currentClock));
    printf("Oldest Packet: %.6f\n", ntohd(response.oldestPacket));
    printf("Newest Packet: %.6f\n", ntohd(response.newestPacket));
    printf("Brightness: %.6f\n", ntohd(response.brightness));
    printf("Wi-Fi Signal: %.6f\n", ntohd(response.wifiSignal));
    printf("Buffer Size: %u\n", ntohl(response.bufferSize));
    printf("Buffer Position: %u\n", ntohl(response.bufferPos));
    printf("FPS Drawing: %u\n", ntohl(response.fpsDrawing));
    printf("Watts: %u\n", ntohl(response.watts));
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    struct NightDriverHeader *header;
    size_t header_size;
    unsigned char buffer[BUFFER_SIZE];
    int bytes_sent, bytes_received;

    // Seed the random number generator
    srand(time(NULL));

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 1000000000L / SENDS_PER_SECOND;  // Interval between sends in nanoseconds

    while (1) {
        size_t data_size = NUM_LEDS * 3;
        header_size = sizeof(struct NightDriverHeader) + data_size;

        // Allocate memory for the struct, including the data payload
        header = (struct NightDriverHeader*)malloc(header_size);
        if (!header) {
            perror("Memory allocation failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Populate the struct
        header->command = htons(0x0003);
        header->channel = htons(0x0000); 
        //header->magic_word = htonl(0x44415645);
        header->length = htonl(NUM_LEDS);  // number of LEDs
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        header->seconds = 0; // htobe64(ts.tv_sec);
        header->millis = 0; // htobe64(ts.tv_nsec / 1000000);
        generate_random_colors(header->data, NUM_LEDS);

        // Print the entire structure
        printf("Structure to send:\n");
        //printf("Magic Word: %u\n", ntohl(header->magic_word));
        printf("Command: 0x%04x\n", ntohs(header->command));
        printf("Channel: %u\n", ntohs(header->channel));
        printf("Length: %u\n", ntohl(header->length));
        printf("Seconds: %lu\n", be64toh(header->seconds));
        printf("Millis: %lu\n", be64toh(header->millis));
        printf("Data: ");
        for (int i = 0; i < data_size; i++) {
            printf("%02x ", header->data[i]);
        }
        printf("\n");

        // Send data
        bytes_sent = send(sockfd, header, header_size, 0);
        if (bytes_sent < 0) {
            perror("Send failed");
            close(sockfd);
            free(header);
            exit(EXIT_FAILURE);
        }
        printf("Bytes sent: %d\n", bytes_sent);
        // Free the allocated memory
        free(header);

        // Receive data
        int valread = read(sockfd, buffer, BUFFER_SIZE);
        if (valread > 0) {
			printf("Received %d bytes.\n", valread);
            struct SocketResponse response;
            memcpy(&response, buffer, sizeof(struct SocketResponse));
            print_socket_response(response);
        }

        // Wait for the calculated interval
        nanosleep(&sleep_time, NULL);
    }

    // Close the socket
    close(sockfd);

    return 0;
}
