
#ifndef HW4_MALLOC_4_H
#define HW4_MALLOC_4_H

void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _num_meta_data_bytes();
size_t _size_meta_data();

#endif //HW4_MALLOC_4_H

/* ADD
 *
 * in smalloc only allocate in mult of 8
 *
 * in split... only split in mult of 8
*/