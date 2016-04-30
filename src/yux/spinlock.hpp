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

#ifndef YUX_SPINLOCK_HPP
#define YUX_SPINLOCK_HPP

#include <boost/smart_ptr/detail/spinlock.hpp>

namespace yux{

	class spinlock{

		boost::detail::spinlock mutex_;

	public:

		class scoped_lock{

		    spinlock &mutex_;
		    bool locked_;

        public:

            scoped_lock(spinlock &mutex):mutex_(mutex){
                mutex_.lock();
                locked_=true;
            }

            ~scoped_lock(){
                if (locked_){
                    mutex_.unlock();
                }
            }

        public:

            void release(){
                //BOOST_ASSERT(locked_);
                locked_=false;
                mutex_.unlock();
            }

		};

	public:

		spinlock(){
			mutex_.v_=0;
		}

	public:

		void lock(){
			mutex_.lock();
		}

		void unlock(){
			mutex_.unlock();
		}

	};


}


#endif // YUX_SPINLOCK_HPP
