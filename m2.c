#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>

#include <xen/gntalloc.h>
#include <xen/event_channel.h> // EVENT!!!!!!!

#define PAGE_SIZE getpagesize()

int main(int argc, char ** argv){
    uint16_t domid; // domU ricevente (es: domuB)
    uint32_t count = 1;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <remote_dom_id>\n", argv[0]);
        return EXIT_FAILURE;
    }

    domid = strtoul(argv[1], NULL, 10);

    int gntalloc_fd = open("/dev/xen/gntalloc", O_RDWR);
    if(gntalloc_fd < 0){
        perror("Couldn't open /dev/xen/gntalloc");
        exit(EXIT_FAILURE);
    }

    struct ioctl_gntalloc_alloc_gref* gref = malloc(sizeof(*gref));
    gref->domid = domid;
    gref->count = count;
    gref->flags = GNTALLOC_FLAG_WRITABLE;

    if(ioctl(gntalloc_fd, IOCTL_GNTALLOC_ALLOC_GREF, gref) < 0){
        perror("IOCTL GNTALLOC failed");
        free(gref);
        close(gntalloc_fd);
        exit(EXIT_FAILURE);
    }

    char* shpages = mmap(NULL, count * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntalloc_fd, gref->index);
    if(shpages == MAP_FAILED){
        perror("mmap failed");
        free(gref);
        close(gntalloc_fd);
        exit(EXIT_FAILURE);
    }

    // EVENT!!!!!!! Allocazione canale evento
    int evtchn_fd = open("/dev/xen/evtchn", O_RDWR);
    if(evtchn_fd < 0){
        perror("Couldn't open /dev/xen/evtchn");
        exit(EXIT_FAILURE);
    }

    struct evtchn_alloc_unbound evt;
    evt.dom = 0; // mio dominio
    evt.remote_dom = domid;

    if(ioctl(evtchn_fd, IOCTL_EVTCHN_ALLOC_UNBOUND, &evt) < 0){
        perror("IOCTL_EVTCHN_ALLOC_UNBOUND failed");
        exit(EXIT_FAILURE);
    }

    printf("gref = %u\n", gref->gref_ids[0]);
    printf("event_channel_port = %u\n", evt.port); // DA COMUNICARE A ricevente

    // Scrivo messaggio
    memset(shpages, 'A', 1023);
    shpages[1023] = '\0';

    getchar(); // Pausa manuale per sincronia

    // EVENT!!!!!!! Notifica ricevente
    ioctl(evtchn_fd, IOCTL_EVTCHN_NOTIFY, &evt.port);
    printf("Mittente ha notificato evento!\n");

    // Cleanup on Ctrl+C
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    int sig;
    sigwait(&set, &sig);

    munmap(shpages, count * PAGE_SIZE);
    struct ioctl_gntalloc_dealloc_gref dgref = {.index = gref->index, .count = count};
    ioctl(gntalloc_fd, IOCTL_GNTALLOC_DEALLOC_GREF, &dgref);

    close(gntalloc_fd);
    close(evtchn_fd);
    free(gref);
    return 0;
}
