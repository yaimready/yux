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

#ifndef YUX_ASIO_TCP_DATA_HPP_
#define YUX_ASIO_TCP_DATA_HPP_

#include "boost/asio.hpp"

#include "yux/fixed_allocator/bigblock_allocator.hpp"
#include "yux/fixed_allocator/object_allocator_proxy.hpp"

namespace yux{
namespace asio{

class asio_tcp_data{

public:

    asio_tcp_data(boost::asio::io_service &service,void *alloactor_ptr):socket_(service){
        alloactor_ptr_=alloactor_ptr;
        destroyer_ptr_=NULL;
    }

public:

    void set_worker_destroyer(boost::function1<void,void *> *destroyer_ptr){
        destroyer_ptr_=destroyer_ptr;
    }

public:

    boost::asio::ip::tcp::socket& get_socket(){
        return socket_;
    }

    void destroy_self();

    void destroy_worker(void *worker_ptr){
        BOOST_ASSERT(destroyer_ptr_!=NULL);
        (*destroyer_ptr_)(worker_ptr);
    }

private:

    boost::asio::ip::tcp::socket socket_;
    void *alloactor_ptr_;
    boost::function1<void,void *> *destroyer_ptr_;

};

typedef yux::fixed_allocator::object_allocator1<
    asio_tcp_data,
    yux::fixed_allocator::bigblock_allocator<asio_tcp_data> > asio_tcp_data_allocator;

inline void asio_tcp_data::destroy_self(){
    reinterpret_cast<asio_tcp_data_allocator *>(alloactor_ptr_)->destruct(this);
}

}
}

#endif // YUX_ASIO_TCP_DATA_HPP_
