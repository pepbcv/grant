#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/ioctl.h>
#include <xen/grant_table.h>

#define PAGE_SIZE 4096

int main() {
    int gnt_fd = open("/dev/xen/gntdev", O_RDWR);
    if (gnt_fd < 0) {
        perror("Errore apertura /dev/xen/gntdev");
        return 1;
    }

    // Alloca una pagina di memoria condivisibile
    void *shared_page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, gnt_fd, 0);
    if (shared_page == MAP_FAILED) {
        perror("mmap fallita");
        close(gnt_fd);
        return 1;
    }

    // Richiede un grant reference
    struct ioctl_gntdev_alloc_gref alloc = {
        .domid = 3,  // da rimpiazzare con DomU-B ID (es: 6)
        .count = 1
    };
    struct ioctl_gntdev_gref gref;

    alloc.gref_ids = (uintptr_t)&gref;
    if (ioctl(gnt_fd, IOCTL_GNTDEV_ALLOC_GREF, &alloc) == -1) {
        perror("Errore nell'allocazione grant");
        munmap(shared_page, PAGE_SIZE);
        close(gnt_fd);
        return 1;
    }

    printf("Grant ref assegnato: %d\n", gref.gref_id);

    // Scrive un messaggio nella pagina
    snprintf((char *)shared_page, PAGE_SIZE, "Hello from DomU-A");

    // Scrive il grant ref su file per DomU-B
    FILE *f = fopen("grant.txt", "w");
    if (f) {
        fprintf(f, "%d\n", gref.gref_id);
        fclose(f);
        printf("Grant ref scritto su grant.txt\n");
    }

    // [Placeholder] Setup event channel in futuro
    // ...

    printf("Messaggio scritto. Rimane attivo per debugging...\n");
    sleep(10);

    munmap(shared_page, PAGE_SIZE);
    close(gnt_fd);
    return 0;
}
