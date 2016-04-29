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

#ifndef YUX_ASIO_TCP_CLIENT_WORKER_HPP_
#define YUX_ASIO_TCP_CLIENT_WORKER_HPP_

#include "yux/worker_controller_impl.hpp"

namespace yux{
namespace asio{

struct client_param{

    enum healthy_type{ healthy_type_init=8, healthy_type_ok=0 ,
            healthy_type_closed=1 , healthy_type_connerr=2 ,
            healthy_type_conning=3 , healthy_type_canceled=4 };

};

struct asio_tcp_client_worker_probe{};

template <typename HandlerClass,typename ReadFilterClass,typename WriteFilterClass>
class asio_tcp_client_worker:public yux::worker_controller_impl<HandlerClass,ReadFilterClass,WriteFilterClass>
        ,public asio_tcp_client_worker_probe{

public:

    asio_tcp_client_worker(yux::asio::asio_tcp_data *data) {
        data_=data;
        status_=client_param::healthy_type_init;
        block_write();
        read_function=boost::bind(&asio_tcp_client_worker::handle_receive,this,boost::asio::placeholders::bytes_transferred,boost::asio::placeholders::error);
        write_function=boost::bind(&asio_tcp_client_worker::handle_send,this,boost::asio::placeholders::error);
    }

    ~asio_tcp_client_worker(){
        data_->get_allocator().destruct(data_);
    }

public:

    bool set_server_address(const std::string& host){
        int pos=host.find(':');
        if (pos==-1){
            printf("tcp client touch invalid host format! --> %s\n",host.c_str());
            return false;
        }
        boost::asio::ip::tcp::endpoint endpoint;
        try{
            std::string ip=host.substr(0,pos);
            unsigned short port=(unsigned short)atol(host.substr(pos+1).c_str());
            endpoint_=boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(ip),port);
            return true;
        }catch(std::exception &){
            printf("tcp client touch invalid endpoint %s\n",host.c_str());
            return false;
        }
    }

    int get_healthy(){
        return status_;
    }

public:

    void io_init(void *userdata){
        BOOST_ASSERT(endpoint_!=boost::asio::ip::tcp::endpoint());
        {
            yux::spinlock::scoped_lock lock(mutex);
            if (status_!=client_param::healthy_type_init){
                printf("client worker can't initialize once more!\n");
                return;
            }
            status_=client_param::healthy_type_conning;
        }
        userdata_=userdata;
        data_->get_socket().async_connect(endpoint_,boost::bind(&asio_tcp_client_worker::handle_connect,this,boost::asio::placeholders::error));
    }

    void io_trycancel(){
        yux::spinlock::scoped_lock lock(mutex);
        if (status_==client_param::healthy_type_init){
            status_=client_param::healthy_type_canceled;
            unref(false);
        }
    }

private:

    void io_read(char *buffer,int buffer_size){
        buffer_=buffer;
        data_->get_socket().async_read_some(boost::asio::buffer(buffer,buffer_size),read_function);
    }

    void io_write(const char *data_ptr,int data_size){
        boost::asio::async_write(data_->get_socket(),boost::asio::buffer(data_ptr,data_size),write_function);
    }

    void io_close(){
        boost::system::error_code err_code;
        data_->get_socket().shutdown(boost::asio::socket_base::shutdown_both,err_code);
        data_->get_socket().close(err_code);
    }

    void io_destroy(){
        delete this;
    }

private:

    void handle_connect(boost::system::error_code err_code) {
        printf("cworker connect --> %d\n",err_code.value());
        if (err_code) {
            status_=client_param::healthy_type_connerr;
            process_error(worker_param::error_type_connect,err_code.value());
            unref(); // 释放仅有的一个引用结束对象
        } else {
            status_=client_param::healthy_type_ok;
            yux::worker_controller_impl<HandlerClass,ReadFilterClass,WriteFilterClass>::io_init(userdata_);
            unblock_write();
        }
    }

    void handle_receive(int read_size,boost::system::error_code err_code){
        if (err_code) {
            process_error(yux::worker_param::error_type_read,err_code.value());
        }else{
            process_read(buffer_,read_size);
        }
    }

    void handle_send(boost::system::error_code err_code){
        //printf("cworker write back!\n");
        if (err_code) {
            process_error(yux::worker_param::error_type_write,err_code.value());
        }else{
            process_write();
        }
    }

private:

    boost::function2<void,boost::system::error_code,int> read_function;
    boost::function2<void,boost::system::error_code,int> write_function;
    boost::asio::ip::tcp::socket* socket_ptr_;
    boost::asio::ip::tcp::endpoint endpoint_;
    char *buffer_;
    int status_;
    void *userdata_;
    yux::asio::asio_tcp_data *data_;
    yux::spinlock mutex;

};

}
}

#endif // YUX_ASIO_TCP_CLIENT_WORKER_HPP_
