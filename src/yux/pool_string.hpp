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

#ifndef YUX_POOL_STRING_HPP_
#define YUX_POOL_STRING_HPP_

#include <string>
#include <boost/noncopyable.hpp>

#include "yux/spinlock.hpp"
#include "yux/linked_ptr_queue.hpp"
#include "yux/fixed_allocator/bigblock_allocator.hpp"

namespace yux{

	struct pool_string_block{

		enum { chunk_size=128 , chunk_capacity=chunk_size-sizeof(pool_string_block *)-sizeof(unsigned short)*2 };

		pool_string_block *next;
		unsigned short block_used;
		unsigned short block_offset;
		char block_data[chunk_capacity];

	};

	struct pool_string_data{

		char *data;
		int size;

	};

	class pool_string_iterator{

		pool_string_block *block_ptr_;
		pool_string_data data_;

	public:

		pool_string_iterator(){
			block_ptr_=NULL;
		}

		pool_string_iterator(pool_string_block *block_ptr){
			block_ptr_=block_ptr;
			data_.data=block_ptr->block_data+block_ptr->block_offset;
			data_.size=block_ptr->block_used;
		}

	public:

		pool_string_data * operator ->(){
			return &data_;
		}

		pool_string_iterator& operator ++(int){
			if (block_ptr_){
				block_ptr_=block_ptr_->next;
				if (block_ptr_!=NULL){
					data_.data=block_ptr_->block_data+block_ptr_->block_offset;
					data_.size=block_ptr_->block_used;
				}
			}
			return *this;
		}

		bool operator !=(pool_string_iterator &iter){
			return iter.block_ptr_!=block_ptr_;
		}

	};

	class pool_string_zone{

	    yux::fixed_allocator::bigblock_allocator<pool_string_block> pool_;
	    yux::spinlock pool_mutex_;
	    linked_ptr_queue<pool_string_block> free_queue_;
	    int total_cnt_;

    public:

        pool_string_zone():pool_(1024){
            total_cnt_=0;
        }

    public:

        int get_total_count(){
            yux::spinlock::scoped_lock lock(pool_mutex_);
            return total_cnt_;
        }

    public:

		pool_string_block *allocate(){
		    //printf("------------------ block malloc! --> %08x\n",this);
			yux::spinlock::scoped_lock lock(pool_mutex_);
			pool_string_block * block_ptr=free_queue_.remove_head();
			if (!block_ptr){
                if (block_ptr=pool_.allocate()){
                    total_cnt_++;
                    //printf("true malloc! -- %d\n",total_cnt_);
                }
			}
			return block_ptr;
		}

		void deallocate(pool_string_block *block_ptr){
		    //printf("------------------ block free! --> %08x\n",this);
			yux::spinlock::scoped_lock lock(pool_mutex_);
			total_cnt_--;
			free_queue_.add_tail(block_ptr);
		}

		void deallocate(pool_string_block *start_block_ptr,pool_string_block *end_block_ptr){
		    //printf("------------------ block batch free! --> %08x\n",this);
		    linked_ptr_queue<pool_string_block> tmp_queue(start_block_ptr,end_block_ptr);
		    pool_string_block *block_ptr=start_block_ptr;
		    int recycle_times=0;
		    while (block_ptr){
                recycle_times++;
#ifdef _DEBUG
                if (block_ptr->next==NULL){
                    BOOST_ASSERT(block_ptr==end_block_ptr);
                }
#endif
                block_ptr=block_ptr->next;
		    }
		    yux::spinlock::scoped_lock lock(pool_mutex_);
		    total_cnt_+=recycle_times;
		    free_queue_.add_tail(&tmp_queue);
		}

	};


	class pool_string_buffered_writer{

        char *buffer_;
        int buffer_used_;

    public:

        pool_string_buffered_writer(char *buffer){
            buffer_=buffer;
            buffer_used_=0;
        }

    public:

        void operator ()(const char *data_ptr,int data_size){
            memcpy(buffer_+buffer_used_,data_ptr,data_size);
            buffer_used_+=data_size;
        }

	};

	class pool_string:public boost::noncopyable{

		typedef pool_string_block block_type;

