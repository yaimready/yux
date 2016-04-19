#ifndef YUX_WORKER_HANDLER_HPP_
#define YUX_WORKER_HANDLER_HPP_

#include "stdio.h"
#include <string>
#include <boost/system/error_code.hpp>

#include "yux/worker_controller.hpp"

namespace yux{

class worker_handler{

    worker_controller *ctrl_ptr_;

public:

    worker_handler(worker_controller *ctrl_ptr){
        ctrl_ptr_=ctrl_ptr;
        printf("worker construct!\n");
    }

    ~worker_handler(){
        printf("worker destruct!\n");
    }

public:

    void handle_open(void *){
        printf("worker open!\n");
        //ctrl_ptr_->trigger_read(true);
        std::string str("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
        ctrl_ptr_->write(str.c_str(),str.size());
        ctrl_ptr_->trigger_read(true);
    }

    void handle_read(const char *data_ptr,int data_size){
        std::string str(data_ptr,data_size);
        printf("worker read %d bytes! --> %s\n",data_size,str.c_str());
        //std::string str("HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello,World");
        //ctrl_ptr_->write(str.c_str(),str.size());
    }

    void handle_error(int type,int err_code){
        boost::system::error_code err(err_code,boost::system::system_category());
        printf("worker error %d(%s) with type %d!\n",err_code,err.message().c_str(),type);
        ctrl_ptr_->close();
    }

    void handle_close(){
        printf("worker close!\n");
    }

};

}

#endif // YUX_WORKER_HANDLER_HPP_
