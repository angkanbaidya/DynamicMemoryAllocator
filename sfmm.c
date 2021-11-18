/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include <errno.h>

#define PUT(p,val) (*((size_t *)(p)) = (val))
#define PACK(i,curr_prev_alloc) ((i) | (curr_prev_alloc))
sf_block* lookthrough(sf_block* head, size_t size);
sf_block* traverse(size_t size);
void insert_into_list(sf_block* test, sf_block* half);
sf_block* coalesce(sf_block* block, sf_block* second);
sf_header *returnHeader(sf_block *pointer){
	return &pointer->header;
}

int init_func(size_t size){
    for (int i = 0; i < NUM_FREE_LISTS;i++){ //initialize free lists
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }

    if (sf_mem_grow() == NULL){
        sf_errno = ENOMEM;
        return -1;
    }

    char* starting = (char*) sf_mem_start();
    starting = starting + 8;
    sf_block *prologue = (sf_block*) starting;
    prologue -> header = 32;
    prologue->header |= THIS_BLOCK_ALLOCATED;

    char* address_for_wilderness = starting + 32;
    sf_block *wilderness = (sf_block*) address_for_wilderness;
    wilderness->header = (8192-48); // set those bits to sizeer
    size_t *footer = (void*) sf_mem_end()- 16;
    *footer = wilderness->header;

    char *ending = (char*) sf_mem_end();
    ending = ending - 8;
    sf_block *epilogue = (sf_block*)ending;
    epilogue->header = THIS_BLOCK_ALLOCATED;


    sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = wilderness;
    sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = wilderness;;
    wilderness->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];
    wilderness->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];

    return 0;
 }

sf_block* traverse(size_t size){

   sf_block* head;
   sf_block* test;

    for(int i = 0; i< NUM_FREE_LISTS ; i++){
        head = &sf_free_list_heads[i];
        test = lookthrough(head,size);
        if (test != NULL){
            return test;

        }
    }

    return NULL;
 }

 sf_block*  grow_heap(){
    char* old_ep = (char*)sf_mem_end();
    old_ep = old_ep - 8; //ADDRESS OF THE OLD EPILOGUE BEFORE MEM GROW
    sf_block* old_epilogue_block = (sf_block*) old_ep; // NEW WILDERNESS WILL WHERE THE OLD EPILOGUE IS
    old_epilogue_block->header &= (~THIS_BLOCK_ALLOCATED);

    sf_block* old_widlerness_block = &sf_free_list_heads[NUM_FREE_LISTS-1]; // THIS IS WHERE THE OLD WILDERNESS ADDRESS IS
     size_t sizeofoldwilderness = old_widlerness_block->header & (~0xf); //SIZE OF THE OLD WILDERNESS



    sf_block* growth = sf_mem_grow(); //MEM GROW
    if(growth == NULL){
        sf_errno = ENOMEM;
        return NULL;
    }
    old_widlerness_block->header = sizeofoldwilderness +8192; //NEW WILDERNESS HEADER
    sf_block* footer = (sf_block*)((char*)old_widlerness_block + (old_widlerness_block->header & (~0xf)) - 8);
    footer->header = old_widlerness_block->header; //SET NEW WILDERNESS FOOTER
    old_widlerness_block->header |= PREV_BLOCK_ALLOCATED;

    char* epilogue_end = (char*) sf_mem_end();
    epilogue_end = epilogue_end - 8;
    sf_block* new_epilogue = (sf_block*) epilogue_end;
    new_epilogue->header = 0;
    new_epilogue->header |= THIS_BLOCK_ALLOCATED;



    return old_widlerness_block;
 }

 sf_block* lookthrough(sf_block* head, size_t size){
    sf_block* thehead = head;
    if (thehead->body.links.next == thehead){
        return NULL;
    }

    sf_block* tofind = thehead;
    int sizeblock = 0;

    while(tofind != NULL){
        sizeblock = tofind->header & (~0xf);
        if (sizeblock >= size){
            return tofind;//found block
        }
        tofind = tofind->body.links.next; // go to next
    }
    return NULL;
 }

 void remove_from_free(sf_block* block){
    block->body.links.next->body.links.prev = block->body.links.prev;
    block->body.links.prev->body.links.next = block->body.links.next;
    block->body.links.prev = NULL;
    block->body.links.next = NULL;
 }

 void insert_into_list(sf_block* test, sf_block* half){
    sf_block* initial = test->body.links.next;
    test->body.links.next = half;
    initial->body.links.prev = half;
    half->body.links.prev = test;
    half->body.links.next = initial;
}
sf_block* check_cases(void *pp){
    if (pp == NULL){ //case 1
        return NULL;
    }
    size_t address = (size_t)pp; //case 2
    if (address %16 != 0){
       return NULL;
    }
    char* address_char = (char*)pp;
    address_char = address_char - 8;
    sf_block* pp_block = (sf_block*) address_char;
    if ((pp_block->header & THIS_BLOCK_ALLOCATED )== 0){
        return NULL;
    }
    if((pp_block->header & (~0xf))<32){ //case 3
        return NULL;
    }
     char* starting = (char*) sf_mem_start();
    starting = starting + 40;
    sf_block* prologue = (sf_block*) starting;
    if(pp_block < prologue){
        return NULL;
    }

      char* ending = (char*) sf_mem_end();
    ending = ending - 8;
    sf_block* epilogue = (sf_block*)ending;
    size_t pp_block_size = pp_block->header & (~0xf);
    address_char = address_char+ pp_block_size - 8;
    sf_block* pp_block_footer = (sf_block*) address_char;

    if(pp_block_footer > epilogue){
        return NULL;
    }

      int previously_allocated = pp_block->header & PREV_BLOCK_ALLOCATED; //case 5
    if(previously_allocated != 2){
    char* previous_block_address =  (char*) pp_block -8;
    sf_block* previous_block =  (sf_block*)previous_block_address;
    int if_previous_allocated = previous_block->header & THIS_BLOCK_ALLOCATED;
    if(previously_allocated != if_previous_allocated){
        return NULL;
    }}

return pp_block;
}

