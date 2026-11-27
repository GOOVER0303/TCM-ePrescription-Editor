#pragma once

#include <cstddef>
#include <optional>
#include <utility>

namespace tcm {

template <typename T>
class DoublyLinkedStack {
private:
    struct Node {
        T value;
        Node* prev{nullptr};
        Node* next{nullptr};
    };

public:
    DoublyLinkedStack() = default;
    DoublyLinkedStack(const DoublyLinkedStack&) = delete;
    DoublyLinkedStack& operator=(const DoublyLinkedStack&) = delete;

    DoublyLinkedStack(DoublyLinkedStack&& other) noexcept
        : top_(other.top_), bottom_(other.bottom_), size_(other.size_) {
        other.top_ = nullptr;
        other.bottom_ = nullptr;
        other.size_ = 0;
    }

    DoublyLinkedStack& operator=(DoublyLinkedStack&& other) noexcept {
        if (this != &other) {
            clear();
            top_ = other.top_;
            bottom_ = other.bottom_;
            size_ = other.size_;
            other.top_ = nullptr;
            other.bottom_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    ~DoublyLinkedStack() { clear(); }

    void push(T value) {
        Node* node = new Node{std::move(value), top_, nullptr};
        if (top_ != nullptr) {
            top_->next = node;
        } else {
            bottom_ = node;
        }
        top_ = node;
        ++size_;
    }

    std::optional<T> pop() {
        if (top_ == nullptr) {
            return std::nullopt;
        }
        Node* oldTop = top_;
        top_ = oldTop->prev;
        if (top_ != nullptr) {
            top_->next = nullptr;
        } else {
            bottom_ = nullptr;
        }
        T value = std::move(oldTop->value);
        delete oldTop;
        --size_;
        return std::optional<T>(std::move(value));
    }

    T* peek() { return top_ == nullptr ? nullptr : &top_->value; }
    const T* peek() const { return top_ == nullptr ? nullptr : &top_->value; }

    bool removeOldest() {
        if (bottom_ == nullptr) {
            return false;
        }
        Node* oldBottom = bottom_;
        bottom_ = oldBottom->next;
        if (bottom_ != nullptr) {
            bottom_->prev = nullptr;
        } else {
            top_ = nullptr;
        }
        delete oldBottom;
        --size_;
        return true;
    }

    void clear() {
        Node* node = bottom_;
        while (node != nullptr) {
            Node* next = node->next;
            delete node;
            node = next;
        }
        top_ = nullptr;
        bottom_ = nullptr;
        size_ = 0;
    }

    bool empty() const { return top_ == nullptr; }
    std::size_t size() const { return size_; }

    template <typename Function>
    void forEachTopDown(Function function) const {
        for (const Node* node = top_; node != nullptr; node = node->prev) {
            function(static_cast<const T&>(node->value));
        }
    }

    bool checkInvariants() const {
        if ((top_ == nullptr) != (bottom_ == nullptr) || (top_ == nullptr && size_ != 0)) {
            return false;
        }
        if (top_ != nullptr && top_->next != nullptr) {
            return false;
        }
        if (bottom_ != nullptr && bottom_->prev != nullptr) {
            return false;
        }

        std::size_t count = 0;
        const Node* previous = nullptr;
        for (const Node* node = bottom_; node != nullptr; node = node->next) {
            if (node->prev != previous) {
                return false;
            }
            previous = node;
            ++count;
        }
        if (previous != top_ || count != size_) {
            return false;
        }

        count = 0;
        const Node* next = nullptr;
        for (const Node* node = top_; node != nullptr; node = node->prev) {
            if (node->next != next) {
                return false;
            }
            next = node;
            ++count;
        }
        return count == size_ && next == bottom_;
    }

private:
    Node* top_{nullptr};
    Node* bottom_{nullptr};
    std::size_t size_{0};
};

}  // namespace tcm
