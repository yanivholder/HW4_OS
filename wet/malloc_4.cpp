#include <cmath>
#include <unistd.h>
#include <cstring>
#include <cassert>

#define MAX_SIZE pow(10, 8)
#define MIN_DATA_SIZE 128

/* ADD
 *
 * in smalloc only allocate in mult of 8
 *
 * in split... only split in mult of 8
 *
 * In the beginning of smalloc and split insert:
 * align_size(&size);

*/


typedef struct MallocMetaData {
    size_t size;
    bool is_free;
    MallocMetaData* next;
    MallocMetaData* prev;
}MMD;

MMD sbrk_head = {0, false, nullptr, nullptr};
MMD* sbrk_head_p = &sbrk_head;

void align_size(size_t* size) {
    if (*size % 8 != 0)
        *size = *size + 8 - (*size % 8);
}

static MMD* find_first_available(size_t size, bool* really_available) {
    assert(size % 8 == 0);
	MMD* curr = sbrk_head_p;
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
    assert(size % 8 == 0);
    if (old_big_block->size < size + MIN_DATA_SIZE + sizeof(MMD))
        return;

    MMD splitter = {old_big_block->size - size - sizeof(MMD), true, old_big_block->next, old_big_block};
    MMD* splitter_address = (MMD*)((char*)old_big_block + sizeof(MMD) + size);
    *splitter_address = splitter;

    old_big_block->next = splitter_address;
    old_big_block->size = size;

}


void* smalloc(size_t size) {
    align_size(&size);

    if(size == 0 || size > MAX_SIZE){
        return nullptr;
    }
    bool really_available = false;
    MMD* first_available = find_first_available(size, &really_available);
    if (really_available){
        split_if_big_enough(first_available, size); // challenge 1 solution
        first_available->is_free = false;
        return (void*)(&first_available[1]); // skip meta data and give user the data pointer
    }
    else { // 'first_available' wasn't really available, simply last in linked list
        if (first_available->is_free) { // challenge 3 solution
            assert(first_available->size < size);
            if (sbrk(size - first_available->size) == (void*)(-1))
                return nullptr;
            first_available->size = size;
            return (void*)(&first_available[1]);
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

void* scalloc(size_t num, size_t size){
    align_size(&size);
    void* address = smalloc(num*size);
    if (address == nullptr){
        return nullptr;
    }
    memset(address, 0, num*size);
    return address;
}

MMD* _sfree_adjacents_next(MMD* mmd_p) {
    if (mmd_p->next->is_free) {
        mmd_p->size += sizeof(MMD) + mmd_p->next->size;
        mmd_p->next = mmd_p->next->next;
        mmd_p->next->prev = mmd_p; // mmd_p->next is already the one after the original next
    }
    return mmd_p;
}

MMD* _sfree_adjacents_prev(MMD* mmd_p) {
    if (mmd_p->prev->is_free) {
        mmd_p->prev->size += sizeof(MMD) + mmd_p->size;
        mmd_p->prev->next = mmd_p->next;
        mmd_p->next->prev = mmd_p->prev;
        return mmd_p->prev;
    }
    else
        return mmd_p;
}

MMD* _sfree_adjacents(MMD* mmd_p) {
    _sfree_adjacents_next(mmd_p);
    return _sfree_adjacents_prev(mmd_p);
}

void sfree(void* p) {
    if (p == nullptr)
        return;

    MMD* mmd_p = &((MMD*)p)[-1];
    if (mmd_p->is_free)
        return;

    mmd_p->is_free = true;
    _sfree_adjacents(mmd_p);
}

MMD* _srealloc_merge(MMD* old_mmd_p, size_t size, void* res) {
	assert(size % 8 == 0);
    if (old_mmd_p->prev->is_free && old_mmd_p->size + old_mmd_p->prev->size > size) {
        return _sfree_adjacents_prev(old_mmd_p);
    }
    else {
        if (old_mmd_p->next->is_free && old_mmd_p->size + old_mmd_p->next->size > size) {
            return _sfree_adjacents_next(old_mmd_p);
        }
        else {
            MMD* merged_mmd_p = _sfree_adjacents_prev(_sfree_adjacents_next(old_mmd_p));
            if (merged_mmd_p->size < size) {
                res = nullptr;
            }
            return merged_mmd_p;
        }
    }
}

void* srealloc(void* oldp, size_t size) {
    align_size(&size);
    if (size == 0 || size > MAX_SIZE)
        return nullptr;

    MMD* old_mmd_p = ((MMD*)oldp)-1;
    if (old_mmd_p->size >= size)
        return oldp;

    void* res;
    MMD* merged_mmd_p = _srealloc_merge(old_mmd_p, size, res);
    if (res != nullptr) {
        memcpy(merged_mmd_p+1, oldp, old_mmd_p->size);
        split_if_big_enough(merged_mmd_p, size);
        return (void*)(merged_mmd_p+1);
    }
    else
        merged_mmd_p->is_free = true;

    void* new_adress_p = smalloc(size);
    memcpy(new_adress_p, oldp, old_mmd_p->size);
    old_mmd_p->is_free = true;
}

size_t _num_free_blocks() {
    MMD* current = sbrk_head_p->next;
    size_t counter = 0;

    while (current != nullptr) {
        if (current->is_free)
            counter++;
        current = current->next;
    }
    return counter;
}

size_t _num_free_bytes() {
    MMD* current = sbrk_head_p->next;
    size_t counter = 0;

    while (current != nullptr) {
        if (current->is_free)
            counter += current->size;
        current = current->next;
    }
    return counter;
}

size_t _num_allocated_blocks() {
    MMD* current = sbrk_head_p->next;
    size_t counter = 0;

    while (current != nullptr) {
        counter++;
        current = current->next;
    }
    return counter;
}

size_t _num_allocated_bytes() {
    MMD* current = sbrk_head_p->next;
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
