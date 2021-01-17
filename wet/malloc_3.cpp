#include <cmath>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>


#define MAX_SIZE pow(10, 8)
#define MIN_DATA_SIZE 128
#define MMAP_THRESH 131072 // == pow(2, 17) == 128kb


typedef struct MallocMetaData {
    size_t size;
    bool is_free;
    MallocMetaData* next;
    MallocMetaData* prev;
}MMD;

MMD head = {0, false, nullptr, nullptr};
MMD* head_p = &head;

MMD mmap_head = {0, false, nullptr, nullptr};
MMD* mmap_head_p = &mmap_head;

static MMD* find_first_available(size_t size, bool* really_available) {
    MMD* curr = head_p;
    while (curr->next != nullptr) {
        curr = curr->next;
        if (curr->is_free == false)
            continue;
        else { //curr is available!
            if (curr->size >= size){
                *really_available = true;
                return curr;
            }
        }
    }
    return curr; //means we did not find available space, return value is last node in list
}

void split_if_big_enough(MMD* old_big_block, size_t size){ // challenge 1 solution
    if (old_big_block->size < size + MIN_DATA_SIZE + sizeof(MMD))
        return;

    MMD* splitter_address = (MMD*)((char*)old_big_block + sizeof(MMD) + size);
    MMD splitter = {old_big_block->size - size - sizeof(MMD), true, old_big_block->next, old_big_block};
    *splitter_address = splitter;

    old_big_block->next = splitter_address;
    old_big_block->size = size;

}

bool is_mmap_allocated(MMD* meta){
    return meta->size > MMAP_THRESH;
}

MMD* last_in_list(MMD* list_head){
    MMD* curr = list_head;
    while (curr->next != nullptr) {
        curr = curr->next;
    }
    return curr;
}

void mmap_list_append(void* meta_address, size_t size){
    MMD* last = last_in_list(mmap_head_p);
    MMD* new_node_address = ((MMD*)meta_address);
    *new_node_address = MMD{size, false, nullptr, last};
    last->next = new_node_address;
}

void* smalloc_helper_sbrk(size_t size){
    bool really_available = false;
    MMD* first_available = find_first_available(size, &really_available);
    if (really_available){
        split_if_big_enough(first_available, size); // challenge 1 solution
        first_available->is_free = false;
        return (void*)(first_available+1); // skip meta data and give user the data pointer
    }
    else { // 'first_available' wasn't really available, simply last in linked list
        if (first_available->is_free) {
            assert(first_available->size < size);
            if (sbrk(size - first_available->size) == (void*)(-1))
                return nullptr;
            first_available->size = size;
            return (void*)(first_available+1);
        }
        else {
            void* meta_address = sbrk(sizeof(MMD));
            if (meta_address == (void*)(-1))
                return nullptr;
            void* data_address = sbrk(size);
            if (data_address == (void*)(-1)) {
                sbrk(-sizeof(MMD)); // cancel prev brk
                return nullptr;
            }
            MMD* last_in_old_list = first_available; // the var's meaning changed, this is just to make code clear
            *((MMD*)meta_address) = MMD{size, false, nullptr, last_in_old_list}; //first available was just the last in linked list
            last_in_old_list->next = (MMD*)meta_address;
            return data_address;
        }
    }
}

void* smalloc_helper_mmap(size_t size){
    void* meta_address = mmap(nullptr, size + sizeof(MMD),  PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if (meta_address == (void*)(-1))
        return nullptr;
    mmap_list_append(meta_address, size);
    void* data_address = (void*)((MMD*)meta_address + 1);
    return data_address;

}

void* smalloc(size_t size) {
    if(size == 0 || size > MAX_SIZE){
        return nullptr;
    }
    if (size < MMAP_THRESH){
        return smalloc_helper_sbrk(size);
    }
    else{
        return smalloc_helper_mmap(size);
    }

}

void* scalloc(size_t num, size_t size){
    void* address = smalloc(num*size);
    if (address == nullptr){
        return nullptr;
    }
    memset(address, 0, num*size);
    return address;
}

void _sfree_adjacents(MMD* mmd_p) {
    if (mmd_p->next->is_free) {
        mmd_p->size += sizeof(MMD) + mmd_p->next->size;
        mmd_p->next = mmd_p->next->next;
        mmd_p->next->prev = mmd_p; // mmd_p->next is already the one after the original next
    }
    if (mmd_p->prev->is_free) {
        mmd_p->prev->size += sizeof(MMD) + mmd_p->size;
        mmd_p->prev->next = mmd_p->next;
        mmd_p->next->prev = mmd_p->prev;
    }
}

static void remove_from_list(MMD* list_obj) {
    MMD *next = list_obj->next;
    MMD *prev = list_obj->prev;

    prev->next = next;
    if (next!= nullptr)
        next->prev = prev;
}

void sfree(void* p) {
    if (p == nullptr)
        return;

    MMD* mmd_p = &((MMD*)p)[-1];
    if (mmd_p->is_free)
        return;

    if(mmd_p->size < MMAP_THRESH){
        mmd_p->is_free = true;
        _sfree_adjacents(mmd_p);
    }
    else{
        remove_from_list(mmd_p);
        size_t mapped_size = mmd_p->size+ sizeof(MMD);
        if (munmap(mmd_p, mapped_size) == -1){
            perror("munmap");
            _exit(1);
        }
    }

}

void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > MAX_SIZE)
        return nullptr;

    MMD* old_mmd_p = &((MMD*)oldp)[-1];
    if (old_mmd_p->size > size) // TODO check if also equal
        return oldp;

    void* new_adress_p = smalloc(size);
    memcpy(new_adress_p, oldp, old_mmd_p->size);
    old_mmd_p->is_free = true;
}

size_t _num_free_blocks() {
    MMD* current = head_p->next;
    size_t counter = 0;

    while (current != nullptr) {
        if (current->is_free)
            counter++;
        current = current->next;
    }
    return counter;
}

size_t _num_free_bytes() {
    MMD* current = head_p->next;
    size_t counter = 0;

    while (current != nullptr) {
        if (current->is_free)
            counter += current->size;
        current = current->next;
    }
    return counter;
}

static size_t mmd_list_len_by_head(MMD* list_head){
    MMD* current = list_head;
    size_t counter = 0;
    while (current->next != nullptr) {
        counter++;
        current = current->next;
    }
    return counter;
}

size_t _num_allocated_blocks() {
    return mmd_list_len_by_head(head_p) + mmd_list_len_by_head(mmap_head_p);
}

static size_t mmd_list_allocated_bytes(MMD* list_head){
    MMD* current = list_head->next;
    size_t counter = 0;

    while (current != nullptr) {
        counter += current->size;
        current = current->next;
    }
    return counter;
}

size_t _num_allocated_bytes() {
    return mmd_list_allocated_bytes(head_p) + mmd_list_allocated_bytes(mmap_head_p);
}

size_t _num_meta_data_bytes() {
    return _num_allocated_blocks() * sizeof(MMD);
}

size_t _size_meta_data() {
    return sizeof(MMD);
}