sf_block* checkcases_realloc(void *pp, size_t rsize){
    size_t address = (size_t) pp;
    if (pp == NULL){

        return NULL;
    }
     if(address % 16 != 0){

        return NULL;
    }
     char* address_char = (char*)pp;
    address_char = address_char - 8;
    sf_block* pp_block = (sf_block*) address_char;
    if ((pp_block->header & THIS_BLOCK_ALLOCATED )== 0){
        sf_errno = EINVAL;
        return NULL;
    }
     if((pp_block->header &(~0xf))<32){
        sf_errno = EINVAL;
        return NULL;
    }
     char* starting = (char*) sf_mem_start();
    starting = starting + 40;
    sf_block* prologue = (sf_block*) starting;
    if(pp_block < prologue){
        sf_errno = EINVAL;
        return NULL;
    }

    char* ending = (char*) sf_mem_end();
    ending = ending - 8;
    sf_block* epilogue = (sf_block*)ending;
    size_t pp_block_size = pp_block->header &(~0xf);
    address_char = address_char+ pp_block_size - 8;
    sf_block* pp_block_footer = (sf_block*) address_char;

    if(pp_block_footer > epilogue){
        sf_errno = EINVAL;
        return NULL;
    }
    int previously_allocated = pp_block->header & PREV_BLOCK_ALLOCATED;
    if(previously_allocated != 2){
    char* previous_block_address =  (char*) pp_block -8;
    sf_block* previous_block =  (sf_block*)previous_block_address;
    int if_previous_allocated = previous_block->header & THIS_BLOCK_ALLOCATED;
    if(previously_allocated != if_previous_allocated){
        sf_errno = EINVAL;
        return NULL;
    }};

return epilogue;
}

 sf_block* coalesce(sf_block* block, sf_block* second){ //TODO
    size_t firstsize = block->header & (~0xf);
    size_t secondsize = second->header &(~0xf);
    remove_from_free(block);
    remove_from_free(second);
    size_t combined = firstsize + secondsize;
    int check = block->header & PREV_BLOCK_ALLOCATED;
    block->header = combined;
    if(check ==2)
        block->header |= PREV_BLOCK_ALLOCATED;
    char* address = (char*) second;
    address = address + secondsize;
    sf_block* footer =  (sf_block*)((char*)block + combined  -8);
    footer->header = block->header;
    int index;
    sf_block* result = &sf_free_list_heads[NUM_FREE_LISTS-1];
    if(block->body.links.next != &sf_free_list_heads[NUM_FREE_LISTS-1] && second->body.links.next != &sf_free_list_heads[NUM_FREE_LISTS-1]){
        int checktosee = combined;
        if (checktosee == 32)
            index = 0;
        if (checktosee > 32 && checktosee < 65)
            index = 1;
        if (checktosee >64 && checktosee <129)
            index =2;
        if (checktosee>128 && checktosee < 257)
            index =3;
        if (checktosee >256 && checktosee<513)
            index = 4;
        if(checktosee>512 && checktosee <1025)
            index = 5;
        if(checktosee> 1024)
            index = 6;
         result  = &sf_free_list_heads[index];

    }
    insert_into_list(result,block);
    return block;
 }



 sf_block* split_blocks(sf_block* block,size_t size){
    int index;
    int wearewilderness = 0;
    sf_block* wilderness_sentinel = &sf_free_list_heads[NUM_FREE_LISTS-1];
    if(block->body.links.next == wilderness_sentinel){
        wearewilderness = 1;
    }

    size_t  portion = (block->header & (~0xf)) - size;
    if (portion == 0){
        return block;
    }
    int shouldwealloc = block->header &  THIS_BLOCK_ALLOCATED;
    block->header = size;
    block->header |= THIS_BLOCK_ALLOCATED;
    int isprologue = 0;
    char* address_block_for_check = (char*)block;
    address_block_for_check = address_block_for_check - 32;
    if((sf_mem_start() + 8) == address_block_for_check){
        block->header |= PREV_BLOCK_ALLOCATED;
        isprologue =1;
    }
    if(isprologue == 0){
    block->header |= PREV_BLOCK_ALLOCATED;
    }
    if(shouldwealloc == 0){
    remove_from_free(block);}
    char* address = (char*) block;
    address = address + size;
    sf_block* half = (sf_block*) address;
    half->header = portion;
    half->header |= PREV_BLOCK_ALLOCATED;
    size_t *footer = (void*) half+portion -8;
    *footer = half->header;
    if(wearewilderness == 0){
        int checktosee = portion / 32;
        if (checktosee == 32)
            index = 0;
        if (checktosee > 32 && checktosee < 65)
            index = 1;
        if (checktosee >64 && checktosee <129)
            index =2;
        if (checktosee>128 && checktosee < 257)
            index =3;
        if (checktosee >256 && checktosee<513)
            index = 4;
        if(checktosee>512 && checktosee <1025)
            index = 5;
        if(checktosee> 1024)
            index = 6;
        wilderness_sentinel = &sf_free_list_heads[index];
        }
    insert_into_list(wilderness_sentinel,half);
    return block;
 }

 sf_block* make_space(sf_block* block_to_find){
    remove_from_free(block_to_find);
    block_to_find->header |= THIS_BLOCK_ALLOCATED;
    size_t sizeofblock = block_to_find->header & (~0xf) ;
    char* address = (char*) block_to_find;
    address = address+ sizeofblock;
    size_t *footer = (void*) block_to_find + sizeofblock- 8;
    *footer = block_to_find->header;
    return block_to_find;
 }


