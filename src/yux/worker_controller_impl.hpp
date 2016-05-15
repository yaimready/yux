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

#ifndef YUX_WORKER_CONTROLLER_IMPL_HPP_
#define YUX_WORKER_CONTROLLER_IMPL_HPP_

#include <boost/type_traits.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

#include "yux/asio/asio_tcp_data.hpp"
#include "yux/pool_string.hpp"
#include "yux/worker_trick.hpp"
#include "yux/spinlock.hpp"
#include "yux/worker_controller.hpp"

#pragma warning(disable:4355)

namespace yux{

// 该程序下的所有函数，都需要保持有引用的情况下使用!
template <typename HandlerClass,typename ReadFilterClass,typename WriteFilterClass>
class worker_controller_impl:public worker_controller,
    public yux::worker_trick<boost::is_same<ReadFilterClass,WriteFilterClass>::value,boost::is_same<HandlerClass,ReadFilterClass>::value,
            ReadFilterClass,WriteFilterClass,HandlerClass>{

    // 0=关闭状态 1=打开状态 2=错误或者永久关闭状态
    // |--R(2bits)--|--WR(1bit)--|--W(2bits)--|--C(2bits)--|--FW(1bit)--|--BW(1bit)--|

    enum {
        read_bit=0x3 , willread_bit=0x4 , write_bit=0x18 ,
        close_bit=0x60 , finalwrite_bit=0x80 , blockwrite_bit=0x0100
        };
    enum { read_lbit=0x1 , write_lbit=0x8 , close_lbit=0x20 };

public:

    worker_controller_impl():yux::worker_trick<boost::is_same<ReadFilterClass,WriteFilterClass>::value,boost::is_same<HandlerClass,ReadFilterClass>::value,
            ReadFilterClass,WriteFilterClass,HandlerClass>(this){
        refcnt_=1;
        status_=0;
    }

public:

    void ref(){
        yux::spinlock::scoped_lock lock(mutex);
        refcnt_++;
    }

    void unref(bool can_be_last_unref=true){
        yux::spinlock::scoped_lock lock(mutex);
        if ((!can_be_last_unref)&&(refcnt_<=1)){
            printf("unsafe unref found -- %08X -- %d\n",this,refcnt_);
        }else{
            refcnt_--;
            __prepare_destroy(lock);
        }
    }

public:

    virtual void io_init(void *data){
        get_handler().handle_open(data);
        unref();
    }

private:

    // 由ReadFilter(读取过滤器)流出的信息，最后流入Handler(处理器)
    void io_filterin(const char *data_ptr,int data_size){
        get_handler().handle_read(data_ptr,data_size);
    }

    // 由WriteFitler(写入过滤器)流出的消息，最后进入系统IO环节
    // 此时写入一定是关闭的
    void io_filterout(const char *data_ptr,int data_size){
        //printf("worker %08X filter out --> %s!\n",this,std::string(data_ptr,data_size).c_str());
        yux::spinlock::scoped_lock lock(mutex);
        if ((status_&close_bit)==0){
            if ((status_&(write_bit|blockwrite_bit))==0){
                status_|=(write_bit&write_lbit); // WRITE=00 -> WRITE=01
                int write_limit=0;
                char *write_buffer=get_write_filter().get_write_buffer(&write_limit);
                if (data_size>write_limit){
                    // 如果数据长度大于缓冲区，则暂存数据的后一部分
                    write_cache_.append(data_ptr+write_limit,data_size-write_limit);
                    data_size=write_limit;
                }
                memcpy(write_buffer,data_ptr,data_size);
                io_write(write_buffer,data_size);
            }else{
                write_cache_.append(data_ptr,data_size);
            }
        }
    }

private:

    virtual void io_close()=0;
    virtual void io_read(char *buffer,int buffer_size)=0;
    virtual void io_write(const char *data_ptr,int data_size)=0;
    virtual void io_destroy()=0;

public:

    void write(const char *data_ptr,int data_size){
        get_write_filter().filter_write(data_ptr,data_size);
    }

    void close(){
        {
            yux::spinlock::scoped_lock lock(mutex);
            if (status_&close_bit){ // CLOSE=01/10/11
                return;
            }
            status_|=close_lbit; // CLOSE=00 -> CLOSE=01
        }
        io_close();
        get_handler().handle_close();
        {
            yux::spinlock::scoped_lock lock(mutex);
            status_|=close_bit; // CLOSE=11
            if ((status_&(willread_bit|read_bit))==0){
                status_|=read_lbit;
                lock.release();
                process_error(worker_param::error_type_read,ENOENT);
                yux::spinlock::scoped_lock lock2(mutex);
                __prepare_destroy(lock2);
            }else{
                __prepare_destroy(lock);
            }
        }
    }

    void trigger_read(bool enable){
        yux::spinlock::scoped_lock lock(mutex);
        if (enable){
            if ((status_&close_bit)==0){
                status_|=willread_bit;
                if ((status_&read_bit)==0){
                    status_|=read_lbit;
                    int read_limit=0;
                    char *read_buffer=get_read_filter().get_read_buffer(&read_limit);
                    io_read(read_buffer,read_limit);
                }
            }
        }else{
            status_&=~willread_bit;
        }
    }

    void final_write(){
        yux::spinlock::scoped_lock lock(mutex);
        if ((status_&write_bit)==0){
            status_|=write_lbit;
            if ((status_&finalwrite_bit)==0){
                status_|=finalwrite_bit;
                lock.release();
                process_error(worker_param::error_type_write,ENOENT);
            }
        }else{
            status_|=finalwrite_bit;
        }
    }

    int get_cached_size(){
        yux::spinlock::scoped_lock lock(mutex);
        return write_cache_.length();
    }

private:

    void __prepare_destroy(yux::spinlock::scoped_lock &lock){
        //printf("C-2/W-2/WILLR-1/R-2 --> %08X --> %08X CHECK:%08X+%d\n",
        //       status_,this,status_&(read_lbit|write_lbit|close_bit),refcnt_);
        if ((refcnt_==0)&&((status_&(read_lbit|write_lbit|close_bit))==close_bit)){
            lock.release();
            io_destroy();
        }
    }

protected:

    void block_write(){
        status_|=blockwrite_bit; // 为了引导写请求全部转入缓存
    }

    void unblock_write(){
        bool let_write=false;
        {
            yux::spinlock::scoped_lock lock(mutex);
            status_&=~blockwrite_bit; // 消除写阻塞
            if ((status_&write_bit)==0){
                status_|=write_lbit; // 产生写请求
                let_write=true;
            }
        }
        if (let_write){
            process_write();
        }
    }

    void process_write(){
        BOOST_ASSERT ((status_&write_bit)==write_lbit);
        bool write_pending=false;
        int write_limit=0;
        char *write_buffer=get_write_filter().get_write_buffer(&write_limit);
        {
            yux::spinlock::scoped_lock lock(mutex);
            int poped_size=write_cache_.pop(write_buffer,write_limit);
            if (poped_size!=0){
                io_write(write_buffer,poped_size);
            }else{
                write_pending=true;
            }
        }
        if (write_pending){
            //printf("worker ## %08X filter notify -- %d\n",this,get_cached_size());
            get_write_filter().filter_notify();
            yux::spinlock::scoped_lock lock(mutex);
            if (write_cache_.length()!=0){
                int poped_size=write_cache_.pop(write_buffer,write_limit);
                io_write(write_buffer,poped_size);
            }else{
                if (status_&finalwrite_bit){
                    lock.release(); // 丢弃旧的锁
                    process_error(worker_param::error_type_write,ENOENT);
                }else{
                    status_&=~write_bit; // WRITE=00
                    __prepare_destroy(lock);
                }
            }
        }
    }

    void process_read(const char *data_ptr,int data_size){
        BOOST_ASSERT ((status_&read_bit)==read_lbit); // READ==01
        get_read_filter().filter_read(data_ptr,data_size);
        yux::spinlock::scoped_lock lock(mutex);
        if (status_&willread_bit){
            int read_limit=0;
            char *read_buffer=get_read_filter().get_read_buffer(&read_limit);
            io_read(read_buffer,read_limit);
        }else{
            status_&=~read_bit; // READ=00
            __prepare_destroy(lock);
        }
    }

    void process_error(int type,int err_code){
        get_handler().handle_error(type,err_code);
        yux::spinlock::scoped_lock lock(mutex);
        if (type==worker_param::error_type_read){
            BOOST_ASSERT((status_&read_bit)==read_lbit); // READ==01
            status_^=read_bit; // READ=01 --> READ=10
        }else if(type==worker_param::error_type_write){
            BOOST_ASSERT((status_&write_bit)==write_lbit); // WRITE==01
            status_^=write_bit; // WRITE=01 --> WRITE=10
        }
        __prepare_destroy(lock);
    }

private:

    int status_;
    int refcnt_;
    yux::pool_string write_cache_;
    yux::spinlock mutex;

};

}

#endif // YUX_WORKER_CONTROLLER_IMPL_HPP_
