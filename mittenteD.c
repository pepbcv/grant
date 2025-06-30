#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>

#include <xen/gntalloc.h> //inclusione driver per funzionalià grant tables

#define PAGE_SIZE getpagesize()

int main(int argc, char ** argv){
    uint16_t domid = 3; //We set to share these grants with Dom0 (domID della vm a cui si vuole dare accesso, cioè domuB_id)
    uint32_t count = 1; //We want to allocate one grant/page

    //apertura file descriptor sul nodo driver
    int gntalloc_fd = open("/dev/xen/gntalloc", O_RDWR);
    if(gntalloc_fd < 0){
        fprintf(stderr, "Couldn't open /dev/xen/gntalloc\n");
        exit(EXIT_FAILURE);
    }

    //allocazione strutturae popolamento dei campi
    struct ioctl_gntalloc_alloc_gref* gref = malloc(sizeof(struct ioctl_gntalloc_alloc_gref));
    if(gref == NULL){
        fprintf(stderr, "Couldn't allocate struct ioctl_gntalloc_alloc_gref\n");
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

    //chiamata map
    char* shpages = mmap(NULL, count*PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntalloc_fd, gref->index);
    if(shpages == MAP_FAILED){
        fprintf(stderr, "mapping the grants failed\n");
        free(gref);
        close(gntalloc_fd);
        exit(EXIT_FAILURE);
    }

    sprintf(shpages, "Hello, World!");

    /**
     * We wait for interrupt to begin cleaning up
     */
    int sig;
    sigset_t set;
    sigemptyset(&set);
    int ret = sigaddset(&set, SIGINT);
    if(ret < 0){
        fprintf(stderr, "Adding signal to sigset failed\n");
    }
    if(sigwait(&set, &sig)){
        fprintf(stderr, "Sigwait failed with error\n");
    }

    err = munmap(shpages, count*PAGE_SIZE);
    if(err < 0){
        fprintf(stderr, "Unmapping grants failed\n");
    }

    struct ioctl_gntalloc_dealloc_gref dgref;
    dgref.index = gref->index;
    dgref.count = count;

    err = ioctl(gntalloc_fd, IOCTL_GNTALLOC_DEALLOC_GREF, &dgref);
    if(err < 0){
        fprintf(stderr, "IOCTL for deallocating grants failed\n");
    }


    free(gref);
    close(gntalloc_fd);
    exit(EXIT_SUCCESS);
}
