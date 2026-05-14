/* -*- C++ -*-
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2024 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
#ifndef tokenrangeH
#define tokenrangeH
//---------------------------------------------------------------------------

#include "config.h"

#include <cstddef>
#include <iterator>
#include <type_traits>

template<typename T>
class RangeBase {
    T* mFront;
    T* mBack;

public:
    RangeBase(T* front, T* back) : mFront(front), mBack(back) {}

    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using value_type = T*;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = T*;

        explicit Iterator(T* t) : mt(t) {}
        Iterator& operator++() {
            //mt = mt->next();
            return *this;
        }
        bool operator==(const Iterator& b) const {
            return mt == b.mt;
        }
        bool operator!=(const Iterator& b) const {
            return mt != b.mt;
        }
        T* operator*() const {
            return mt;
        }
    protected:
        T* mt;
    };

    Iterator begin() const {
        return Iterator(mFront);
    }
    Iterator end() const {
        return Iterator(mBack);
    }
};

#endif // tokenrangeH