void *sf_malloc(size_t size) {
	if (size ==0)
    	return NULL;
    size = size + 8;

    if (size < 32)
    	size = 32;

    if (size % 16 != 0){
    	int numtoadd = 16 - (size % 16);
    	size =  size + numtoadd;
    }

    if(sf_mem_start() == sf_mem_end()){
        if(init_func(size) == -1){ //initalize the lists and all
            sf_errno = ENOMEM;
            return NULL;
        }
    }

    sf_block* block_to_find = traverse(size);
    int test= block_to_find->header & (~0xf);
    while(block_to_find == NULL){
        block_to_find = traverse(size);
    }
    if (test -size > 32){
      block_to_find = split_blocks(block_to_find,size);
    }
    else{
        make_space(block_to_find); //if block size isnt bigger than the requested size
    }

    return block_to_find->body.payload;
}


void sf_free(void *pp) {
    if(check_cases(pp) == NULL){
        abort();

    }
    char* address_char = (char*)pp;
    address_char = address_char - 8;
    sf_block* pp_block = (sf_block*) address_char;
    char* starting = (char*) sf_mem_start();
    starting = starting + 40;
    char* ending = (char*) sf_mem_end();
    ending = ending - 8;
    size_t pp_block_size = pp_block->header & (~0xf);
    address_char = address_char+ pp_block_size - 8;


    address_char = (char*) pp_block;
    address_char = address_char + pp_block_size;
    int previously = pp_block->header & PREV_BLOCK_ALLOCATED;
    sf_block* next_block = (sf_block*)address_char;
    pp_block->header &= (~THIS_BLOCK_ALLOCATED);
    next_block->header &= (~PREV_BLOCK_ALLOCATED);
    size_t *footer = (void*) address_char-8; // access footer of the current block
    *footer = pp_block->header; //set the footer equal to the header;
    int index;
    int checktosee = pp_block_size;
        if (checktosee == 32)
            index = 0;
        if (checktosee > 32 && checktosee < 65)
            index = 1;
        if (checktosee >64 && checktosee <129)
            index =2;
        if (checktosee>128 && checktosee < 257)
            index =3;
        if (checktosee >256 && checktosee<513)
            index = 4;
        if(checktosee>512 && checktosee <1025)
            index = 5;
        if(checktosee> 1024)
            index = 6;
        sf_block* block_value = &sf_free_list_heads[index];
        insert_into_list(block_value,pp_block);
        sf_block* coalesced  = pp_block;
        if(previously != 2){
            char* previous_block_address =  (char*) pp_block -8; //get footer of previous block
            sf_block* previous_block =  (sf_block*)previous_block_address;
            char* new_address = previous_block_address - (previous_block->header & ((~0xf))) + 8;
            previous_block = (sf_block*) new_address;
            coalesced = coalesce(previous_block,pp_block);
        if((next_block->header & THIS_BLOCK_ALLOCATED) == 0){
            coalesced = coalesce(coalesced,next_block);
        }
    }
        else{
            if((next_block->header & THIS_BLOCK_ALLOCATED)== 0 ){
                coalesced = coalesce(pp_block,next_block);
            }
        }


}
int checkPower(size_t align){
    return (align != 0) && ((align & (align - 1))== 0);
}


