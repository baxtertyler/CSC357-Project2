#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

typedef unsigned char byte;
typedef unsigned int uint;

typedef struct chunckhead {
    uint size, info;
    byte *next, *prev;
} chunkhead;

#define PAGESIZE 4096
#define CHSIZE sizeof(chunkhead)

chunkhead* get_last_chunk();
byte *mymalloc(uint s);
void myfree(byte *addy);
void analyze();
void splitChunk(chunkhead*, uint);

void* startofheap = NULL;

int main() {
    struct timeval start, end;   
    int i;
    float clock_diff;
    byte*a[100];
    gettimeofday(&start, NULL);

    analyze();
    for(i = 0; i < 100; i++) {
        a[i] = mymalloc(1000);
    }
    for(i = 0; i < 90; i++) {
        myfree(a[i]);
    }
    /*analyze();*/
    myfree(a[95]);
    
    myfree(a[94]);
    analyze();
    
    a[94] = mymalloc(1000);
    analyze();
    a[95] = mymalloc(1000);
    analyze();
    for(i = 90; i < 100; i++) {
        myfree(a[i]);
    }
    analyze();
    printf("\n\n");

    gettimeofday(&end, NULL);

    clock_diff = end.tv_usec - start.tv_usec;
    printf("Time to run: %f\n\n", clock_diff);
    return 0;
}

/* finds the last chunk, MODIFIED FROM PROJ SPECS */
chunkhead* get_last_chunk() {
    chunkhead* ch;
    if(!startofheap)
        return NULL;
    ch = (chunkhead*)startofheap;
    for (; ch->next; ch = (chunkhead*)ch->next);
    return ch;
}

byte *mymalloc(uint s) {
    chunkhead *c0, *c1;
    int* location;

    /* make sure s is a multiple of page size*/
    s = s + CHSIZE;
    if (!(s % PAGESIZE == 0)) {
        s = ((s / PAGESIZE)+1) * PAGESIZE;
    }
    if (s < PAGESIZE) {
        s = PAGESIZE;
    }

    /* if start of heap is not initialized, initialize it */
    if (startofheap == NULL) {
        startofheap = sbrk(PAGESIZE);
        ((chunkhead*)startofheap)->next = NULL;
        ((chunkhead*)startofheap)->prev = NULL;
        ((chunkhead*)startofheap)->info = 0;
        ((chunkhead*)startofheap)->size = PAGESIZE;
    }

    c1 = NULL;
    /* find best fit ~if possible */
    c0 = startofheap;
    if (c0->next != NULL) {
        c0 = ((chunkhead*)c0->next);
    }
    while (c0 != NULL) {
        if (c0->info == 0 && c0->size+1 >= s) {
            if (c0->size - s == 0) {
                c0->info = 1;
            }
            else {
                splitChunk(c0, s);
            }
            return (byte*)c0 + CHSIZE;
        }
        c0 = ((chunkhead*)c0->next);
    }
    
    /* allocate room for the next chunk since there was not room */
    c1 = get_last_chunk();
    location = sbrk(s);
    if (location == NULL) {
        printf("FAILED TO INCREASE HEAP: OUT OF MEMORY!\n");
        return NULL;
    }

    /* add the chunk to the end of the heap */
    c1->next = (byte*)location;
    ((chunkhead*)c1->next)->prev = (byte*)c1;
    c1 = (chunkhead*)c1->next;
    c1->next = NULL;
    c1->info = 1;
    c1->size = s;

    return (byte*)c1 + CHSIZE;
}

void splitChunk(chunkhead* c, uint s) {
    chunkhead* c1 = (chunkhead*)((byte*)c + s);
    c1->size = c->size - (s);
    c1->info = 0;
    c1->next = c->next;
    c1->prev = (byte*)c;
    if (c->next != NULL) {
        (((chunkhead*)c1->next)->prev) = (byte*)c1;
    }
    c->size = s;
    c->next = (byte*)c1;
    c->info = 1;
}

void myfree(byte *addy) {
    chunkhead *c0, *c1;
    void* brk;
    int s;

    /* get the requested chunkhead and set its info to 0, meaning it is now free */
    c0 = (chunkhead*)addy - 1;
    c0->info = 0;

    /* merge with chunk before ~if possible */
    if (c0->prev != NULL && ((chunkhead*)c0->prev)->info == 0) {
        ((chunkhead*)c0->prev)->size += c0->size;
        ((chunkhead*)c0->prev)->next = c0->next;
        if (c0->next != NULL) {
            ((chunkhead*)c0->next)->prev = c0->prev;
        }
        c0 = ((chunkhead*)c0->prev);
    }

    /* merge with chunk after ~if possible */
    if (c0->next != NULL && ((chunkhead*)c0->next)->info == 0) {
        c0->size += ((chunkhead*)c0->next)->size;
        c0->next = ((chunkhead*)c0->next)->next;
        if (((chunkhead*)c0->next)->next != NULL) {
            ((chunkhead*)((chunkhead*)c0->next)->next)->prev = (byte*)c0;
        }
    }

    /* if my free was called on last chunk, shrink heap! */
    c1 = get_last_chunk();
    if (c1 == c0) {
        s = (c0->size) * -1;
        brk = sbrk(s);
        if (brk == NULL) {
            printf("HEAP SHRINK WITH MYFREE WAS UNSUCCESSFUL!\n");
            exit(1);
        }
        startofheap = NULL;
    }
}

/* analyse function, MODIFIED FROM PROJECT SPECS */
void analyze() {                                                      
    chunkhead* ch;
    int no;
    printf("\n----------------------------------------------------------\n");
    if(!(startofheap)) {
        printf("no heap, program break on address: %p\n", sbrk(0));
        return;
    }
    ch = (chunkhead*)startofheap;
    for (no = 0; ch; ch = (chunkhead*)ch->next, no++) {
        printf("%d | current addr: %x |", no, ((uint)(intptr_t)ch));
        printf("size: %d | ", ch->size);
        printf("info: %d | ", ch->info);
        printf("next: %x | ", ((uint)((intptr_t)ch->next)));
        printf("prev: %x", ((uint)((intptr_t)ch->prev)));
        printf("       \n");
    }
    printf("program break on address: %x\n", ((uint)(intptr_t)sbrk(0)));
}