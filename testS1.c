#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Definizione locale (manuale) per allocazione grant reference
#define IOCTL_GNTDEV_ALLOC_GREF _IOWR('G', 0, struct ioctl_gntdev_alloc_gref)

struct ioctl_gntdev_alloc_gref {
    uint16_t domid;      // ID del dominio ricevente
    uint16_t flags;      // Non usato per ora
    uint32_t count;      // Numero di grant refs da allocare
    uint64_t index;      // Offset per mmap()
    uint32_t gref_ids[1]; // Grant ref ID generati
};

int main() {
    int gnt_fd = open("/dev/xen/gntdev", O_RDWR);
    if (gnt_fd < 0) {
        perror("Errore apertura /dev/xen/gntdev");
        return 1;
    }

    struct ioctl_gntdev_alloc_gref alloc;
    memset(&alloc, 0, sizeof(alloc));
    alloc.domid = 6;  // ← DomU ricevente
    alloc.count = 1;

    if (ioctl(gnt_fd, IOCTL_GNTDEV_ALLOC_GREF, &alloc) == -1) {
        perror("Errore ioctl GNTDEV_ALLOC_GREF");
        close(gnt_fd);
        return 1;
    }

    printf("Grant reference allocato: gref_id = %u\n", alloc.gref_ids[0]);
    printf("Offset per mmap: %lu\n", alloc.index);

    close(gnt_fd);
    return 0;
}

