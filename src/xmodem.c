#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

#define BUFFER_SIZE 128

struct xm_packet {
    uint8_t code;
    uint8_t blk;
    uint8_t blk_inv;
    uint8_t data[BUFFER_SIZE];
    uint8_t chk;
};

uint8_t checksum(struct xm_packet *packet) {
    int sum = 0;
    int i;
    uint8_t res;
    sum += packet->code;
    sum += packet->blk;
    sum += packet->blk_inv;
    for (i = 0; i < sizeof(packet->data); i++) {
        sum += packet->data[i];
    }
    res = sum & 0xFF;
    return res;
}

int send_packet(int device, struct xm_packet *packet) {
    int bytes_written;
    int bytes_written_so_far = 0;
   
    while (bytes_written_so_far < sizeof(struct xm_packet)) {
        bytes_written = write(device, packet + bytes_written_so_far, sizeof(struct xm_packet) - bytes_written_so_far);
        if (bytes_written == -1) {
            printf("Error writing to serial port\n");
            return -1;
        }
        bytes_written_so_far += bytes_written;
    }
    
    return 0;
}

int xmodem_send(int device, FILE *fd) {
    int i;
    uint8_t buf[BUFFER_SIZE];
    uint8_t block = 1;
    size_t bytes_read;
    printf("Sending file...\n");

    while ((bytes_read = fread(&buf, sizeof(uint8_t), BUFFER_SIZE, fd)) > 0) {
        struct xm_packet packet;
        
        /* Set up the xmodem packet to be sent, according to xmodem specification */
        packet.code = SOH;
        packet.blk = block;
        packet.blk_inv = 255 - block;
        memcpy(&(packet.data), &buf, BUFFER_SIZE);
        packet.chk = checksum(&packet);
        
        /* Make 10 attempts to sent the packet */
        for (i = 0; i < 10; i++) {
            uint8_t rec_buf;
            int bytes_read;
            if(send_packet(device, &packet) == -1) {
                return -1;
            }
            bytes_read = read(device, &rec_buf, 1);
            if (bytes_read > 0 && rec_buf == ACK) {
                printf("Successfully sent packet %i\n", block);
                break; /* Received ACK; packet sent successfully */ 
            }
            if (bytes_read > 0 && rec_buf == NAK && i < 9) {
                printf("Received NAK, expected ACK. Retrying...\n");
                sleep(1); /* Received NAK; packet not sent. Wait 1 second and try again */
                continue;
            }
            if (i == 9) {
                printf("Error: No ACK received in 10 attempts\n");
                return -1;
            }
            printf("Couldn't send packet, retrying...\n");
            sleep(1);
            
        }
        block++;

    }
    for (i = 0; i < 10; i++) {
        uint8_t rec_buf;
        uint8_t send_buf = EOT;
        if (write(device, &send_buf, sizeof(uint8_t)) < sizeof(uint8_t)) 
            continue;
        if (read(device, &rec_buf, sizeof(uint8_t)) < sizeof(uint8_t)) 
            continue;
        if (rec_buf != ACK) {
            printf("No ACK for EOT message, retrying...\n");
            sleep(1);
            continue;
        }

        /* File transfer was successfully completed, return to caller */
        return 0;
    }

    /* File transfer was unsuccessful. Return with error code. */
    return -1;
}

