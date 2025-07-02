#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>

#include <xen/gntalloc.h> // grant tables

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

    char* shpages = mmap(NULL, count*PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntalloc_fd, gref->index);
    if(shpages == MAP_FAILED){
        fprintf(stderr, "mapping the grants failed\n");
        free(gref);
        close(gntalloc_fd);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 10000; i++) {
        while (shpages[0] != 0);  // attendo che il ricevente legga

        // TEMPO!! — Costruzione messaggio leggibile + padding
        char msg[1024];
        snprintf(msg, 128, "Messaggio numero %d", i); // parte leggibile
        memset(msg + strlen(msg), 'X', 1023 - strlen(msg)); // padding
        msg[1023] = '\0';

        memcpy(shpages + 1, msg, 1023); // scrittura nella shared page
        shpages[0] = 1;
        printf("Mittente ha inviato: %.*s\n", 40, msg); // stampa solo i primi caratteri
    }

    int sig;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigwait(&set, &sig);

    err = munmap(shpages, count*PAGE_SIZE);
    struct ioctl_gntalloc_dealloc_gref dgref = {.index = gref->index, .count = count};
    ioctl(gntalloc_fd, IOCTL_GNTALLOC_DEALLOC_GREF, &dgref);

    free(gref);
    close(gntalloc_fd);
    return 0;
}