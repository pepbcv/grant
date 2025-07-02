#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>

#include <xen/grant_table.h>
#include <xen/gntdev.h>

#define PAGE_SIZE getpagesize()

int main(int argc, char ** argv){
    int i;
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
        fprintf(stderr, "Couldn't allocate gref struct\n");
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    gref->count = nb_grant;
    for(i = 0; i < nb_grant; i++){
        gref->refs[i].domid = domid;
        gref->refs[i].ref = refid[i];
    }

    int err = ioctl(gntdev_fd, IOCTL_GNTDEV_MAP_GRANT_REF, gref);
    if(err < 0){
        fprintf(stderr, "IOCTL failed\n");
        free(gref); free(refid); close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    char* shbuf = mmap(NULL, nb_grant*PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntdev_fd, gref->index);
    if(shbuf == MAP_FAILED){
        fprintf(stderr, "Mapping failed\n");
        free(gref); free(refid); close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    // TEMPO!! — Lettura + misurazione latenza
    long total_latency = 0;

    for (int i = 0; i < 10000; i++) {
        
        struct timespec start, end;
        
        shbuf[0] = 0;
        clock_gettime(CLOCK_MONOTONIC, &start); //INIZIO

        while (shbuf[0] != 1);

        clock_gettime(CLOCK_MONOTONIC, &end); //FINE
        
        long latency_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        total_latency += latency_ns;

        printf("Ricevuto: %.*s | Latenza: %ld ns\n", 40, shbuf + 1, latency_ns);
        
    }

    printf("LATENZA MEDIA: %.2f µs\n", total_latency / 10000.0 / 1000.0);

    err = munmap(shbuf, nb_grant*PAGE_SIZE);
    struct ioctl_gntdev_unmap_grant_ref ugref = {.index = gref->index, .count = nb_grant};
    ioctl(gntdev_fd, IOCTL_GNTDEV_UNMAP_GRANT_REF, &ugref);

    free(gref);
    free(refid);
    close(gntdev_fd);
    return 0;
}