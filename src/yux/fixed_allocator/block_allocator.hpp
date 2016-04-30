/* 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef YUX_FIXED_ALLOCATOR_BLOCK_ALLOCATOR_HPP_
#define YUX_FIXED_ALLOCATOR_BLOCK_ALLOCATOR_HPP_

#include "boost/pool/pool.hpp"
#include "yux/spinlock.hpp"

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
        return (block_type *)malloc(BlockSize);
#else
        yux::spinlock::scoped_lock lock(mutex_);
        return reinterpret_cast<BlockType *>(pool_.malloc());
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


}
}

#endif // YUX_FIXED_ALLOCATOR_BLOCK_ALLOCATOR_HPP_
