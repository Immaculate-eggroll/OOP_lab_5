#pragma once
#include <memory_resource>
#include <iterator>
#include <utility>

template <typename T>
class pmr_slist {
private:
    struct Node {
        T value;
        Node* next;
        template<typename... Args>
        Node(Args&&... args) : value(std::forward<Args>(args)...), next(nullptr) {}
    };

    using Alloc = std::pmr::polymorphic_allocator<Node>;
    Alloc alloc_;
    Node* head_;

public:
    explicit pmr_slist(std::pmr::memory_resource* mr = std::pmr::get_default_resource())
        : alloc_(mr), head_(nullptr) {}

    ~pmr_slist() { clear(); }

    pmr_slist(const pmr_slist&) = delete;
    pmr_slist& operator=(const pmr_slist&) = delete;

    pmr_slist(pmr_slist&& other) noexcept
        : alloc_(other.alloc_), head_(other.head_) {
        other.head_ = nullptr;
    }

    pmr_slist& operator=(pmr_slist&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = other.head_;
            other.head_ = nullptr;
        }
    return *this;
}

    template<typename... Args>
    void push_front(Args&&... args) {
        Node* n = alloc_.allocate(1);
        new (n) Node(std::forward<Args>(args)...);
        n->next = head_;
        head_ = n;
    }

    void pop_front() {
        if (!head_) return;
        Node* old = head_;
        head_ = head_->next;
        old->~Node();
        alloc_.deallocate(old, 1);
    }

    void clear() {
        while (head_)
            pop_front();
    }

    bool empty() const { return head_ == nullptr; }

    class iterator {
        Node* ptr_;
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;

        iterator(Node* p) : ptr_(p) {}
        T& operator*() const { return ptr_->value; }
        iterator& operator++() { ptr_ = ptr_->next; return *this; }
        bool operator==(const iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const iterator& other) const { return ptr_ != other.ptr_; }
    };

    iterator begin() { return iterator(head_); }
    iterator end() { return iterator(nullptr); }
};
