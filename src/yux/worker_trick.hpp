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

#ifndef YUX_WORKER_TRICK_HPP_
#define YUX_WORKER_TRICK_HPP_

#include "yux/worker_controller.hpp"

namespace yux{

template <int ReadWriteIsSame,int HandlerReadIsSame,
    typename ReadFilterClass,typename WriteFilterClass,typename HandlerClass>
class worker_trick{

    ReadFilterClass read_filter_;
    WriteFilterClass write_filter_;
    HandlerClass handler_;

public:

    worker_trick(yux::worker_controller *ctrl_ptr):read_filter_(ctrl_ptr),
            write_filter_(ctrl_ptr),handler_(ctrl_ptr){
        //
    }

protected:

    ReadFilterClass& get_read_filter(){
        return read_filter_;
    }

    WriteFilterClass& get_write_filter(){
        return write_filter_;
    }

    HandlerClass& get_handler(){
        return handler_;
    }

};

// 此时读和回调类在一处，可共用
template <typename ReadFilterHandlerClass,typename WriteFilterClass,typename ReadFilterHandlerClass1>
class worker_trick<0,1,ReadFilterHandlerClass,WriteFilterClass,ReadFilterHandlerClass1>{

    WriteFilterClass write_filter_;
    ReadFilterHandlerClass handler_;

public:

    worker_trick(yux::worker_controller *ctrl_ptr):write_filter_(ctrl_ptr),handler_(ctrl_ptr){
        //
    }

protected:

    ReadFilterHandlerClass& get_read_filter(){
        return handler_;
    }

    WriteFilterClass& get_write_filter(){
        return write_filter_;
    }

    ReadFilterHandlerClass& get_handler(){
        return handler_;
    }

};

// 此时读和写类在一处，可共用
template <typename ReadWriterFilterClass,typename ReadWriterFilterClass1,typename HandlerClass>
class worker_trick<1,0,ReadWriterFilterClass,ReadWriterFilterClass1,HandlerClass>{

    ReadWriterFilterClass filter_;
    HandlerClass handler_;

public:

    worker_trick(yux::worker_controller *ctrl_ptr):filter_(ctrl_ptr),handler_(ctrl_ptr){
        //
    }

protected:

    ReadWriterFilterClass& get_read_filter(){
        return filter_;
    }

    ReadWriterFilterClass& get_write_filter(){
        return filter_;
    }

    HandlerClass& get_handler(){
        return handler_;
    }

};

// 此时读、写和回调类在一处，三者可共用
template <typename ReadWriterFilterHandlerClass>
class worker_trick<1,1,
    ReadWriterFilterHandlerClass,ReadWriterFilterHandlerClass,ReadWriterFilterHandlerClass>{

    ReadWriterFilterHandlerClass filter_handler_;

public:

    worker_trick(yux::worker_controller *ctrl_ptr):filter_handler_(ctrl_ptr){
        //
    }

protected:

    ReadWriterFilterHandlerClass& get_read_filter(){
        return filter_handler_;
    }

    ReadWriterFilterHandlerClass& get_write_filter(){
        return filter_handler_;
    }

    ReadWriterFilterHandlerClass& get_handler(){
        return filter_handler_;
    }

};

}

#endif // YUX_WORKER_TRICK_HPP_
