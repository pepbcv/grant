#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>

#include <xen/grant_table.h>
#include <xen/gntdev.h>
#include <xen/event_channel.h> // EVENT!!!!!!!

#define PAGE_SIZE getpagesize()

int main(int argc, char ** argv){
    if(argc < 4){
        fprintf(stderr, "Uso: %s <domid_mittente> <grant_ref> <port_evtchn>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    uint32_t nb_grant = 1;
    uint16_t domid = atoi(argv[1]);
    uint32_t* refid = malloc(sizeof(uint32_t) * nb_grant);
    refid[0] = atoi(argv[2]);
    uint32_t remote_port = atoi(argv[3]); // EVENT!!!!!!!

    int gntdev_fd = open("/dev/xen/gntdev", O_RDWR);
    if(gntdev_fd < 0){
        fprintf(stderr, "Couldn't open /dev/xen/gntdev\n");
        exit(EXIT_FAILURE);
    }

    struct ioctl_gntdev_map_grant_ref* gref = malloc(sizeof(*gref) + (nb_grant-1)*sizeof(struct ioctl_gntdev_grant_ref));
    if(gref == NULL){
        fprintf(stderr, "Couldn't allocate map struct\n");
        close(gntdev_fd);
        exit(EXIT_FAILURE);
    }

    gref->count = nb_grant;
    for(int i=0; i<nb_grant; i++){
        gref->refs[i].domid = domid;
        gref->refs[i].ref = refid[i];
    }

    int err = ioctl(gntdev_fd, IOCTL_GNTDEV_MAP_GRANT_REF, gref);
    if(err < 0){
        perror("GNTDEV ioctl failed");
        exit(EXIT_FAILURE);
    }

    char* shbuf = mmap(NULL, nb_grant * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntdev_fd, gref->index);
    if(shbuf == MAP_FAILED){
        fprintf(stderr, "Failed to map grant\n");
        exit(EXIT_FAILURE);
    }

    // EVENT!!!!!!! — Ricezione notifica
    int evtchn_fd = open("/dev/xen/evtchn", O_RDWR);
    if(evtchn_fd < 0){
        perror("Couldn't open evtchn");
        exit(EXIT_FAILURE);
    }

    struct evtchn_bind_interdomain bind = {
        .remote_dom = domid,
        .remote_port = remote_port
    };
    ioctl(evtchn_fd, IOCTL_EVTCHN_BIND_INTERDOMAIN, &bind);
    printf("In attesa su porta locale %u\n", bind.local_port);

    struct pollfd pfd = {.fd = evtchn_fd, .events = POLLIN};
    poll(&pfd, 1, -1); // attesa bloccante
    uint32_t dummy;
    read(evtchn_fd, &dummy, sizeof(dummy));
    printf("Notifica ricevuta.\n");

    // Leggi messaggio
    fwrite(shbuf, 1, 1024, stdout);
    printf("\n");

    // Cleanup
    munmap(shbuf, nb_grant * PAGE_SIZE);
    struct ioctl_gntdev_unmap_grant_ref ugref = {.index = gref->index, .count = nb_grant};
    ioctl(gntdev_fd, IOCTL_GNTDEV_UNMAP_GRANT_REF, &ugref);

    free(gref);
    free(refid);
    close(gntdev_fd);
    close(evtchn_fd);
    return 0;
}

