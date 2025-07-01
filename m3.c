#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>                  // TEMPO!!

#include <xen/grant_table.h>
#include <xen/gntdev.h>

#define PAGE_SIZE getpagesize()

// passa come parametri domuA_ID e grant_ref
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
        fprintf(stderr, "Couldn't allocate struct ioctl_gntdev_map_grant_ref\n");
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    gref->count = nb_grant;
    for(i = 0; i < nb_grant; i++){
        struct ioctl_gntdev_grant_ref* ref = &gref->refs[i];
        ref->domid = domid;
        ref->ref = refid[i];
    }

    int err = ioctl(gntdev_fd, IOCTL_GNTDEV_MAP_GRANT_REF, gref);
    if(err < 0){
        fprintf(stderr, "IOCTL failed\n");
        free(gref); free(refid);
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    char* shbuf = mmap(NULL, nb_grant*PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntdev_fd, gref->index);
    if(shbuf == MAP_FAILED){
        fprintf(stderr, "Failed mapping the grant references\n");
        free(gref); free(refid);
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 10; i++) {
        while (shbuf[0] != 1);

        char raw[1024];                                    // TEMPO!!
        memcpy(raw, shbuf + 1, 1023);                       // TEMPO!!
        raw[1023] = '\0';                                  // TEMPO!!

        long sent_sec, sent_nsec;                          // TEMPO!!
        char msg[900];                                     // TEMPO!!
        sscanf(raw, "%ld|%ld|%899[^\n]", &sent_sec, &sent_nsec, msg); // TEMPO!!

        struct timespec now;                               // TEMPO!!
        clock_gettime(CLOCK_MONOTONIC, &now);              // TEMPO!!

        long latency_ns = (now.tv_sec - sent_sec) * 1000000000L + (now.tv_nsec - sent_nsec); // TEMPO!!
        printf("Ricevuto: %s\nLatenza: %ld ns\n", msg, latency_ns); // TEMPO!!

        shbuf[0] = 0;
    }

    err = munmap(shbuf, nb_grant*PAGE_SIZE);
    if(err < 0){
        fprintf(stderr, "Unmapping the grants failed\n");
    }

    struct ioctl_gntdev_unmap_grant_ref ugref;
    ugref.index = gref->index;
    ugref.count = nb_grant;

    err = ioctl(gntdev_fd, IOCTL_GNTDEV_UNMAP_GRANT_REF, &ugref);
    if(err < 0){
        fprintf(stderr, "IOCTL for unmap failed\n");
    }

    free(gref);
    free(refid);
    close(gntdev_fd);
    exit(EXIT_SUCCESS);
}