#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <xen/grant_table.h>
#include <xen/gntdev.h>
#include <xen/event_channel.h> // EVENT!!!!!!!

#define PAGE_SIZE getpagesize()

int main(int argc, char ** argv){
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <domA_ID> <grant_ref> <event_channel_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t domid = strtoul(argv[1], NULL, 10);
    uint32_t refid = strtoul(argv[2], NULL, 10);
    evtchn_port_t port = strtoul(argv[3], NULL, 10);

    // GRANT TABLE
    int gntdev_fd = open("/dev/xen/gntdev", O_RDWR);
    if(gntdev_fd < 0){
        perror("Couldn't open /dev/xen/gntdev");
        exit(EXIT_FAILURE);
    }

    struct ioctl_gntdev_map_grant_ref* gref = malloc(sizeof(*gref));
    gref->count = 1;
    gref->refs[0].domid = domid;
    gref->refs[0].ref = refid;

    if(ioctl(gntdev_fd, IOCTL_GNTDEV_MAP_GRANT_REF, gref) < 0){
        perror("IOCTL GNTDEV failed");
        free(gref);
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    char* shbuf = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntdev_fd, gref->index);
    if(shbuf == MAP_FAILED){
        perror("mmap failed");
        free(gref);
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    // EVENT!!!!!!! — BIND della porta evento
    int evtchn_fd = open("/dev/xen/evtchn", O_RDWR);
    if(evtchn_fd < 0){
        perror("Couldn't open /dev/xen/evtchn");
        exit(EXIT_FAILURE);
    }

    struct evtchn_bind_port bind = {.port = port};
    if(ioctl(evtchn_fd, IOCTL_EVTCHN_BIND, &bind) < 0){
        perror("IOCTL_EVTCHN_BIND failed");
        exit(EXIT_FAILURE);
    }

    printf("Ricevente in attesa evento sulla porta %u...\n", port);

    // Attesa evento
    evtchn_port_t fired_port;
    read(evtchn_fd, &fired_port, sizeof(fired_port));
    printf("Evento ricevuto su porta: %u\n", fired_port);

    // Lettura messaggio
    fwrite(shbuf, 1, 1024, stdout);
    printf("\n");

    // Cleanup
    munmap(shbuf, PAGE_SIZE);
    struct ioctl_gntdev_unmap_grant_ref ugref = {.index = gref->index, .count = 1};
    ioctl(gntdev_fd, IOCTL_GNTDEV_UNMAP_GRANT_REF, &ugref);

    close(gntdev_fd);
    close(evtchn_fd);
    free(gref);
    return 0;
}

