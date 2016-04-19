/*************************************************
Copyright: GPL
Author: yinghua.yu
Date: 2016-03-29
Description: 使用Boost::ASIO包装了TCP服务端的连接（业务）类
**************************************************/

#ifndef YUX_ASIO_TCP_WORKER_HPP_
#define YUX_ASIO_TCP_WORKER_HPP_

#include "yux/worker_controller_impl.hpp"

namespace yux{
namespace asio{

template <typename HandlerClass,typename ReadFilterClass,typename WriteFilterClass>
class asio_tcp_worker:public yux::worker_controller_impl<HandlerClass,ReadFilterClass,WriteFilterClass>{

public:


    /*************************************************
    Function:       // asio_tcp_worker
    Description:    // 初始化TCP相关的数据和初始化读写回调函数
    Input:          // 传入yux::asio::asio_tcp_data类型的数据，
                    // 由yux::asio::asio_tcp_server动态产生，
                    // 用于将TCP的信息传入到当前类。
    Output:         // 无
    Return:         // 无
    *************************************************/
    asio_tcp_worker(yux::asio::asio_tcp_data *data) {
        data_=data;
        read_function=boost::bind(&asio_tcp_worker::handle_receive,this,boost::asio::placeholders::bytes_transferred,boost::asio::placeholders::error);
        write_function=boost::bind(&asio_tcp_worker::handle_send,this,boost::asio::placeholders::error);
    }

    /*************************************************
    Function:       // ~asio_tcp_worker
    Description:    // 销毁TCP相关的函数
    Input:          // 无
    Output:         // 无
    Return:         // 无
    *************************************************/
    ~asio_tcp_worker(){
        data_->get_allocator().destruct(data_);
    }

public:

    bool get_remote_address(std::string& host){
        boost::system::error_code err_code;
        boost::asio::ip::tcp::endpoint endpoint=data_->get_socket().remote_endpoint(err_code);
        if (err_code){
            return false;
        }
        host=endpoint.address().to_string(err_code);
        return !err_code;
    }

private:

    /*************************************************
    Function:       // io_read
    Description:    // 产生读请求，内部调用Boost::ASIO的异步读
                    // 调用来自yux::worker_controller_impl（基类）的trigger_read和process_read。
    Input:          // 传入缓冲区指针（buffer）和缓冲区长度（buffer_size）
                    // 缓冲区指针来自ReadFilterClass（模板参数）类的调用获得，
                    // 默认ReadFilterClass应该使用yux::read_sink。
    Output:         // 异步读取的结果，默认写回到缓冲区对应的空间中。
    Return:         // 无
    *************************************************/
    void io_read(char *buffer,int buffer_size){
        buffer_=buffer;
        data_->get_socket().async_read_some(boost::asio::buffer(buffer,buffer_size),read_function);
    }

    /*************************************************
    Function:       // io_write
    Description:    // 产生写请求，内部调用Boost::ASIO的异步写
                    // 调用来自yux::worker_controller_impl（基类）的io_filterout和process_write。
    Input:          // 传入写入数据区指针（data_ptr）和数据长度（data_size）
                    // 缓冲区指针来自WriteFilterClass（模板参数）类的调用获得，
                    // 默认WriteFilterClass应该使用yux::write_sink。
    Output:         // 无
    Return:         // 无
    *************************************************/
    void io_write(const char *data_ptr,int data_size){
        boost::asio::async_write(data_->get_socket(),boost::asio::buffer(data_ptr,data_size),write_function);
    }

    /*************************************************
    Function:       // io_close
    Description:    // 关闭Socket对应的TCP链接
                    // 调用来自yux::worker_controller_impl（基类）的close
    Input:          // 无
    Output:         // 无
    Return:         // 无
    *************************************************/
    void io_close(){
        boost::system::error_code err_code;
        data_->get_socket().shutdown(boost::asio::socket_base::shutdown_both,err_code);
        data_->get_socket().close(err_code);
    }

    void io_destroy(){
        data_->destroy_worker(this);
    }

private:

    void handle_receive(int read_size,boost::system::error_code err_code){
        if (err_code) {
            process_error(yux::worker_param::error_type_read,err_code.value());
        }else{
            process_read(buffer_,read_size);
        }
    }

    void handle_send(boost::system::error_code err_code){
        if (err_code) {
            process_error(yux::worker_param::error_type_write,err_code.value());
        }else{
            process_write();
        }
    }

private:

    boost::function2<void,boost::system::error_code,int> read_function;
    boost::function2<void,boost::system::error_code,int> write_function;
    yux::asio::asio_tcp_data *data_;
    char *buffer_;

};

}
}

#endif // YUX_ASIO_TCP_WORKER_HPP_
