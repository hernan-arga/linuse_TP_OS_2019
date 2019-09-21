#include "libmuse.h"
#include <stdlib.h>
#include <string.h>

int muse_init(int id, char* ip, int puerto){ //Case 1
    return 0;
}

void muse_close(){ /* Does nothing :) */ } //Case 2

uint32_t muse_alloc(uint32_t tam){ //Case 3
    return (uint32_t) malloc(tam);


    //musemaloc(tam)
}

void muse_free(uint32_t dir) { //Case 4
    free((void*) dir);
}

int muse_get(void* dst, uint32_t src, size_t n){ //Case 5
    memcpy(dst, (void*) src, n);
    return 0;
}

int muse_cpy(uint32_t dst, void* src, int n){//Case 6
    memcpy((void*) dst, src, n);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
uint32_t muse_map(char *path, size_t length, int flags){ //Case 7
    return 0;
}

int muse_sync(uint32_t addr, size_t len){ //Case 8
    return 0;
}

int muse_unmap(uint32_t dir){ //Case 9
    return 0;
}
