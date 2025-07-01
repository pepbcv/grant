#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h> // TEMPO!!

#include <xen/grant_table.h>
#include <xen/gntdev.h>

#define PAGE_SIZE getpagesize()

int main(int argc, char ** argv){
    uint32_t nb_grant = 1;
    uint16_t domid;
    uint32_t* refid = malloc(sizeof(uint32_t) * nb_grant);

    domid = strtoul(argv[1], NULL, 10);
    refid[0] = strtoul(argv[2], NULL, 10);

    int gntdev_fd = open("/dev/xen/gntdev", O_RDWR);
    if(gntdev_fd < 0){
        fprintf(stderr, "Couldn't open /dev/xen/gntdev\n");
        exit(EXIT_FAILURE);
    }

    struct ioctl_gntdev_map_grant_ref* gref = malloc(sizeof(struct ioctl_gntdev_map_grant_ref) + (nb_grant-1) * sizeof(struct ioctl_gntdev_grant_ref));
    if(gref == NULL){
        fprintf(stderr, "Couldn't allocate struct ioctl_gntdev_map_grant_ref\n");
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    gref->count = nb_grant;
    for(int i = 0; i < nb_grant; i++){
        gref->refs[i].domid = domid;
        gref->refs[i].ref = refid[i];
    }

    int err = ioctl(gntdev_fd, IOCTL_GNTDEV_MAP_GRANT_REF, gref);
    if(err < 0){
        fprintf(stderr, "IOCTL failed\n");
        free(gref); free(refid);
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    char* shbuf = mmap(NULL, nb_grant * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntdev_fd, gref->index);
    if(shbuf == MAP_FAILED){
        fprintf(stderr, "Failed mapping the grant references\n");
        free(gref); free(refid);
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    // TEMPO!!
    struct timespec start_time, end_time;
    double total_latency = 0;

    for (int i = 0; i < 10000; i++) {
        while (shbuf[0] != 1);

        // TEMPO!! - Tempo attuale alla ricezione
        clock_gettime(CLOCK_MONOTONIC, &end_time);

        // TEMPO!! - Lettura timestamp dal mittente
        struct timespec sent_time;
        memcpy(&sent_time, shbuf + 1, sizeof(struct timespec));

        // TEMPO!! - Calcolo latenza
        double latency = (end_time.tv_sec - sent_time.tv_sec) +
                         (end_time.tv_nsec - sent_time.tv_nsec) / 1e9;
        total_latency += latency;

        printf("Ricevente ha ricevuto: %s | Latenza: %.6f s\n", shbuf + 1 + sizeof(struct timespec), latency);
        shbuf[0] = 0;
    }

    // TEMPO!! - Stampa latenza media
    printf("Latenza media: %.6f secondi\n", total_latency / 10000);

    err = munmap(shbuf, nb_grant * PAGE_SIZE);
    if(err < 0){
        fprintf(stderr, "Unmapping the grants failed\n");
    }

    struct ioctl_gntdev_unmap_grant_ref ugref = {.index = gref->index, .count = nb_grant};
    ioctl(gntdev_fd, IOCTL_GNTDEV_UNMAP_GRANT_REF, &ugref);

    free(gref);
    free(refid);
    close(gntdev_fd);
    exit(EXIT_SUCCESS);
}