		pool_string_zone *zone_ptr_;
		bool zone_owner_;
		pool_string_block *head_block_ptr_;
		pool_string_block *tail_block_ptr_;
		char *write_ptr_;
		int write_space_;
		int length_;

	public:

		typedef pool_string_iterator iterator;

	public:

		pool_string(){
		    static pool_string_zone zone;
		    zone_ptr_=&zone;
			initialize();
		}

		pool_string(pool_string_zone &zone){
		    zone_ptr_=&zone;
		    initialize();
		}

		~pool_string(){
		    free_blocks();
		}

	private:

		void initialize(){
			head_block_ptr_=tail_block_ptr_=NULL;
			write_ptr_=NULL;
			write_space_=0;
			length_=0;
		}

		bool ensure_first_block(){
            if (head_block_ptr_!=NULL){
                return true;
            }
            head_block_ptr_=tail_block_ptr_=zone_ptr_->allocate();
            if (head_block_ptr_==NULL){
                printf("no memory to allocate for pool string.\n");
                return false;
            }
            head_block_ptr_->next=NULL;
            head_block_ptr_->block_used=0;
            head_block_ptr_->block_offset=0;
            write_ptr_=head_block_ptr_->block_data;
            write_space_=pool_string_block::chunk_capacity;
            return true;
        }

	public:

		template <typename AppendFunction>
		int pop_lambda(int pop_len,AppendFunction append_fn){
			pool_string_block *block_ptr=head_block_ptr_;
			if (block_ptr==NULL){
                // 如果缓冲区指针为NULL，则不弹出任何字符串
                return 0;
			}
			int buffer_used=0;
			while (true){
				// if block data is bigger than buffer space ,
				// cut block data to buffer and resize the block data
				if (block_ptr->block_used+buffer_used>pop_len){
                    int block_using=pop_len-buffer_used;
                    if (block_using!=0){
                        append_fn(block_ptr->block_data+block_ptr->block_offset,block_using);
                        block_ptr->block_used-=block_using;
                        block_ptr->block_offset+=block_using;
                        buffer_used=pop_len;
                    }
					break;
				}
				// else if block data is smaller than buffer space or just fit in bufferspace
				// fill all block data to buffer space
				append_fn(block_ptr->block_data+block_ptr->block_offset,block_ptr->block_used);
				buffer_used+=block_ptr->block_used;
				if (block_ptr->next==NULL){
					// reserve last block for append
					block_ptr->block_used=0;
					block_ptr->block_offset=0;
					break;
				}
				pool_string_block *raw_block_ptr=block_ptr;
				block_ptr=block_ptr->next;
				zone_ptr_->deallocate(raw_block_ptr);
			}
			if (block_ptr->next==NULL){
                int block_using=block_ptr->block_offset+block_ptr->block_used;
				write_ptr_=block_ptr->block_data+block_using;
				write_space_=pool_string_block::chunk_capacity-block_using;
			}
			head_block_ptr_=block_ptr;
			length_-=buffer_used;
			return buffer_used;
		}

		int pop(int buffer_len){
			return pop_lambda(buffer_len,&pool_string::blackhole_writer);
		}

		int pop(char *buffer,int buffer_len){
			BOOST_ASSERT(buffer!=NULL);
			pool_string_buffered_writer writer(buffer);
			return pop_lambda(buffer_len,writer);
		}

	public:

		void append(const char *data_ptr,int data_size){
		    if (!ensure_first_block()){
                return;
		    }
			length_+=data_size;
			//printf("length_=%d , write_space_=%d \n",length_,write_space_);
			if (data_size>=write_space_){
				// fill last block first , mark last block full
				memcpy(write_ptr_,data_ptr,write_space_);
				tail_block_ptr_->block_used+=write_space_;
				// move pointer and counter for left data
				data_ptr+=write_space_;
				data_size-=write_space_;
				// calc how many block need create
				// if no data left , create one for future
				int block_num=data_size/pool_string_block::chunk_capacity+1;
				for (int i=1;i<block_num;i++){
					// create new block , fill with full data
					pool_string_block *last_block_pointer=zone_ptr_->allocate();
					if (last_block_pointer==NULL){
                        // 劳资没有内存资源了!
                        printf("no memory to allocate for pool string.\n");
                        return;
					}
                    last_block_pointer->block_used=pool_string_block::chunk_capacity;
                    last_block_pointer->block_offset=0;
                    last_block_pointer->next=NULL;
                    memcpy(last_block_pointer->block_data,data_ptr,pool_string_block::chunk_capacity);
                    // move next
                    data_ptr+=pool_string_block::chunk_capacity;
                    data_size-=pool_string_block::chunk_capacity;
                    // link last block to new block , mark new block as last block
                    tail_block_ptr_->next=last_block_pointer;
                    tail_block_ptr_=last_block_pointer;
				}
				pool_string_block *last_block_pointer=zone_ptr_->allocate();
				if (last_block_pointer==NULL){
                    // 劳资没有内存资源了!
                    printf("no memory to allocate for pool string.\n");
                    return;
				}
                last_block_pointer->block_used=data_size;
                last_block_pointer->block_offset=0;
                last_block_pointer->next=NULL;
                if (data_size!=0){
                    memcpy(last_block_pointer->block_data,data_ptr,data_size);
                }
                // link last block to new block , mark new block as last block
                tail_block_ptr_->next=last_block_pointer;
                tail_block_ptr_=last_block_pointer;
                // set write setting for next write
                write_ptr_=last_block_pointer->block_data+data_size;
                write_space_=pool_string_block::chunk_capacity-data_size;
			}else{
				memcpy(write_ptr_,data_ptr,data_size);
				write_ptr_+=data_size;
				write_space_-=data_size;
				tail_block_ptr_->block_used+=data_size;
			}
		}

		void append(char ch){
		    if (!ensure_first_block()){
                return;
		    }
			length_++;
			tail_block_ptr_->block_used++;
			if (write_space_==1){
				*write_ptr_=ch;
				pool_string_block *last_block_pointer=zone_ptr_->allocate();
				if (last_block_pointer==NULL){
                    // 劳资没有内存资源了!
                    printf("no memory to allocate for pool string.\n");
                    return;
				}
                last_block_pointer->next=NULL;
                last_block_pointer->block_used=0;
                last_block_pointer->block_offset=0;
                write_ptr_=last_block_pointer->block_data;
                write_space_=pool_string_block::chunk_capacity;
                tail_block_ptr_->next=last_block_pointer;
                tail_block_ptr_=last_block_pointer;
			}else{
				*(write_ptr_++)=ch;
				write_space_--;
			}
		}

	public:

		pool_string& operator +=(char ch){
			append(ch);
			return *this;
		}

		pool_string& operator +=(const char *str){
			append(str,strlen(str));
			return *this;
		}

		pool_string& operator +=(const std::string &str){
			append(str.c_str(),str.size());
			return *this;
		}

		pool_string& operator +=(pool_string& input_string){
			for (pool_string::iterator it=input_string.begin();it!=input_string.end();it++){
				if (it->size){
					append(it->data,it->size);
				}
			}
			return *this;
		}

	public:

		iterator begin(){
		    if (length_!=0){
                return pool_string_iterator(head_block_ptr_);
		    }else{
                return end();
		    }
		}

		iterator& end(){
			static pool_string_iterator emtpy_iterator;
			return emtpy_iterator;
		}

		void clear(){
            // head_block_ptr_不是NULL，代表被分配过空间
		    if (head_block_ptr_!=NULL){
                free_blocks();
                initialize();
		    }
		}

		std::string str(){
			std::string ret;
			for (pool_string::iterator it=begin();it!=end();it++){
				ret.append(it->data,it->size);
			}
			return ret;
		}

		int size(){
			return length_;
		}

		int length(){
			return length_;
		}

		bool empty(){
			return length_==0;
		}

    public:

        void use_zone(pool_string_zone &zone){
            clear();
            zone_ptr_=&zone;
        }

    private:

		static void blackhole_writer(const char *data_ptr,int data_size){
		    // for pop function
		}

        void free_blocks(){
            if (head_block_ptr_!=NULL){
                zone_ptr_->deallocate(head_block_ptr_,tail_block_ptr_);
            }
		}

	};

}

#endif // YUX_POOL_STRING_HPP_