void *sf_realloc(void *pp, size_t rsize) {




    if (checkcases_realloc(pp,rsize) == NULL) {
        sf_errno = EINVAL;
        return NULL;
    }


     char* address_char = (char*)pp;
    address_char = address_char - 8;
    sf_block* pp_block = (sf_block*) address_char;
    char* starting = (char*) sf_mem_start();
    starting = starting + 40;
    char* ending = (char*) sf_mem_end();
    ending = ending - 8;
    size_t pp_block_size = pp_block->header &(~0xf);
    address_char = address_char+ pp_block_size - 8;
    if (rsize == 0){
        sf_free(pp);
        return NULL;
    }
   char* previous_block_address =  (char*) pp;
   previous_block_address = previous_block_address - 8;
   pp_block = (sf_block*) previous_block_address;
   pp_block_size = pp_block->header  &(~0xf);
    if(pp_block_size > rsize){
        size_t header = rsize+ 8;
        if (header < 32){
            header = 32;
        }

        else{
            while(header < 32){
                header = header +1;
            }
        }

        if((pp_block_size - header) % 32 == 0 && (pp_block_size - header > 0)){
            address_char = (char*) pp_block;
            address_char = address_char -pp_block_size;
            sf_block* next = (sf_block*) address_char;
            address_char = (char*) pp_block;
            address_char = address_char + header;
            sf_block* remains = (sf_block*) address_char;
            sf_block* allocatedd = split_blocks(pp_block,header);
            if((next->header & THIS_BLOCK_ALLOCATED) == 0){
                coalesce(remains,next);
            }
            return allocatedd;
        }

    }
    if(pp_block_size < rsize){
        void *payload = sf_malloc(rsize);
        if(payload == NULL){

            return NULL;
        }
        size_t size_of_pay = pp_block_size - 8;
        void* copied = memcpy(payload,pp,size_of_pay);
        sf_free(pp);
        return copied;
    }



    return pp;
}

void *sf_memalign(size_t size, size_t align) {
    if (align < 32){
        sf_errno = EINVAL;
        return NULL;
    }
    if (checkPower(align) == 0){
        sf_errno = EINVAL;
        return NULL;
    }

    if (size == 0){
        sf_errno = EINVAL;
        return NULL;
    }

    size_t sizeneeded = size + align + 32;
    void* payload = sf_malloc(sizeneeded);
    if(payload == NULL){
        sf_errno = EINVAL;
        return NULL;
    }



    return NULL;
}
