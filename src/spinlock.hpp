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
