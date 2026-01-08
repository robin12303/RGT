// CircularBuffer.hpp
#pragma once
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
class CircularBuffer {
public:
    explicit CircularBuffer(std::size_t capacity)
        : buf_(capacity), cap_(capacity), head_(0), size_(0) {
        if (cap_ == 0) throw std::invalid_argument("capacity must be > 0");
    }

    // 기본 복사/이동  
    CircularBuffer(const CircularBuffer&) = default;
    CircularBuffer& operator=(const CircularBuffer&) = default;
    CircularBuffer(CircularBuffer&&) noexcept = default;
    CircularBuffer& operator=(CircularBuffer&&) noexcept = default;
    ~CircularBuffer() = default;

    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return cap_; }
    bool empty() const noexcept { return size_ == 0; }

    // 가장 오래된 데이터
    T& front() {
        if (empty()) throw std::out_of_range("front() on empty buffer");
        return buf_[head_];
    }
    const T& front() const {
        if (empty()) throw std::out_of_range("front() on empty buffer");
        return buf_[head_];
    }

    // 가장 최근 데이터
    T& back() {
        if (empty()) throw std::out_of_range("back() on empty buffer");
        return buf_[physIndex(size_ - 1)];
    }
    const T& back() const {
        if (empty()) throw std::out_of_range("back() on empty buffer");
        return buf_[physIndex(size_ - 1)];
    }

    void push_back(const T& item) {
        if (size_ < cap_) {
            buf_[physIndex(size_)] = item;
            ++size_;
        } else {
            // overwrite oldest
            buf_[head_] = item;
            head_ = (head_ + 1) % cap_;
        }
    }

    void push_back(T&& item) {
        if (size_ < cap_) {
            buf_[physIndex(size_)] = std::move(item);
            ++size_;
        } else {
            buf_[head_] = std::move(item);
            head_ = (head_ + 1) % cap_;
        }
    }

    void pop_front() {
        if (empty()) throw std::out_of_range("pop_front() on empty buffer");
        head_ = (head_ + 1) % cap_;
        --size_;
    }

    // ===== iterator (forward) =====
private:
    template <bool IsConst>
    class Iter {
        using BufPtr = std::conditional_t<IsConst, const CircularBuffer*, CircularBuffer*>;
        BufPtr owner_ = nullptr;
        std::size_t offset_ = 0; // 0..size (논리적 위치)

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using reference         = std::conditional_t<IsConst, const T&, T&>;
        using pointer           = std::conditional_t<IsConst, const T*, T*>;

        Iter() = default;
        Iter(BufPtr owner, std::size_t off) : owner_(owner), offset_(off) {}

        reference operator*() const {
            return owner_->buf_[owner_->physIndex(offset_)];
        }
        pointer operator->() const { return std::addressof(operator*()); }

        Iter& operator++() { ++offset_; return *this; }
        Iter operator++(int) { Iter tmp = *this; ++(*this); return tmp; }

        friend bool operator==(const Iter& a, const Iter& b) {
            return a.owner_ == b.owner_ && a.offset_ == b.offset_;
        }
        friend bool operator!=(const Iter& a, const Iter& b) {
            return !(a == b);
        }
    };

public:
    using iterator = Iter<false>;
    using const_iterator = Iter<true>;

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size_); }

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, size_); }

    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, size_); }

private:
    // offset(논리) -> 실제 인덱스(물리)
    std::size_t physIndex(std::size_t logicalOffset) const noexcept {
        return (head_ + logicalOffset) % cap_;
    }

private:
    std::vector<T> buf_;
    std::size_t cap_;
    std::size_t head_; // oldest index
    std::size_t size_; // number of valid items
};
