#ifndef YUX_FIXED_ALLOCATOR_HPP_
#define YUX_FIXED_ALLOCATOR_HPP_

#include "boost/pool/pool.hpp"

#include "yux/spinlock.hpp"
#include "yux/linked_ptr_queue.hpp"

//#define YUX_FIXED_ALLOCATOR_POOL_OFF

namespace yux{
namespace fixed_allocator{

template <typename BlockType,int BlockSize=sizeof(BlockType)>
class block_allocator{

    enum { block_size=BlockSize };

public:

    block_allocator():pool_(block_size){
        //
    }

public:

    BlockType *allocate(){
#ifdef YUX_FIXED_ALLOCATOR_POOL_OFF
        return (BlockType *)malloc(BlockSize);
#else
        yux::spinlock::scoped_lock lock(mutex_);
        BlockType* obj_ptr=(BlockType *)pool_.malloc();
        BOOST_ASSERT(obj_ptr!=NULL);
        return obj_ptr;
#endif // YUX_FIXED_ALLOCATOR_POOL_OFF
    }

    void deallocate(BlockType *obj_ptr){
#ifdef YUX_FIXED_ALLOCATOR_POOL_OFF
        free(obj_ptr);
#else
        yux::spinlock::scoped_lock lock(mutex_);
        pool_.free(obj_ptr);
#endif // YUX_FIXED_ALLOCATOR_POOL_OFF
    }

private:

    yux::spinlock mutex_;
    boost::pool<> pool_;

};

template <typename ObjectType>
class object_allocator{

public:

    object_allocator():pool_(sizeof(ObjectType)){
        //
    }

public:

    ObjectType *construct(){
#ifdef YUX_FIXED_ALLOCATOR_POOL_OFF
        return new ObjectType();
#else
        yux::spinlock::scoped_lock lock(mutex_);
        ObjectType* obj_ptr=(ObjectType *)pool_.malloc();
        BOOST_ASSERT(obj_ptr!=NULL);
        new (obj_ptr) ObjectType();
        return obj_ptr;
#endif // YUX_FIXED_ALLOCATOR_POOL_OFF
    }

    template <typename T>
    ObjectType *construct(T arg){
#ifdef YUX_FIXED_ALLOCATOR_POOL_OFF
        return new ObjectType(arg);
#else
        yux::spinlock::scoped_lock lock(mutex_);
        ObjectType* obj_ptr=(ObjectType *)pool_.malloc();
        BOOST_ASSERT(obj_ptr!=NULL);
        new (obj_ptr) ObjectType(arg);
        return obj_ptr;
#endif // YUX_FIXED_ALLOCATOR_POOL_OFF
    }

    template <typename T1,typename T2>
    ObjectType *construct(T1 arg1,T2 arg2){
#ifdef YUX_FIXED_ALLOCATOR_POOL_OFF
        return new ObjectType(arg1,arg2);
#else
        yux::spinlock::scoped_lock lock(mutex_);
        ObjectType* obj_ptr=(ObjectType *)pool_.malloc();
        BOOST_ASSERT(obj_ptr!=NULL);
        new (obj_ptr) ObjectType(arg1,arg2);
        return obj_ptr;
#endif // YUX_FIXED_ALLOCATOR_POOL_OFF
    }

    template <typename T1,typename T2,typename T3>
    ObjectType *construct(T1 arg1,T2 arg2,T3 arg3){
#ifdef YUX_FIXED_ALLOCATOR_POOL_OFF
        return new ObjectType(arg1,arg2,arg3);
#else
        yux::spinlock::scoped_lock lock(mutex_);
        ObjectType* obj_ptr=(ObjectType *)pool_.malloc();
        BOOST_ASSERT(obj_ptr!=NULL);
        new (obj_ptr) ObjectType(arg1,arg2,arg3);
        return obj_ptr;
#endif // YUX_FIXED_ALLOCATOR_POOL_OFF
    }


    template <typename T1,typename T2,typename T3,typename T4>
    ObjectType *construct(T1 arg1,T2 arg2,T3 arg3,T4 arg4){
#ifdef YUX_FIXED_ALLOCATOR_POOL_OFF
        return new ObjectType(arg1,arg2,arg3,arg4);
#else
        yux::spinlock::scoped_lock lock(mutex_);
        ObjectType* obj_ptr=(ObjectType *)pool_.malloc();
        BOOST_ASSERT(obj_ptr!=NULL);
        new (obj_ptr) ObjectType(arg1,arg2,arg3,arg4);
        return obj_ptr;
#endif // YUX_FIXED_ALLOCATOR_POOL_OFF
    }

    template <typename T1,typename T2,typename T3,typename T4,typename T5>
    ObjectType *construct(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5){
#ifdef YUX_FIXED_ALLOCATOR_POOL_OFF
        return new ObjectType(arg1,arg2,arg3,arg4);
#else
        yux::spinlock::scoped_lock lock(mutex_);
        ObjectType* obj_ptr=(ObjectType *)pool_.malloc();
        BOOST_ASSERT(obj_ptr!=NULL);
        new (obj_ptr) ObjectType(arg1,arg2,arg3,arg4,arg5);
        return obj_ptr;
#endif // YUX_FIXED_ALLOCATOR_POOL_OFF
    }

    void destruct(ObjectType *obj_ptr){
#ifdef YUX_FIXED_ALLOCATOR_POOL_OFF
        delete obj_ptr;
#else
        yux::spinlock::scoped_lock lock(mutex_);
        obj_ptr->~ObjectType();
        pool_.free(obj_ptr);
#endif // YUX_FIXED_ALLOCATOR_POOL_OFF
    }

private:

    yux::spinlock mutex_;
    boost::pool<> pool_;

};


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

public:

    bigblock_allocator(int step_size=32){
        step_size_=step_size;
        create_cache();
    }

private:

    void create_cache(){
        // initialize a large memory
        size_t cache_size=step_size_*sizeof(cache_node);
        cache_node *header_ptr=(cache_node *)::malloc(cache_size);
        assert(header_ptr!=NULL);
        if (!header_ptr){
            return;
        }
        // link each node to next node
        cache_node *next_ptr=header_ptr;
        for (int i=0;i<step_size_;i++,next_ptr++){
#ifdef _DEBUG
            next_ptr->mark=int(next_ptr)^20150509;
#endif
            next_ptr->next=next_ptr+1;
        }
        // let last node link to NULL
        next_ptr--;
        next_ptr->next=NULL;
        yux::linked_ptr_queue<cache_node> new_queue(header_ptr,next_ptr);
        queue_.add_tail(&new_queue);
        yux::linked_ptr_wrapper<cache_node *> *item_ptr=allocator_.allocate();
        item_ptr->data=header_ptr;
        cache_list_.add_tail(item_ptr);
    }

public:

    BlockType *allocate(){
        yux::spinlock::scoped_lock lock(mutex_);
        cache_node *node_ptr=queue_.remove_head();
        if (!node_ptr){
            create_cache();
            node_ptr=queue_.remove_head();
            BOOST_ASSERT(node_ptr!=NULL);
        }
        return (BlockType *)(node_ptr->data);
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
    yux::fixed_allocator::block_allocator<yux::linked_ptr_wrapper<cache_node *> > allocator_;
    yux::linked_ptr_queue<yux::linked_ptr_wrapper<cache_node *> >cache_list_;
    yux::spinlock mutex_;
    int step_size_;

};

}
}

#endif // YUX_FIXED_ALLOCATOR_HPP_
