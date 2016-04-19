#ifndef YUX_ASIO_TCP_CLIENT_HPP_
#define YUX_ASIO_TCP_CLIENT_HPP_

#include <boost/type_traits.hpp>

namespace yux{
namespace asio{

template <typename WorkerClass>
class asio_tcp_client{

    BOOST_STATIC_ASSERT_MSG((boost::is_base_of<yux::asio::asio_tcp_client_worker_probe,WorkerClass>::value==1),
                            "asio_tcp_client 's template argument <WorkerClass> must inherit from asio_tcp_client_worker!");

public:

    asio_tcp_client(){
        worker_ptr_=new WorkerClass(create_tcp_data());
        worker_ptr_->ref();
    }

    ~asio_tcp_client(){
        worker_ptr_->io_trycancel();
        worker_ptr_->unref();
    }

public:

    int get_healthy(){
        return worker_ptr_->get_healthy();
    }

    yux::asio::asio_tcp_data* create_tcp_data(){
        static yux::fixed_allocator::object_allocator<yux::asio::asio_tcp_data> data_allocator;
        static yux::asio::asio_multithread_service multithread_service_(20);
        return data_allocator.construct(boost::ref(multithread_service_.get_service()),&data_allocator);
    }

public:

    bool set_server_address(const std::string& host){
        return worker_ptr_->set_server_address(host);
    }

    worker_controller *get_controller(){
        return worker_ptr_;
    }

    void start(void *data){
        worker_ptr_->io_init(data);
    }

private:

    WorkerClass *worker_ptr_;

};

}
}

#endif // YUX_ASIO_TCP_CLIENT_HPP_
