#ifndef YUX_FIXED_ALLOCATOR_BIGBLOCK_ALLOCATOR_HPP_
#define YUX_FIXED_ALLOCATOR_BIGBLOCK_ALLOCATOR_HPP_

#include "yux/spinlock.hpp"
#include "yux/linked_ptr_queue.hpp"

namespace yux{
namespace fixed_allocator{

template <typename BlockType,int BlockSize=sizeof(BlockType)>
class bigblock_allocator{

    struct cache_node;

    enum { block_size=BlockSize ,
#ifdef _DEBUG
    extra_size=sizeof(cache_node *)+sizeof(int)
#else
    extra_size=sizeof(cache_node *)
#endif
    };

    struct cache_node{
        cache_node *next;
#ifdef _DEBUG
        int mark;
#endif
        char data[block_size];
    };

    struct header_cache_node{
        header_cache_node *next;
        cache_node node;
    };

public:

    bigblock_allocator(int single_cache_size,int max_caches=-1){
        single_cache_size_=single_cache_size;
        max_caches_=max_caches;
        create_cache();
    }

    ~bigblock_allocator(){
        header_cache_node *node_ptr=header_queue_.remove_head();
        while (node_ptr){
            header_cache_node *next_ptr=node_ptr->next;
            free(node_ptr);
            node_ptr=next_ptr;
        }
    }

private:

    void create_cache(){
        // initialize a large memory
        size_t cache_size=(single_cache_size_-1)*sizeof(cache_node)+sizeof(header_cache_node);
        char *cache_ptr=(char *)::malloc(cache_size);
        if (cache_ptr!=NULL){
            // link each node to next node
            header_cache_node *header_ptr=(header_cache_node *)cache_ptr;
            cache_node *first_ptr=(cache_node *)(cache_ptr+(sizeof(header_cache_node)-sizeof(cache_node)));
            cache_node *next_ptr=first_ptr;
            for (int i=0;i<single_cache_size_;i++){
#ifdef _DEBUG
                next_ptr->mark=int(next_ptr)^20150509;
#endif
                next_ptr->next=next_ptr+1;
                next_ptr++;
            }
            // let last node link to NULL
            next_ptr--;
            next_ptr->next=NULL;
            yux::linked_ptr_queue<cache_node> new_queue(first_ptr,next_ptr);
            queue_.add_tail(&new_queue);
            header_queue_.add_tail(header_ptr);
        }
    }

public:

    BlockType *allocate(){
        yux::spinlock::scoped_lock lock(mutex_);
        cache_node *node_ptr=queue_.remove_head();
        if (node_ptr){
            return (BlockType *)(node_ptr->data);
        }
        if (max_caches_!=0){
            create_cache();
            if (max_caches_>0){
                max_caches_--;
            }
            node_ptr=queue_.remove_head();
        }
        if (node_ptr){
            return (BlockType *)(node_ptr->data);
        }else{
            return NULL;
        }
    }

    void deallocate(BlockType *ptr){
        yux::spinlock::scoped_lock lock(mutex_);
        cache_node *node_ptr=(cache_node *)((char *)ptr-extra_size);
#ifdef _DEBUG
        assert((node_ptr->mark^20150509)==int(node_ptr));
#endif
        queue_.add_tail(node_ptr);
    }

private:

    yux::linked_ptr_queue<cache_node> queue_;
    yux::linked_ptr_queue<header_cache_node> header_queue_;
    yux::spinlock mutex_;
    int single_cache_size_;
    int max_caches_;

};

}
}

#endif // YUX_FIXED_ALLOCATOR_BIGBLOCK_ALLOCATOR_HPP_
