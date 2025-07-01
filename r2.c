#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <xen/gntdev.h>
#include <xen/grant_table.h>

#define PAGE_SIZE getpagesize()
#define MSG_SIZE 1023

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <sender_domid> <gref>\n", argv[0]);
        return 1;
    }

    uint16_t domid = strtoul(argv[1], NULL, 10);
    uint32_t gref_id = strtoul(argv[2], NULL, 10);
    uint32_t nb_grant = 1;

    int gntdev_fd = open("/dev/xen/gntdev", O_RDWR);
    if (gntdev_fd < 0) {
        perror("Couldn't open /dev/xen/gntdev");
        exit(EXIT_FAILURE);
    }

    struct ioctl_gntdev_map_grant_ref *gref =
        malloc(sizeof(*gref) + (nb_grant - 1) * sizeof(struct ioctl_gntdev_grant_ref));
    gref->count = nb_grant;
    gref->refs[0].domid = domid;
    gref->refs[0].ref = gref_id;

    if (ioctl(gntdev_fd, IOCTL_GNTDEV_MAP_GRANT_REF, gref) < 0) {
        perror("IOCTL_MAP_GREF failed");
        close(gntdev_fd);
        free(gref);
        exit(EXIT_FAILURE);
    }

    char *shbuf = mmap(NULL, nb_grant * PAGE_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED, gntdev_fd, gref->index);
    if (shbuf == MAP_FAILED) {
        perror("mmap failed");
        close(gntdev_fd);
        free(gref);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 10; i++) {
        while (shbuf[0] != 1) usleep(100);  // attende notifica

        printf("Ricevuto messaggio %d: ", i + 1);
        fwrite(shbuf + 1, 1, MSG_SIZE, stdout);
        printf("\n");

        shbuf[0] = 0;  // resetta flag
    }

    munmap(shbuf, nb_grant * PAGE_SIZE);

    struct ioctl_gntdev_unmap_grant_ref ugref = {
        .index = gref->index,
        .count = nb_grant
    };
    ioctl(gntdev_fd, IOCTL_GNTDEV_UNMAP_GRANT_REF, &ugref);

    close(gntdev_fd);
    free(gref);
    return 0;
}