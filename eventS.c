#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <xen/event_channel.h> // EVENT CHANNELS

#define IOCTL_EVTCHN_ALLOC_UNBOUND 0x40044501
#define IOCTL_EVTCHN_NOTIFY        0x40044506

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <remote_dom_id>\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t remote_dom = strtoul(argv[1], NULL, 10);

    // Apri file descriptor evento
    int evtchn_fd = open("/dev/xen/evtchn", O_RDWR);
    if (evtchn_fd < 0) {
        perror("open /dev/xen/evtchn");
        return EXIT_FAILURE;
    }

    // Richiedi unbound channel verso domid remoto
    struct evtchn_alloc_unbound evt = {
        .dom = 0,              // mio dominio
        .remote_dom = remote_dom
    };

    if (ioctl(evtchn_fd, IOCTL_EVTCHN_ALLOC_UNBOUND, &evt) < 0) {
        perror("IOCTL_EVTCHN_ALLOC_UNBOUND");
        close(evtchn_fd);
        return EXIT_FAILURE;
    }

    printf("EVENT CHANNEL creato. Porta: %u\n", evt.port);
    printf("Premi INVIO per inviare la notifica...\n");
    getchar();

    // Notifica
    if (ioctl(evtchn_fd, IOCTL_EVTCHN_NOTIFY, &evt.port) < 0) {
        perror("IOCTL_EVTCHN_NOTIFY");
        close(evtchn_fd);
        return EXIT_FAILURE;
    }

    printf("Notifica inviata sulla porta %u\n", evt.port);

    close(evtchn_fd);
    return 0;
}
