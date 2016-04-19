/*************************************************
Copyright: GPL
Author: yinghua.yu
Date: 2016-03-29
Description: ʹ��Boost::ASIO��װ��TCP����˵����ӣ�ҵ����
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
    Description:    // ��ʼ��TCP��ص����ݺͳ�ʼ����д�ص�����
    Input:          // ����yux::asio::asio_tcp_data���͵����ݣ�
                    // ��yux::asio::asio_tcp_server��̬������
                    // ���ڽ�TCP����Ϣ���뵽��ǰ�ࡣ
    Output:         // ��
    Return:         // ��
    *************************************************/
    asio_tcp_worker(yux::asio::asio_tcp_data *data) {
        data_=data;
        read_function=boost::bind(&asio_tcp_worker::handle_receive,this,boost::asio::placeholders::bytes_transferred,boost::asio::placeholders::error);
        write_function=boost::bind(&asio_tcp_worker::handle_send,this,boost::asio::placeholders::error);
    }

    /*************************************************
    Function:       // ~asio_tcp_worker
    Description:    // ����TCP��صĺ���
    Input:          // ��
    Output:         // ��
    Return:         // ��
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
    Description:    // �����������ڲ�����Boost::ASIO���첽��
                    // ��������yux::worker_controller_impl�����ࣩ��trigger_read��process_read��
    Input:          // ���뻺����ָ�루buffer���ͻ��������ȣ�buffer_size��
                    // ������ָ������ReadFilterClass��ģ���������ĵ��û�ã�
                    // Ĭ��ReadFilterClassӦ��ʹ��yux::read_sink��
    Output:         // �첽��ȡ�Ľ����Ĭ��д�ص���������Ӧ�Ŀռ��С�
    Return:         // ��
    *************************************************/
    void io_read(char *buffer,int buffer_size){
        buffer_=buffer;
        data_->get_socket().async_read_some(boost::asio::buffer(buffer,buffer_size),read_function);
    }

    /*************************************************
    Function:       // io_write
    Description:    // ����д�����ڲ�����Boost::ASIO���첽д
                    // ��������yux::worker_controller_impl�����ࣩ��io_filterout��process_write��
    Input:          // ����д��������ָ�루data_ptr�������ݳ��ȣ�data_size��
                    // ������ָ������WriteFilterClass��ģ���������ĵ��û�ã�
                    // Ĭ��WriteFilterClassӦ��ʹ��yux::write_sink��
    Output:         // ��
    Return:         // ��
    *************************************************/
    void io_write(const char *data_ptr,int data_size){
        boost::asio::async_write(data_->get_socket(),boost::asio::buffer(data_ptr,data_size),write_function);
    }

    /*************************************************
    Function:       // io_close
    Description:    // �ر�Socket��Ӧ��TCP����
                    // ��������yux::worker_controller_impl�����ࣩ��close
    Input:          // ��
    Output:         // ��
    Return:         // ��
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
