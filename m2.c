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

//passa domuB_ID 
int main(int argc, char ** argv){
    uint16_t domid; //We set to share these grants with Dom0 (domID della vm a cui si vuole dare accesso, cioè domuB_id)
    uint32_t count = 1; //We want to allocate one grant/page

    domid = strtoul(argv[1], NULL, 10);

    //apertura file descriptor sul nodo driver
    int gntalloc_fd = open("/dev/xen/gntalloc", O_RDWR);
    if(gntalloc_fd < 0){
        fprintf(stderr, "Couldn't open /dev/xen/gntalloc\n");
        exit(EXIT_FAILURE);
    }

    //allocazione dinamica struttura che contiene i parametri necessari per la chiamata ioctl (che alloca grant references)
    struct ioctl_gntalloc_alloc_gref* gref = malloc(sizeof(struct ioctl_gntalloc_alloc_gref));
    if(gref == NULL){
        fprintf(stderr, "Couldn't allocate struct ioctl_gntalloc_alloc_gref\n");
        close(gntalloc_fd);
        exit(EXIT_FAILURE);
    }
    //la struttura una volta allocata, viene popolata
    gref->domid = domid; //domID di chi si vuole venga concessa la pagina
    gref->count = count; //numero di grant references
    gref->flags = GNTALLOC_FLAG_WRITABLE;

    //Invio richiesta al driver gntalloc (con questo comando effettivamente si alloca la pagina e si ottiene il riferimento)
    int err = ioctl(gntalloc_fd, IOCTL_GNTALLOC_ALLOC_GREF, gref);
    if(err < 0){
        fprintf(stderr, "IOCTL failed\n");
        free(gref);
        close(gntalloc_fd);
        exit(EXIT_FAILURE);
    }

    printf("gref = %u\n", gref->gref_ids[0]);

    //chiamata map: mappa la memoria tra kernel e spazio utente, cioè con questo comando (a cui passo le info ottenute da ioctl) ottengo un riferimento utilizzabile dalla domU
    char* shpages = mmap(NULL, count*PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gntalloc_fd, gref->index);
    if(shpages == MAP_FAILED){
        fprintf(stderr, "mapping the grants failed\n");
        free(gref);
        close(gntalloc_fd);
        exit(EXIT_FAILURE);
    }

    //sprintf(shpages, "Hello, World!");
    //memset(shpages, 'A', 1023);
    //shpages[1023] = '\0';
    
    for (int i = 0; i < 10; i++) {
    // Aspetto che il ricevente abbia letto il messaggio precedente
    while (shpages[0] != 0);

    // Scrivo il messaggio
    snprintf(shpages + 1, 1023, "Messaggio numero %d", i);

    // Imposto il flag per notificare il ricevente
    shpages[0] = 1;
    printf("Mittente ha inviato: Messaggio numero %d\n", i);
	}
    

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
