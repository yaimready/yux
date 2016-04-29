#ifndef YUX_ASIO_TCP_SERVER_HPP_
#define YUX_ASIO_TCP_SERVER_HPP_

#include <new>
#include "boost/thread.hpp"

#include "yux/fixed_allocator/bigblock_allocator.hpp"
#include "yux/fixed_allocator/object_allocator_proxy.hpp"

#include "yux/asio/asio_tcp_data.hpp"
#include "yux/asio/asio_multithread_service.hpp"

namespace yux{
namespace asio{

template <typename WorkerClass>
class asio_tcp_server:boost::noncopyable{

    enum { starting_status=0,running_status=1,not_ready_status=2,ready_status=3 };

    // 必须引用自yux::asio::asio_tcp_data_allocator，
    // 因为我是通过万能指针(void *)来传递allocator的。
    typedef yux::asio::asio_tcp_data_allocator tcp_data_allocator_type;

    typedef yux::fixed_allocator::object_allocator1<
        WorkerClass,
        yux::fixed_allocator::bigblock_allocator<WorkerClass> > tcp_worker_allocator_type;

public:

    asio_tcp_server(int worker_service_num=8):
            tcp_worker_allocator_(1024),tcp_data_allocator_(1024){
        worker_service_num_=worker_service_num;
        next_worker_index_=0;
        accepter_ptr_=NULL;
        tcp_worker_destroyer_=boost::bind(&asio_tcp_server<WorkerClass>::destroy_worker,this,_1);
        status_=not_ready_status;
    }

public:

    bool set_bind_address(const char *host,int port){
        host_=host;
        port_=port;
        try{
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::from_string(host),port_);
            accepter_ptr_=new boost::asio::ip::tcp::acceptor(accepter_service_,endpoint);
            status_=ready_status;
        }catch(std::exception &e){
            printf("asio server can not bind port at %d --> %s\n",port_,e.what());
            accepter_ptr_=NULL;
        }
        return accepter_ptr_!=NULL;
    }

    int get_status(){
        return status_;
    }

    void start(void *data){
        BOOST_ASSERT(status_!=not_ready_status);
        if(status_!=ready_status){
            printf("worker services is running ,please stop before!\n");
            return;
        }
        status_=starting_status;
        multithread_service_ptr_=new yux::asio::asio_multithread_service(worker_service_num_);
        new_accept();
        stop_signal_=false;
        data_=data;
        printf("now, service at port %d is running ( %d threads ready ).\n",port_,worker_service_num_);
        accepter_service_thread_ptr_=new boost::thread(boost::bind(&boost::asio::io_service::run,&accepter_service_));
        status_=running_status;
    }

    void join(){
        if ((status_==running_status)&&accepter_service_thread_ptr_){
            accepter_service_thread_ptr_->join();
            printf("asio server accpeter die ...\n");
            delete accepter_service_thread_ptr_;
            accepter_service_thread_ptr_=NULL;
            delete multithread_service_ptr_;
            status_=ready_status;
        }
    }

    void stop(){
        stop_signal_=true;
        accepter_service_.stop();
    }


private:

    void new_accept(){
        current_tcp_data_ptr_=tcp_data_allocator_.construct(boost::ref(multithread_service_ptr_->get_service()),&tcp_data_allocator_);
        if (current_tcp_data_ptr_){
            accepter_ptr_->async_accept(current_tcp_data_ptr_->get_socket(),
                                        boost::bind(&asio_tcp_server::accept_handler,this,boost::asio::placeholders::error));
            if (next_worker_index_==0){
                next_worker_index_=worker_service_num_;
            }
            next_worker_index_--;
        }else{
            // 看看等待一段时间之后，是否还能分配到数据结构。
            boost::asio::deadline_timer timer(accepter_service_, boost::posix_time::seconds(5));
            timer.async_wait(boost::bind(&asio_tcp_server::after_accept_wait,this,boost::asio::placeholders::error));
        }
    }

    void after_accept_wait(const boost::system::error_code& err_code){
        if (!err_code){
            new_accept();
        }
    }

    void accept_handler(const boost::system::error_code &err_code){
        if (err_code){
            printf("asio tcp server error occured in accept! --> %s\n",err_code.message().c_str());
            current_tcp_data_ptr_->destroy_self();
        }else{
            int index=next_worker_index_;
            WorkerClass *worker_ptr=tcp_worker_allocator_.construct(current_tcp_data_ptr_);
            if (worker_ptr){
                current_tcp_data_ptr_->set_worker_destroyer(&tcp_worker_destroyer_);
                worker_ptr->io_init(data_);
            }else{
                current_tcp_data_ptr_->destroy_self();
            }
        }
        if (!stop_signal_){
            new_accept();
        }
    }

    void destroy_worker(void *worker_ptr){
        tcp_worker_allocator_.destruct(reinterpret_cast<WorkerClass *>(worker_ptr));
    }

private:

    boost::asio::io_service accepter_service_;
    boost::asio::ip::tcp::acceptor *accepter_ptr_;
    yux::asio::asio_multithread_service *multithread_service_ptr_;
    boost::thread *accepter_service_thread_ptr_;

    tcp_data_allocator_type tcp_data_allocator_;
    tcp_worker_allocator_type tcp_worker_allocator_;
    boost::function1<void,void *> tcp_worker_destroyer_;
    yux::asio::asio_tcp_data* current_tcp_data_ptr_;
    int next_worker_index_;

    std::string host_;
    int port_;
    int worker_service_num_;
    int status_;
    volatile bool stop_signal_;
    void *data_;

};

}
}

#endif // YUX_ASIO_TCP_SERVER_HPP_
