#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <xen/grant_table.h>
#include <xen/gntdev.h>

#define PAGE_SIZE getpagesize()

//passa come parametri domuA_ID e grant_ref
int main(int argc, char ** argv){
    int i;
    uint32_t nb_grant = 1;
    uint16_t domid;
    uint32_t* refid = malloc(sizeof(uint32_t) * nb_grant);

    domid = strtoul(argv[1], NULL, 10);
    refid[0] = strtoul(argv[2], NULL, 10);

    //aprire file descriptor sul nodo gntdev
    int gntdev_fd = open("/dev/xen/gntdev", O_RDWR);
    if(gntdev_fd < 0){
        fprintf(stderr, "Couldn't open /dev/xen/gntdev\n");
        exit(EXIT_FAILURE);
    }

    //allocazione struttura che serve a comunicare al kernel quali grant vogliamo mappare (allochaimo nb_grant elementi - 1, perchè perche la struct già ne include uno) 
    struct ioctl_gntdev_map_grant_ref* gref = malloc(sizeof(struct ioctl_gntdev_map_grant_ref) + (nb_grant-1) * sizeof(struct ioctl_gntdev_grant_ref));
    if(gref == NULL){
        fprintf(stderr, "Couldn't allocate struct ioctl_gntdev_map_grant_ref\n");
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    //popolamento con i grant ricevuti dal mittente
    gref->count = nb_grant;
    for(i = 0; i < nb_grant; i++){
        struct ioctl_gntdev_grant_ref* ref = &gref->refs[i];
        ref->domid = domid;
        ref->ref = refid[i];
    }

    //chiamata al driver per far mappare pagine esterne nel proprio processo
    int err = ioctl(gntdev_fd, IOCTL_GNTDEV_MAP_GRANT_REF, gref);
    if(err < 0){
        fprintf(stderr, "IOCTL failed\n");
        free(gref); free(refid);
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    //ottenimento di un puntatore userspace per avere accesso alla memoria lato domU
    char* shbuf = mmap(NULL, nb_grant*PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntdev_fd, gref->index);
    if(shbuf == MAP_FAILED){
        fprintf(stderr, "Failed mapping the grant references\n");
        free(gref); free(refid);
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    //printf("%s\n", shbuf);
    //fwrite(shbuf, 1, 1024, stdout);
    //printf("\n");
    
    for (int i = 0; i < 10; i++) {
    // Aspetto che ci sia un messaggio da leggere
    while (shbuf[0] != 1);

    // Leggo il messaggio
    printf("Ricevente ha ricevuto: %s\n", shbuf + 1);

    // Resetto il flag per permettere al mittente di scrivere il prossimo
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
