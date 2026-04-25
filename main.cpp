
#include <iostream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <deque>

using namespace std;

namespace sjtu {

template <typename T>
class deque {
private:
    T** map;           // Array of pointers to blocks
    size_t map_size;   // Size of the map
    size_t start_block; // Index of first block in map
    size_t start_pos;   // Position within first block
    size_t end_block;   // Index of last block in map
    size_t end_pos;     // Position within last block
    size_t _size;       // Current number of elements
    
    static const size_t BLOCK_SIZE = 512;  // Elements per block
    
    void allocate_map(size_t new_map_size) {
        map = new T*[new_map_size];
        map_size = new_map_size;
        for (size_t i = 0; i < map_size; ++i) {
            map[i] = nullptr;
        }
    }
    
    void allocate_block(size_t block_index) {
        if (map[block_index] == nullptr) {
            map[block_index] = new T[BLOCK_SIZE];
        }
    }
    
    void deallocate_block(size_t block_index) {
        if (map[block_index] != nullptr) {
            delete[] map[block_index];
            map[block_index] = nullptr;
        }
    }
    
    void deallocate_all() {
        for (size_t i = 0; i < map_size; ++i) {
            deallocate_block(i);
        }
        delete[] map;
        map = nullptr;
    }
    
    void expand_map() {
        size_t new_map_size = map_size * 2;
        T** new_map = new T*[new_map_size];
        
        // Copy existing blocks to the center of new map
        size_t offset = (new_map_size - map_size) / 2;
        for (size_t i = 0; i < map_size; ++i) {
            new_map[offset + i] = map[i];
        }
        
        // Initialize new positions
        for (size_t i = 0; i < offset; ++i) {
            new_map[i] = nullptr;
        }
        for (size_t i = offset + map_size; i < new_map_size; ++i) {
            new_map[i] = nullptr;
        }
        
        delete[] map;
        map = new_map;
        map_size = new_map_size;
        
        // Update block indices
        start_block += offset;
        end_block += offset;
    }
    
    void shrink_map() {
        if (map_size <= 8) return; // Don't shrink too much
        
        // Find first and last used blocks
        size_t first_used = map_size;
        size_t last_used = 0;
        for (size_t i = 0; i < map_size; ++i) {
            if (map[i] != nullptr) {
                first_used = min(first_used, i);
                last_used = max(last_used, i);
            }
        }
        
        size_t used_range = last_used - first_used + 1;
        if (used_range * 4 >= map_size) return; // Don't shrink if still using >25% of map
        
        size_t new_map_size = max(8UL, used_range * 2);
        T** new_map = new T*[new_map_size];
        
        size_t offset = (new_map_size - used_range) / 2;
        for (size_t i = 0; i < used_range; ++i) {
            new_map[offset + i] = map[first_used + i];
        }
        
        // Initialize new positions
        for (size_t i = 0; i < offset; ++i) {
            new_map[i] = nullptr;
        }
        for (size_t i = offset + used_range; i < new_map_size; ++i) {
            new_map[i] = nullptr;
        }
        
        delete[] map;
        map = new_map;
        map_size = new_map_size;
        
        // Update block indices
        start_block = offset;
        end_block = offset + used_range - 1;
    }
    
public:
    deque() : map(nullptr), map_size(0), start_block(0), start_pos(0), 
              end_block(0), end_pos(0), _size(0) {
        allocate_map(8);
        start_block = end_block = map_size / 2;
        start_pos = end_pos = BLOCK_SIZE / 2;
    }
    
    ~deque() {
        clear();
        if (map) {
            deallocate_all();
        }
    }
    
    deque(const deque& other) : map(nullptr), map_size(0), start_block(0), start_pos(0),
                                end_block(0), end_pos(0), _size(0) {
        allocate_map(other.map_size);
        start_block = other.start_block;
        start_pos = other.start_pos;
        end_block = other.end_block;
        end_pos = other.end_pos;
        _size = other._size;
        
        // Copy all blocks
        for (size_t i = 0; i < map_size; ++i) {
            if (other.map[i] != nullptr) {
                allocate_block(i);
                for (size_t j = 0; j < BLOCK_SIZE; ++j) {
                    map[i][j] = other.map[i][j];
                }
            }
        }
    }
    
    deque& operator=(const deque& other) {
        if (this != &other) {
            clear();
            if (map) {
                deallocate_all();
            }
            
            allocate_map(other.map_size);
            start_block = other.start_block;
            start_pos = other.start_pos;
            end_block = other.end_block;
            end_pos = other.end_pos;
            _size = other._size;
            
            // Copy all blocks
            for (size_t i = 0; i < map_size; ++i) {
                if (other.map[i] != nullptr) {
                    allocate_block(i);
                    for (size_t j = 0; j < BLOCK_SIZE; ++j) {
                        map[i][j] = other.map[i][j];
                    }
                }
            }
        }
        return *this;
    }
    
