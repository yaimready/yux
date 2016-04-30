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

#ifndef YUX_READ_FILTER_HPP_
#define YUX_READ_FILTER_HPP_

#include "yux/worker_controller.hpp"

namespace yux{

class read_sink{

    worker_controller *ctrl_ptr_;

public:

    read_sink(worker_controller *ctrl_ptr){
        ctrl_ptr_=ctrl_ptr;
    }

public:

    void filter_read(const char *data_ptr,int data_size){
        ctrl_ptr_->io_filterin(data_ptr,data_size);
    }

    worker_controller *get_controller(){
        return ctrl_ptr_;
    }

};


class simple_read_sink:public read_sink{

    char buffer_[1024];

public:

    simple_read_sink(worker_controller *ctrl_ptr):read_sink(ctrl_ptr){
        //
    }

public:

    char *get_read_buffer(int *read_limit_ptr){
        *read_limit_ptr=1024;
        return buffer_;
    }

};


}

#endif // YUX_READ_FILTER_HPP_
