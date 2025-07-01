#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <xen/event_channel.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s DOMU_SENDER DOMU_RECEIVER\n", argv[0]);
        return 1;
    }

    int dom_sender = atoi(argv[1]);
    int dom_receiver = atoi(argv[2]);

    int fd = open("/dev/xen/evtchn", O_RDWR);
    if (fd < 0) {
        perror("Errore apertura /dev/xen/evtchn");
        return 1;
    }

    struct evtchn_alloc_unbound req = {
        .dom = dom_sender,
        .remote_dom = dom_receiver
    };

    if (ioctl(fd, IOCTL_EVTCHN_ALLOC_UNBOUND, &req) < 0) {
        perror("Errore allocazione canale unbound");
        close(fd);
        return 1;
    }

    printf("Canale evento creato tra dom%d (mittente) e dom%d (ricevente): porta %u\n",
           dom_sender, dom_receiver, req.port);

    FILE *out = fopen("channel_info.txt", "w");
    fprintf(out, "%u\n", req.port);
    fclose(out);

    close(fd);
    return 0;
}
