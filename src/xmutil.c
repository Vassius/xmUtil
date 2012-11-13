#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdint.h>

#define VERSION "0.1"

extern int xmodem_send(int device, FILE *fd);

void usage(char *cmd) {
    printf("xmUtil v%s by Markus Wiik <vassius@gmail.com>\n", VERSION);
    printf("Usage: \n");
    printf("\t%s <device> <filename>\n\n", cmd);
}

int main(int argc, char* argv[]) {
    int fd; /* File descriptor for serial device */
    FILE *file_to_send; /* File descriptor for the file to send */
    struct termios attr; /* Struct to hold serial device attributes */
    int retval;
    uint8_t rcv_buf[128];

    if (argc != 3) {
        usage(argv[0]);
        return 0;
    }

    file_to_send = fopen(argv[2], "r");
    if (file_to_send == 0) {
        printf("Couldn't open file %s\n", argv[2]);
        return 1;
    }

    fd = open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        printf("Couldn't open serial device %s\n", argv[1]);
        return 1;
    }
    tcgetattr(fd, &attr);

    /* Set input and output speed to 115200 baud */
    cfsetispeed(&attr, B115200);
    cfsetospeed(&attr, B115200);

    /* Set data bits, parity and stop bit to 8N1 */
    attr.c_cflag &= ~PARENB;
    attr.c_cflag &= ~CSTOPB;
    attr.c_cflag &= ~CSIZE;
    attr.c_cflag |= CS8;
    
    attr.c_cflag &= ~CRTSCTS;
    attr.c_iflag &= ~(IXON | IXOFF | IXANY);
    attr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    /* Apply attributes to the serial port device */
    tcsetattr(fd, TCSANOW, &attr);
    fcntl(fd, F_SETFL, 0);

    printf("Initializing file transfer...\n");
    read(fd, &rcv_buf, sizeof(rcv_buf));
    retval = xmodem_send(fd, file_to_send);

    if (retval == -1) 
        printf("File transfer failed.\n");
    else 
        printf("File transfer finished successfully.\n");

    fclose(file_to_send);
    close(fd);

    return 0;
}

