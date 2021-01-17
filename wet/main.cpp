#include <iostream>
#include "malloc_3.h"


int main() {
    std::cout << "Hello, World!" << std::endl;

    void* v1 = smalloc(10);
    void* v2 = smalloc(20);
    void* v3 = smalloc(30);

    sfree(v2);
    void* v4 = smalloc(5);
    sfree(v1);




    return 0;
}