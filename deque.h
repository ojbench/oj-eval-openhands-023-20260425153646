#ifndef DEQUE_H
#define DEQUE_H

#include <iostream>
#include <stdexcept>
#include <algorithm>

template <typename T>
class Deque {
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
                first_used = std::min(first_used, i);
                last_used = std::max(last_used, i);
            }
        }
        
        size_t used_range = last_used - first_used + 1;
        if (used_range * 4 >= map_size) return; // Don't shrink if still using >25% of map
        
        size_t new_map_size = std::max(8UL, used_range * 2);
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
    Deque() : map(nullptr), map_size(0), start_block(0), start_pos(0), 
              end_block(0), end_pos(0), _size(0) {
        allocate_map(8);
        start_block = end_block = map_size / 2;
        start_pos = end_pos = BLOCK_SIZE / 2;
    }
    
    ~Deque() {
        clear();
        if (map) {
            deallocate_all();
        }
    }
    
    Deque(const Deque& other) : map(nullptr), map_size(0), start_block(0), start_pos(0),
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
    
    Deque& operator=(const Deque& other) {
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
            throw std::out_of_range("Deque is empty");
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
            throw std::out_of_range("Deque is empty");
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
            throw std::out_of_range("Deque is empty");
        }
        return map[start_block][start_pos];
    }
    
    const T& front() const {
        if (_size == 0) {
            throw std::out_of_range("Deque is empty");
        }
        return map[start_block][start_pos];
    }
    
    T& back() {
        if (_size == 0) {
            throw std::out_of_range("Deque is empty");
        }
        return map[end_block][end_pos];
    }
    
    const T& back() const {
        if (_size == 0) {
            throw std::out_of_range("Deque is empty");
        }
        return map[end_block][end_pos];
    }
    
    bool empty() const {
        return _size == 0;
    }
    
    size_t size() const {
        return _size;
    }
    
    void clear() {
        while (!empty()) {
            pop_front();
        }
    }
    
    // Iterator class for basic iteration
    class iterator {
    private:
        Deque* deque;
        size_t current_block;
        size_t current_pos;
        size_t index;
        
    public:
        iterator(Deque* d, size_t idx) : deque(d), index(idx) {
            if (deque && deque->_size > 0) {
                if (idx == 0) {
                    current_block = deque->start_block;
                    current_pos = deque->start_pos;
                } else if (idx >= deque->_size) {
                    current_block = deque->end_block;
                    current_pos = deque->end_pos + 1;
                } else {
                    // Calculate position from index
                    size_t total_pos = deque->start_pos + idx;
                    current_block = deque->start_block + total_pos / BLOCK_SIZE;
                    current_pos = total_pos % BLOCK_SIZE;
                }
            }
        }
        
        T& operator*() {
            return deque->map[current_block][current_pos];
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
        
        bool operator!=(const iterator& other) const {
            return index != other.index;
        }
    };
    
    iterator begin() {
        return iterator(this, 0);
    }
    
    iterator end() {
        return iterator(this, _size);
    }
};

#endif