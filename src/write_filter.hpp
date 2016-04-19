#ifndef YUX_WRITER_FILTER_HPP_
#define YUX_WRITER_FILTER_HPP_

#include "yux/worker_controller.hpp"

namespace yux{

class write_sink{

    worker_controller *ctrl_ptr_;

public:

    write_sink(worker_controller *ctrl_ptr){
        ctrl_ptr_=ctrl_ptr;
    }

public:

    void filter_write(const char *data_ptr,int data_size){
        ctrl_ptr_->io_filterout(data_ptr,data_size);
    }

    void filter_notify(){
        //
    }

    worker_controller *get_controller(){
        return ctrl_ptr_;
    }

};

class simple_write_sink:public write_sink{

    char buffer_[1024];

public:

    simple_write_sink(worker_controller *ctrl_ptr):write_sink(ctrl_ptr){
        //
    }

public:

    char *get_write_buffer(int *write_limit_ptr){
        *write_limit_ptr=1024;
        return buffer_;
    }

};

}

#endif // YUX_WRITER_FILTER_HPP_


