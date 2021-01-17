#include <iostream>
#include "malloc_2.h"

typedef struct MallocMetaData {
    size_t size;
    bool is_free;
    MallocMetaData* next;
    MallocMetaData* prev;
}MMD;

int main() {
    std::cout << "Hello, World!" << std::endl;

    MMD a;
    size_t size = sizeof(a);

    void* v1 = (void*)(-1);
    void* v2 = (void*)(-1);

    bool b = v1==v2;

    void* v3 = smalloc(-1);



    int* address = (int*)smalloc(sizeof(int)*2);

    return 0;
}