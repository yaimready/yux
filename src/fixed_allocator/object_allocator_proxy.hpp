#ifndef YUX_FIXED_ALLOCATOR_OBJECT_ALLOCATOR_PROXY_HPP_
#define YUX_FIXED_ALLOCATOR_OBJECT_ALLOCATOR_PROXY_HPP_

#include <new>

namespace yux{
namespace fixed_allocator{

template <typename ObjectType,typename AllocatorType>
class object_allocator_proxy{

    AllocatorType& allocator_;

public:

    object_allocator_proxy(AllocatorType &alloc):allocator_(alloc){
        //
    }

public:

    ObjectType *construct(){
        ObjectType *from_ptr=allocator_.allocate();
        if (from_ptr){
            new (from_ptr) ObjectType();
        }
        return from_ptr;
    }

    template <typename T>
    ObjectType *construct(T arg){
        ObjectType *from_ptr=allocator_.allocate();
        if (from_ptr){
            new (from_ptr) ObjectType(arg);
        }
        return from_ptr;
    }

    template <typename T1,typename T2>
    ObjectType *construct(T1 arg1,T2 arg2){
        ObjectType *from_ptr=allocator_.allocate();
        if (from_ptr){
            new (from_ptr) ObjectType(arg1,arg2);
        }
        return from_ptr;
    }

    template <typename T1,typename T2,typename T3>
    ObjectType *construct(T1 arg1,T2 arg2,T3 arg3){
        ObjectType *from_ptr=allocator_.allocate();
        if (from_ptr){
            new (from_ptr) ObjectType(arg1,arg2,arg3);
        }
        return from_ptr;
    }


    template <typename T1,typename T2,typename T3,typename T4>
    ObjectType *construct(T1 arg1,T2 arg2,T3 arg3,T4 arg4){
        ObjectType *from_ptr=allocator_.allocate();
        if (from_ptr){
            new (from_ptr) ObjectType(arg1,arg2,arg3,arg4);
        }
        return from_ptr;
    }

    template <typename T1,typename T2,typename T3,typename T4,typename T5>
    ObjectType *construct(T1 arg1,T2 arg2,T3 arg3,T4 arg4,T5 arg5){
        ObjectType *from_ptr=allocator_.allocate();
        if (from_ptr){
            new (from_ptr) ObjectType(arg1,arg2,arg3,arg4,arg5);
        }
        return from_ptr;
    }

    void destruct(ObjectType *obj_ptr){
        obj_ptr->~ObjectType();
        allocator_.deallocate(obj_ptr);
    }

};

template <typename ObjectType,typename AllocatorType>
struct object_allocator0:public object_allocator_proxy<ObjectType,AllocatorType>{

    AllocatorType allocator_;

public:

    object_allocator0():object_allocator_proxy(allocator_){
        //
    }

};

template <typename ObjectType,typename AllocatorType>
struct object_allocator1:public object_allocator_proxy<ObjectType,AllocatorType>{

    AllocatorType allocator_;

public:

    template <typename A1>
    object_allocator1(A1 arg1):allocator_(arg1),object_allocator_proxy(allocator_){
        //
    }

};


template <typename ObjectType,typename AllocatorType>
struct object_allocator2:public object_allocator_proxy<ObjectType,AllocatorType>{

    AllocatorType allocator_;

public:

    template <typename A1,typename A2>
    object_allocator2(A1 arg1,A2 arg2):allocator_(arg1,arg2),object_allocator_proxy(allocator_){
        //
    }

};


}
}

#endif // YUX_FIXED_ALLOCATOR_OBJECT_ALLOCATOR_PROXY_HPP_
