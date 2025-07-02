#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>

#include <xen/gntalloc.h>

#define PAGE_SIZE getpagesize()

int main(int argc, char ** argv){
    uint16_t domid;
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

    memset(shpages, 0, PAGE_SIZE);  // pulizia iniziale memoria

    struct timespec send_time;

    for (int i = 0; i < 10; i++) {
        while (shpages[0] != 0);

        clock_gettime(CLOCK_MONOTONIC, &send_time);  // TEMPO!!
		printf("DEBUG Mittente: send_time = %ld.%09ld\n", send_time.tv_sec, send_time.tv_nsec);  // DEBUG
		memcpy(shpages + 1, &send_time, sizeof(struct timespec)); // scrittura tempo

        shpages[0] = 1;
        printf("Mittente ha inviato: %d\n", i);
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