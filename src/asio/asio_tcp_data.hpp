#ifndef YUX_ASIO_TCP_DATA_HPP_
#define YUX_ASIO_TCP_DATA_HPP_

#include "boost/asio.hpp"

#include "yux/fixed_allocator.hpp"

namespace yux{
namespace asio{

class asio_tcp_data{

    boost::asio::ip::tcp::socket socket_;
    yux::fixed_allocator::object_allocator<asio_tcp_data> *alloactor_ptr_;
    boost::function1<void,void *> *destroyer_ptr_;

public:

    asio_tcp_data(boost::asio::io_service &service,
                  yux::fixed_allocator::object_allocator<asio_tcp_data> *alloactor_ptr,
                  boost::function1<void,void *> *destroyer_ptr):socket_(service){
        alloactor_ptr_=alloactor_ptr;
		destroyer_ptr_=destroyer_ptr;
    }

public:

    boost::asio::ip::tcp::socket& get_socket(){
        return socket_;
    }

    yux::fixed_allocator::object_allocator<asio_tcp_data>& get_allocator(){
        return *alloactor_ptr_;
    }

    void destroy_worker(void *worker_ptr){
        (*destroyer_ptr_)(worker_ptr);
    }

};

}
}

#endif // YUX_ASIO_TCP_DATA_HPP_
