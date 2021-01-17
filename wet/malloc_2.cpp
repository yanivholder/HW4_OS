#include <iostream>
#include <cmath>
#include <unistd.h>
#include <cstring>

#define MAX_SIZE pow(10, 8)


typedef struct MallocMetaData {
    size_t size;
    bool is_free;
    MallocMetaData* next;
    MallocMetaData* prev;
}MMD;

MMD head = {0, false, nullptr, nullptr};
MMD* head_p = &head;

static MMD* find_first_available(size_t size, bool* really_available){
    MMD* curr = head_p;
    while (curr->next != nullptr) {
        curr = curr->next;
        if (curr->is_free == false)
            continue;
        else{ //curr is available!
            if (curr->size >= size){
                *really_available = true;
                return curr;
            } else //not enough space in curr
                continue;
        }
    }
    return curr; //means we did not find available space, return value is last node in list
}


void* smalloc(size_t size){

    bool really_available = false;
    MMD* first_available = find_first_available(size, &really_available);
    if (really_available){
        first_available->is_free = false;
        return (void*)(&first_available[1]); //skip meta data and give user the data pointer
    }
    else{ //'first_available' wasn't really available, simply last in linked list
        if(size == 0 || size > MAX_SIZE){
            return nullptr;
        }
        void* meta_address = sbrk(sizeof(MMD));
        void* data_address = sbrk(size);
        if (meta_address == (void*)(-1) || data_address == (void*)(-1)){
            return nullptr;
        } else{
            MMD* last_in_old_list = first_available; // the var's meaning changed, this is just to make code clear
            *((MMD*)(meta_address)) = MMD{size, false, nullptr, last_in_old_list}; //first available was just the last in linked list
            last_in_old_list->next = (MMD*)(meta_address);
            return data_address;
        }
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

void sfree(void* p) {
    if (p == nullptr)
        return;

    MMD* p_MMD = (MMD*)p;
    MMD* mmd_p = &p_MMD[-1];
    if (mmd_p->is_free)
        return;

    mmd_p->is_free = true;
    mmd_p->prev = head_p;
    mmd_p->next = head_p->next;
    head_p->next = mmd_p;
}

void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > MAX_SIZE)
        return nullptr;

    MMD* oldp_MMD = (MMD*)oldp;
    MMD* old_mmd_p = &oldp_MMD[-1];
    if (old_mmd_p->size > size)
        return oldp;

    void* new_adress_p = smalloc(size);
    memcpy(new_adress_p, oldp, old_mmd_p->size);
    sfree(oldp);
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

size_t _num_allocated_blocks() {
    MMD* current = head_p->next;
    size_t counter = 0;

    while (current != nullptr) {
        counter++;
        current = current->next;
    }
    return counter;
}

size_t _num_allocated_bytes() {
    MMD* current = head_p->next;
    size_t counter = 0;

    while (current != nullptr) {
        counter += current->size;
        current = current->next;
    }
    return counter;
}

size_t _num_meta_data_bytes() {
    return _num_allocated_blocks() * sizeof(MMD);
}

size_t _size_meta_data() {
    return sizeof(MMD);
}
