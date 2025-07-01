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

#define PAGE_SIZE getpagesize()
#define MSG_SIZE 1023

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <receiver_domid>\n", argv[0]);
        return 1;
    }

    uint16_t domid = strtoul(argv[1], NULL, 10);
    uint32_t count = 1;

    int gntalloc_fd = open("/dev/xen/gntalloc", O_RDWR);
    if (gntalloc_fd < 0) {
        perror("Couldn't open /dev/xen/gntalloc");
        exit(EXIT_FAILURE);
    }

    struct ioctl_gntalloc_alloc_gref *gref = malloc(sizeof(*gref));
    gref->domid = domid;
    gref->count = count;
    gref->flags = GNTALLOC_FLAG_WRITABLE;

    if (ioctl(gntalloc_fd, IOCTL_GNTALLOC_ALLOC_GREF, gref) < 0) {
        perror("IOCTL_ALLOC_GREF failed");
        close(gntalloc_fd);
        free(gref);
        exit(EXIT_FAILURE);
    }

    printf("gref = %u\n", gref->gref_ids[0]);

    char *shpages = mmap(NULL, count * PAGE_SIZE, PROT_READ | PROT_WRITE,
                         MAP_SHARED, gntalloc_fd, gref->index);
    if (shpages == MAP_FAILED) {
        perror("mmap failed");
        close(gntalloc_fd);
        free(gref);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 10; i++) {
        while (shpages[0] != 0) usleep(100);  // attende che B legga

        memset(shpages + 1, 'A' + (i % 26), MSG_SIZE);
        shpages[MSG_SIZE + 1] = '\0';
        shpages[0] = 1;  // notifica B

        printf("Messaggio %d inviato\n", i + 1);
    }

    pause();  // aspetta CTRL+C per pulizia

    munmap(shpages, count * PAGE_SIZE);

    struct ioctl_gntalloc_dealloc_gref dgref = {
        .index = gref->index,
        .count = count
    };
    ioctl(gntalloc_fd, IOCTL_GNTALLOC_DEALLOC_GREF, &dgref);

    close(gntalloc_fd);
    free(gref);
    return 0;
}