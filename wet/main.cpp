#include <iostream>
#include "malloc_4.h"


int main() {
    std::cout << "Hello, World!" << std::endl;

    void* v1 = smalloc(100);
    void* v2 = smalloc(200);
    void* v3 = smalloc(300);
    srealloc(v3, 20);

    sfree(v2);
    void* v4 = smalloc(5);
    sfree(v1);




    return 0;
}