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
    uint32_t nb_grant = 1;
    uint16_t domid = strtoul(argv[1], NULL, 10);
    uint32_t refid = strtoul(argv[2], NULL, 10);

    int gntdev_fd = open("/dev/xen/gntdev", O_RDWR);
    if(gntdev_fd < 0){
        perror("open /dev/xen/gntdev");
        exit(EXIT_FAILURE);
    }

    struct ioctl_gntdev_map_grant_ref* gref = malloc(
        sizeof(struct ioctl_gntdev_map_grant_ref) +
        (nb_grant - 1) * sizeof(struct ioctl_gntdev_grant_ref));
    gref->count = nb_grant;
    gref->refs[0].domid = domid;
    gref->refs[0].ref = refid;

    int err = ioctl(gntdev_fd, IOCTL_GNTDEV_MAP_GRANT_REF, gref);
    if(err < 0){
        perror("IOCTL MAP");
        free(gref);
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    char* shbuf = mmap(NULL, nb_grant * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntdev_fd, gref->index);
    if(shbuf == MAP_FAILED){
        perror("mmap");
        free(gref);
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    struct timespec sent, received;

    for (int i = 0; i < 10; i++) {
        while (shbuf[0] != 1);

        __sync_synchronize(); // SYNC!
        memcpy(&sent, shbuf + 1, sizeof(struct timespec));
		clock_gettime(CLOCK_MONOTONIC, &received); // TEMPO!!

		// DEBUG
		printf("DEBUG Ricevente: sent_time = %ld.%09ld, recv_time = %ld.%09ld\n",
       		sent.tv_sec, sent.tv_nsec,
       		received.tv_sec, received.tv_nsec);
            
        long delta_ns = (received.tv_sec - sent.tv_sec) * 1e9 + (received.tv_nsec - sent.tv_nsec);
        printf("Ricevuto %d, latenza: %ld ns\n", i, delta_ns);

        shbuf[0] = 0;
    }

    munmap(shbuf, nb_grant * PAGE_SIZE);
    struct ioctl_gntdev_unmap_grant_ref ugref = {.index = gref->index, .count = nb_grant};
    ioctl(gntdev_fd, IOCTL_GNTDEV_UNMAP_GRANT_REF, &ugref);

    free(gref);
    close(gntdev_fd);
    return 0;
}