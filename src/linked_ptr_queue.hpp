#ifndef YUX_LINKED_PTR_LIST_HPP_
#define YUX_LINKED_PTR_LIST_HPP_

#include "stddef.h"

#include "boost/assert.hpp"

namespace yux {

template <typename T>
struct linked_ptr_wrapper {
    linked_ptr_wrapper<T> *next;
    T data;
};

template<typename NodeType>
class linked_ptr_queue {

    typedef NodeType node_type;

    node_type *head_ptr_;
    node_type *tail_ptr_;

public:

    linked_ptr_queue() {
        head_ptr_=NULL;
        tail_ptr_=NULL;
    }

    linked_ptr_queue(node_type *head_ptr,node_type *tail_ptr) {
        head_ptr_=head_ptr;
        tail_ptr_=tail_ptr;
    }

public:

    void add_head(node_type *node_ptr) {
        BOOST_ASSERT(node_ptr!=NULL);
        if (head_ptr_) {
            node_ptr->next=head_ptr_;
            head_ptr_=node_ptr;
        } else {
            head_ptr_=tail_ptr_=node_ptr;
        }
    }

    void add_tail(node_type *node_ptr) {
        BOOST_ASSERT(node_ptr!=NULL);
        if (tail_ptr_) {
            tail_ptr_->next=node_ptr;
            tail_ptr_=node_ptr;
        } else {
            head_ptr_=tail_ptr_=node_ptr;
        }
        node_ptr->next=NULL;
    }

    bool is_empty() {
        return head_ptr_==NULL;
    }

    node_type *remove_head() {
        node_type *node_ptr=head_ptr_;
        if (node_ptr) {
            head_ptr_=node_ptr->next;
            if (!head_ptr_) {
                tail_ptr_=NULL;
            }
        }
        return node_ptr;
    }

    node_type *get_tail(){
        return tail_ptr_;
    }

    void add_tail(linked_ptr_queue *queue_ptr) {
        node_type *node_ptr=queue_ptr->head_ptr_;
        if (node_ptr) {
            if (tail_ptr_) {
                tail_ptr_->next=queue_ptr->head_ptr_;
            } else {
                head_ptr_=node_ptr;
            }
            tail_ptr_=queue_ptr->tail_ptr_;
        }
    }

    void add_head(linked_ptr_queue *queue_ptr) {
        node_type *node_ptr=queue_ptr->head_ptr_;
        if (node_ptr) {
            queue_ptr->tail_ptr_->next=head_ptr_;
            head_ptr_=node_ptr;
            if (!tail_ptr_) {
                tail_ptr_=queue_ptr->tail_ptr_;
            }
        }
    }

    void clear() {
        head_ptr_=NULL;
        tail_ptr_=NULL;
    }

};

}

#endif // YUX_LINKED_PTR_LIST_HPP_