    void push_front(const T& value) {
        if (_size == 0) {
            allocate_block(start_block);
            map[start_block][start_pos] = value;
            _size = 1;
            return;
        }
        
        // Move start position backward
        if (start_pos == 0) {
            if (start_block == 0) {
                expand_map();
            }
            start_block--;
            start_pos = BLOCK_SIZE - 1;
        } else {
            start_pos--;
        }
        
        allocate_block(start_block);
        map[start_block][start_pos] = value;
        _size++;
    }
    
    void push_back(const T& value) {
        if (_size == 0) {
            allocate_block(end_block);
            map[end_block][end_pos] = value;
            _size = 1;
            return;
        }
        
        // Move end position forward
        if (end_pos == BLOCK_SIZE - 1) {
            if (end_block == map_size - 1) {
                expand_map();
            }
            end_block++;
            end_pos = 0;
        } else {
            end_pos++;
        }
        
        allocate_block(end_block);
        map[end_block][end_pos] = value;
        _size++;
    }
    
    void pop_front() {
        if (_size == 0) {
            throw out_of_range("Deque is empty");
        }
        
        if (_size == 1) {
            deallocate_block(start_block);
            _size = 0;
            return;
        }
        
        // Move start position forward
        if (start_pos == BLOCK_SIZE - 1) {
            deallocate_block(start_block);
            start_block++;
            start_pos = 0;
        } else {
            start_pos++;
        }
        
        _size--;
        
        // Try to shrink map
        shrink_map();
    }
    
    void pop_back() {
        if (_size == 0) {
            throw out_of_range("Deque is empty");
        }
        
        if (_size == 1) {
            deallocate_block(end_block);
            _size = 0;
            return;
        }
        
        // Move end position backward
        if (end_pos == 0) {
            deallocate_block(end_block);
            end_block--;
            end_pos = BLOCK_SIZE - 1;
        } else {
            end_pos--;
        }
        
        _size--;
        
        // Try to shrink map
        shrink_map();
    }
    
    T& front() {
        if (_size == 0) {
            throw out_of_range("Deque is empty");
        }
        return map[start_block][start_pos];
    }
    
    const T& front() const {
        if (_size == 0) {
            throw out_of_range("Deque is empty");
        }
        return map[start_block][start_pos];
    }
    
    T& back() {
        if (_size == 0) {
            throw out_of_range("Deque is empty");
        }
        return map[end_block][end_pos];
    }
    
    const T& back() const {
        if (_size == 0) {
            throw out_of_range("Deque is empty");
        }
        return map[end_block][end_pos];
    }
    
    bool empty() const {
        return _size == 0;
    }
    
    size_t size() const {
        return _size;
    }
    
    // Iterator class for basic iteration
    class iterator {
    private:
        deque* deq;
        size_t current_block;
        size_t current_pos;
        size_t index;
        
    public:
        friend class deque;
        
        iterator() : deq(nullptr), current_block(0), current_pos(0), index(0) {}
        
        iterator(deque* d, size_t idx) : deq(d), index(idx) {
            if (deq && deq->_size > 0) {
                if (idx == 0) {
                    current_block = deq->start_block;
                    current_pos = deq->start_pos;
                } else if (idx >= deq->_size) {
                    current_block = deq->end_block;
                    current_pos = deq->end_pos + 1;
                } else {
                    // Calculate position from index
                    size_t total_pos = deq->start_pos + idx;
                    current_block = deq->start_block + total_pos / BLOCK_SIZE;
                    current_pos = total_pos % BLOCK_SIZE;
                }
            }
        }
        
        T& operator*() {
            return deq->map[current_block][current_pos];
        }
        
        T* operator->() {
            return &deq->map[current_block][current_pos];
        }
        
        iterator& operator++() {
            if (current_pos == BLOCK_SIZE - 1) {
                current_block++;
                current_pos = 0;
            } else {
                current_pos++;
            }
            index++;
            return *this;
        }
        
        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        
        iterator& operator--() {
            if (current_pos == 0) {
                current_block--;
                current_pos = BLOCK_SIZE - 1;
            } else {
                current_pos--;
            }
            index--;
            return *this;
        }
        
        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }
        
        iterator operator+(int n) const {
            iterator result = *this;
            for (int i = 0; i < n; ++i) {
                ++result;
            }
            return result;
        }
        
        iterator operator-(int n) const {
            iterator result = *this;
            for (int i = 0; i < n; ++i) {
                --result;
            }
            return result;
        }
        
        ptrdiff_t operator-(const iterator& other) const {
            return static_cast<ptrdiff_t>(index) - static_cast<ptrdiff_t>(other.index);
        }
        
        iterator& operator+=(int n) {
            if (n > 0) {
                for (int i = 0; i < n; ++i) {
                    ++(*this);
                }
            } else if (n < 0) {
                for (int i = 0; i < -n; ++i) {
                    --(*this);
                }
            }
            return *this;
        }
        
        iterator& operator-=(int n) {
            return operator+=(-n);
        }
        
        bool operator!=(const iterator& other) const {
            return index != other.index;
        }
        
        bool operator==(const iterator& other) const {
            return index == other.index;
        }
    };
    
    class const_iterator {
    private:
        const deque* deq;
        size_t current_block;
        size_t current_pos;
        size_t index;
        
