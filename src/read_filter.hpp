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
