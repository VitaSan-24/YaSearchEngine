/*
 * paginator.h
 *
 *  Created on: 7 сент. 2024 г.
 *      Author: vitasan
 */

#pragma once
#include <vector>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <iostream>

template<typename Container>
auto Paginate(const Container &c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template<typename Iterator>
class IteratorRange {
public:

    explicit IteratorRange(const Iterator &p_begin, const Iterator &p_end) {
        begin_ = p_begin;
        end_ = p_end;
        if (p_begin == p_end) {
            size_ = 1;
        } else {
            for (Iterator it = p_begin; it != p_end; ++it) {
                ++size_;
            }
        }
    }

    auto begin() const {
        return begin_;
    }
    auto end() const {
        return end_;
    }
    size_t size() const {
        return size_;
    }

private:
    Iterator begin_;
    Iterator end_;
    size_t size_ = 0;
};

template<typename Iterator>
class Paginator {
public:

    explicit Paginator(const Iterator &p_begin, const Iterator &p_end,
            size_t page_size) {

        for (Iterator it = p_begin; it != p_end; ++it) {
            ++size_;
        }
        if (size_ == 0) {
            using std::operator""s;
            throw std::invalid_argument("Ничего не найдено"s);
        }

        Iterator p_begin_tmp = p_begin;
        Iterator p_end_tmp = p_end;
        for (size_t left = distance(p_begin_tmp, p_end_tmp); left > 0;) {
            const size_t current_page_size = std::min(page_size, left);
            const Iterator current_page_end = next(p_begin_tmp,
                    current_page_size);
            IteratorRange result(p_begin_tmp, current_page_end); // @suppress("Type cannot be resolved")
            pages_.push_back(result);
            left -= current_page_size;
            p_begin_tmp = current_page_end;
        }
    }

    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
    size_t size_ = 0;
};

