#ifndef YUX_ASIO_TCP_SERVER_HPP_
#define YUX_ASIO_TCP_SERVER_HPP_

#include "boost/thread.hpp"

#include "yux/fixed_allocator.hpp"
#include "yux/asio/asio_tcp_data.hpp"
#include "yux/asio/asio_multithread_service.hpp"

namespace yux{
namespace asio{

template <typename WorkerClass>
class asio_tcp_server:boost::noncopyable{

    enum { stopped_status=0,starting_status=1,running_status=2,fail_status=3 };

    typedef yux::fixed_allocator::object_allocator<yux::asio::asio_tcp_data> tcp_data_allocator;
    typedef yux::fixed_allocator::object_allocator<WorkerClass> tcp_worker_allocator;

public:

    asio_tcp_server(int port,int worker_service_num=8){
        port_=port;
        worker_service_num_=worker_service_num;
        hot_worker_index_=0;
        tcp_worker_destroyers_ptr_=new boost::function1<void,void *>[worker_service_num];
        tcp_worker_allocators_ptr_=new tcp_worker_allocator[worker_service_num];
        for (int i=0;i<worker_service_num_;i++){
            *(tcp_worker_destroyers_ptr_+i)=boost::bind(&asio_tcp_server<WorkerClass>::destroy_worker,this,i,_1);
        }
        status_=stopped_status;
    }

    ~asio_tcp_server(){
        delete []tcp_worker_destroyers_ptr_;
        delete []tcp_worker_allocators_ptr_;
    }

private:

    void new_accept(){
        tcp_data_allocator& allocator_=tcp_data_allocators_ptr_[hot_worker_index_];
        worker_allocator_ptr_=tcp_worker_allocators_ptr_+hot_worker_index_;
        hot_tcp_data_ptr_=allocator_.construct(boost::ref(multithread_service_ptr_->get_service()),&allocator_,&(tcp_worker_destroyers_ptr_[hot_worker_index_]));
        accepter_ptr_->async_accept(hot_tcp_data_ptr_->get_socket(),boost::bind(&asio_tcp_server::accept_handler,this,boost::asio::placeholders::error));
        if (hot_worker_index_==0){
            hot_worker_index_=worker_service_num_;
        }
        hot_worker_index_--;
    }

    void accept_handler(const boost::system::error_code &err_code){
        if (err_code){
            printf("asio tcp server error occured in accept! --> %s\n",err_code.message().c_str());
            hot_tcp_data_ptr_->get_allocator().destruct(hot_tcp_data_ptr_);
        }else{
            worker_allocator_ptr_->construct(hot_tcp_data_ptr_)->io_init(data_);
        }
        if (!stop_signal_){
            new_accept();
        }
    }

    void destroy_worker(int index,void *worker_ptr){
        tcp_worker_allocators_ptr_[index].destruct(reinterpret_cast<WorkerClass *>(worker_ptr));
    }

public:

    bool get_healty(){
        return status_;
    }

    void start(void *data){
        if (status_==fail_status){
            printf("please clear error before start!\n");
        }else if(status_!=stopped_status){
            printf("worker services is running ,please stop before!\n");
            return;
        }
        status_=starting_status;
        try{
            accepter_ptr_=new boost::asio::ip::tcp::acceptor(accepter_service_,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port_));
        }catch(std::exception &e){
            printf("accepter init failed , maybe bind port %d fobbiden! --> %s\n",port_,e.what());
            status_=fail_status;
            return;
        }
        if (accepter_ptr_==NULL){
            printf("port %d maybe unusable, please check network again!\n",port_);
            status_=fail_status;
            return;
        }
        tcp_data_allocators_ptr_=new tcp_data_allocator[worker_service_num_];
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
            delete []tcp_data_allocators_ptr_;
            status_=stopped_status;
        }
    }

    void stop(){
        stop_signal_=true;
        accepter_service_.stop();
    }

private:

    boost::asio::io_service accepter_service_;
    boost::asio::ip::tcp::acceptor *accepter_ptr_;
    yux::asio::asio_multithread_service *multithread_service_ptr_;
    yux::asio::asio_tcp_data* hot_tcp_data_ptr_;
    boost::thread *accepter_service_thread_ptr_;
    tcp_worker_allocator* worker_allocator_ptr_;

    tcp_data_allocator *tcp_data_allocators_ptr_;
    boost::function1<void,void *> *tcp_worker_destroyers_ptr_;
    tcp_worker_allocator *tcp_worker_allocators_ptr_;


    int port_;
    int worker_service_num_;
    int hot_worker_index_;
    int status_;
    volatile bool stop_signal_;
    void *data_;

};

}
}

#endif // YUX_ASIO_TCP_SERVER_HPP_
