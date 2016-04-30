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

#ifndef YUX_ASIO_MULTITHREAD_SERVICE_HPP_
#define YUX_ASIO_MULTITHREAD_SERVICE_HPP_

#include "boost/asio.hpp"
#include "boost/thread.hpp"

#include "yux/spinlock.hpp"

namespace yux{
namespace asio{

class asio_multithread_service{

    typedef boost::asio::io_service::work* holder_work_ptr;
    typedef boost::thread* thread_ptr;

public:

    asio_multithread_service(int worker_service_num){
        BOOST_ASSERT(worker_service_num>0);
        hot_worker_index_=0;
        worker_service_num_=worker_service_num;
        worker_services_=new boost::asio::io_service[worker_service_num];
        holder_work_ptrs_=new holder_work_ptr[worker_service_num];
        worker_service_thread_ptrs_=new thread_ptr[worker_service_num];
        for (int i=0;i<worker_service_num;i++){
            holder_work_ptrs_[i]=new boost::asio::io_service::work(worker_services_[i]);
            worker_service_thread_ptrs_[i]=new boost::thread(boost::bind(&boost::asio::io_service::run,&(worker_services_[i])));
        }
    }

    ~asio_multithread_service(){
        stop();
    }

public:

    boost::asio::io_service& get_service(){
        yux::spinlock::scoped_lock lock(hot_worker_mutex_);
        if (hot_worker_index_==0){
            hot_worker_index_=worker_service_num_;
        }
        hot_worker_index_--;
        return worker_services_[hot_worker_index_];
    }

public:

    void stop(){
        printf("asio multithread service stoping...\n");
        for (int i=0;i<worker_service_num_;i++){
            delete holder_work_ptrs_[i];
            printf("asio multithread service thread[%d] prepare stop!\n",i);
            worker_service_thread_ptrs_[i]->join();
            printf("asio multithread service thread[%d] stopped!\n",i);
            delete worker_service_thread_ptrs_[i];
        }
        delete [] holder_work_ptrs_;
        delete [] worker_service_thread_ptrs_;
        delete [] worker_services_;
    }

private:

    int worker_service_num_;
    boost::asio::io_service *worker_services_;
    holder_work_ptr *holder_work_ptrs_;
    thread_ptr *worker_service_thread_ptrs_;
    yux::spinlock hot_worker_mutex_;
    int hot_worker_index_;

};

}
}

#endif // YUX_ASIO_MULTITHREAD_SERVICE_HPP_