    public:
        friend class deque;
        const_iterator() : deq(nullptr), current_block(0), current_pos(0), index(0) {}
        
        const_iterator(const deque* d, size_t idx) : deq(d), index(idx) {
            if (deq && deq->_size > 0) {
                if (idx == 0) {
                    current_block = deq->start_block;
                    current_pos = deq->start_pos;
                } else if (idx >= deq->_size) {
                    current_block = deq->end_block;
                    current_pos = deq->end_pos + 1;
                } else {
                    // Calculate position from index
                    size_t total_pos = deq->start_pos + idx;
                    current_block = deq->start_block + total_pos / BLOCK_SIZE;
                    current_pos = total_pos % BLOCK_SIZE;
                }
            }
        }
        
        const T& operator*() const {
            return deq->map[current_block][current_pos];
        }
        
        const T* operator->() const {
            return &deq->map[current_block][current_pos];
        }
        
        const_iterator& operator++() {
            if (current_pos == BLOCK_SIZE - 1) {
                current_block++;
                current_pos = 0;
            } else {
                current_pos++;
            }
            index++;
            return *this;
        }
        
        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        
        const_iterator& operator--() {
            if (current_pos == 0) {
                current_block--;
                current_pos = BLOCK_SIZE - 1;
            } else {
                current_pos--;
            }
            index--;
            return *this;
        }
        
        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --(*this);
            return tmp;
        }
        
        bool operator!=(const const_iterator& other) const {
            return index != other.index;
        }
        
        bool operator==(const const_iterator& other) const {
            return index == other.index;
        }
    };
    
    // Element access
    T& operator[](size_t pos) {
        if (pos >= _size) {
            throw out_of_range("deque index out of range");
        }
        
        size_t total_pos = start_pos + pos;
        size_t block = start_block + total_pos / BLOCK_SIZE;
        size_t block_pos = total_pos % BLOCK_SIZE;
        
        return map[block][block_pos];
    }
    
    const T& operator[](size_t pos) const {
        if (pos >= _size) {
            throw out_of_range("deque index out of range");
        }
        
        size_t total_pos = start_pos + pos;
        size_t block = start_block + total_pos / BLOCK_SIZE;
        size_t block_pos = total_pos % BLOCK_SIZE;
        
        return map[block][block_pos];
    }
    
    T& at(size_t pos) {
        return operator[](pos);
    }
    
    const T& at(size_t pos) const {
        return operator[](pos);
    }
    
    // Iterators
    iterator begin() {
        return iterator(this, 0);
    }
    
    iterator end() {
        return iterator(this, _size);
    }
    
    const_iterator begin() const {
        return const_iterator(this, 0);
    }
    
    const_iterator end() const {
        return const_iterator(this, _size);
    }
    
    const_iterator cbegin() const {
        return const_iterator(this, 0);
    }
    
    const_iterator cend() const {
        return const_iterator(this, _size);
    }
    
    // Modifiers
    iterator insert(iterator pos, const T& value) {
        size_t index = pos.index;
        if (index > _size) {
            throw out_of_range("insert position out of range");
        }
        
        if (index == _size) {
            push_back(value);
            return end() - 1;
        }
        
        if (index == 0) {
            push_front(value);
            return begin();
        }
        
        // For simplicity, move elements after position and insert
        deque temp;
        for (size_t i = 0; i < index; ++i) {
            temp.push_back(operator[](i));
        }
        temp.push_back(value);
        for (size_t i = index; i < _size; ++i) {
            temp.push_back(operator[](i));
        }
        
        *this = temp;
        return begin() + index;
    }
    
    iterator erase(iterator pos) {
        size_t index = pos.index;
        if (index >= _size) {
            throw out_of_range("erase position out of range");
        }
        
        deque temp;
        for (size_t i = 0; i < index; ++i) {
            temp.push_back(operator[](i));
        }
        for (size_t i = index + 1; i < _size; ++i) {
            temp.push_back(operator[](i));
        }
        
        *this = temp;
        return begin() + index;
    }
    
    void resize(size_t new_size) {
        if (new_size < _size) {
            while (_size > new_size) {
                pop_back();
            }
        } else if (new_size > _size) {
            while (_size < new_size) {
                push_back(T());
            }
        }
    }
    
    void resize(size_t new_size, const T& value) {
        if (new_size < _size) {
            while (_size > new_size) {
                pop_back();
            }
        } else if (new_size > _size) {
            while (_size < new_size) {
                push_back(value);
            }
        }
    }
    
    void swap(deque& other) {
        std::swap(map, other.map);
        std::swap(map_size, other.map_size);
        std::swap(start_block, other.start_block);
        std::swap(start_pos, other.start_pos);
        std::swap(end_block, other.end_block);
        std::swap(end_pos, other.end_pos);
        std::swap(_size, other._size);
    }
    
    void clear() {
        while (!empty()) {
            pop_front();
        }
    }
};

} // namespace sjtu

// Empty main function - the actual test code is provided by the judge system
int main() {
    return 0;
}
