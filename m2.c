#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>

#include <xen/gntalloc.h>      // grant tables
#include <xen/event_channel.h> // EVENT!!!!!!!

#define PAGE_SIZE getpagesize()

int main(int argc, char ** argv){
    uint16_t domid; // ID del ricevente (domuB)
    uint32_t count = 1;

    domid = strtoul(argv[1], NULL, 10);

    int gntalloc_fd = open("/dev/xen/gntalloc", O_RDWR);
    if(gntalloc_fd < 0){
        fprintf(stderr, "Couldn't open /dev/xen/gntalloc\n");
        exit(EXIT_FAILURE);
    }

    struct ioctl_gntalloc_alloc_gref* gref = malloc(sizeof(*gref));
    if(gref == NULL){
        fprintf(stderr, "Couldn't allocate gref\n");
        close(gntalloc_fd);
        exit(EXIT_FAILURE);
    }
    gref->domid = domid;
    gref->count = count;
    gref->flags = GNTALLOC_FLAG_WRITABLE;

    int err = ioctl(gntalloc_fd, IOCTL_GNTALLOC_ALLOC_GREF, gref);
    if(err < 0){
        fprintf(stderr, "IOCTL failed\n");
        free(gref);
        close(gntalloc_fd);
        exit(EXIT_FAILURE);
    }

    printf("gref = %u\n", gref->gref_ids[0]);

    char* shpages = mmap(NULL, count * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntalloc_fd, gref->index);
    if(shpages == MAP_FAILED){
        fprintf(stderr, "mapping the grants failed\n");
        free(gref);
        close(gntalloc_fd);
        exit(EXIT_FAILURE);
    }



    // EVENT!!!!!!! — Inizializza unbound channel
    int evtchn_fd = open("/dev/xen/evtchn", O_RDWR);
    if (evtchn_fd < 0) {
        perror("Couldn't open /dev/xen/evtchn");
        exit(EXIT_FAILURE);
    }

    struct evtchn_alloc_unbound req = {
        .dom = 0,            // mio dominio
        .remote_dom = domid  // ricevente
    };
    
    err = ioctl(evtchn_fd, IOCTL_EVTCHN_ALLOC_UNBOUND, &req);
    if (err < 0) {
        perror("Failed to allocate event channel");
        exit(EXIT_FAILURE);
    }

    printf("Porta evento: %u\n", req.port);


    //scrivo
    // Scrittura messaggio 1024 byte
    memset(shpages, 'A', 1023);
    shpages[1023] = '\0';

    getchar();  // attesa opzionale
    
    // EVENT!!!!!!! — Notifica ricevente
    ioctl(evtchn_fd, IOCTL_EVTCHN_NOTIFY, &req.port);
    printf("Mittente ha notificato\n");

    // Cleanup su CTRL+C
    int sig;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigwait(&set, &sig);

    munmap(shpages, count * PAGE_SIZE);
    struct ioctl_gntalloc_dealloc_gref dgref = {.index = gref->index, .count = count};
    ioctl(gntalloc_fd, IOCTL_GNTALLOC_DEALLOC_GREF, &dgref);

    free(gref);
    close(gntalloc_fd);
    close(evtchn_fd);
    return 0;
}
