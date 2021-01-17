#include <iostream>
#include "headers.h"

int main() {
    std::cout << "Hello, World!" << std::endl;

    void* v1 = (void*)(-1);
    void* v2 = (void*)(-1);

    bool b = v1==v2;

    void* v3 = smalloc(-1);



    int* address = (int*)smalloc(sizeof(int)*2);

    return 0;
}