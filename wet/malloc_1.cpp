#include <cmath>
#include <unistd.h>

#define MAX_SIZE pow(10, 8)

//void* smalloc(size_t size)
//● Tries to allocate ‘size’ bytes.
//● Return value:
//i. Success –a pointer to the first allocated byte within the allocated block.
//ii. Failure –
//a. If ‘size’ is 0 returns NULL.
//b. If ‘size’ is more than 10 , return NULL.
//8
//c. If sbrk fails, return NULL.

void* smalloc(size_t size){
    if(size == 0 || size > MAX_SIZE){
        return nullptr;
    }
    void* address = sbrk(size);
    if (address == (void*)(-1)){
        return nullptr;
    } else{
        return address;
    }
}

//maybe keep vector of freed spaces by start and length and when allocating try to use those