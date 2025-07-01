#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <xen/event_channel.h> // EVENT CHANNELS

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <event_channel_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    evtchn_port_t port = strtoul(argv[1], NULL, 10);

    // Apri file descriptor evento
    int evtchn_fd = open("/dev/xen/evtchn", O_RDWR);
    if (evtchn_fd < 0) {
        perror("open /dev/xen/evtchn");
        return EXIT_FAILURE;
    }

    // BIND sulla porta condivisa
    struct evtchn_bind_port bind = { .port = port };
    if (ioctl(evtchn_fd, IOCTL_EVTCHN_BIND, &bind) < 0) {
        perror("IOCTL_EVTCHN_BIND");
        close(evtchn_fd);
        return EXIT_FAILURE;
    }

    printf("In attesa di evento sulla porta %u...\n", port);

    evtchn_port_t fired_port;
    if (read(evtchn_fd, &fired_port, sizeof(fired_port)) < 0) {
        perror("read");
        close(evtchn_fd);
        return EXIT_FAILURE;
    }

    printf("Evento ricevuto sulla porta: %u\n", fired_port);
    close(evtchn_fd);
    return 0;
}
