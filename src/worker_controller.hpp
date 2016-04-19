#ifndef YUX_WORKER_CONTROLLER_HPP_
#define YUX_WORKER_CONTROLLER_HPP_

#include "stddef.h"

namespace yux{

    class write_sink;
    class read_sink;

    struct worker_param{

        enum error_type{ error_type_read=0 , error_type_write=1 ,
            error_type_connect=2 , error_type_other=3 };

    };

    class worker_controller{

        friend class yux::write_sink;
        friend class yux::read_sink;

    public:

        worker_controller(){
            shared_data_=NULL;
        }

        virtual ~worker_controller(){
            //
        }

    public:

        virtual void write(const char *data_ptr,int data_size)=0;
        virtual void close()=0;
        virtual void trigger_read(bool enable)=0;
        virtual void final_write()=0;
        virtual int get_cached_size()=0;

    public:

        virtual void ref()=0;
        virtual void unref(bool can_be_last_unref=true)=0;

    private:

        virtual void io_filterin(const char *data_ptr,int data_size)=0;
        virtual void io_filterout(const char *data_ptr,int data_size)=0;

    public:

        void* get_shared(){
            return shared_data_;
        }

        void set_shared(void *user_data){
            shared_data_=user_data;
        }

    private:

        void *shared_data_;


    };

}

#endif // YUX_WORKER_CONTROLLER_HPP_
