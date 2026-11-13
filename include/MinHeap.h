#pragma once

#include <stdexcept>

template <typename T>
struct MinHeapLess {
    bool operator()(const T& left, const T& right) const {
        return left < right;
    }
};

template <typename T, typename Comparator = MinHeapLess<T>>
class MinHeap {
private:
    T* items;
    int itemCount;
    int capacity;
    Comparator comesBefore;

    void grow() {
        const int newCapacity = capacity == 0 ? 8 : capacity * 2;
        T* expanded = new T[newCapacity];
        for (int i = 0; i < itemCount; ++i) {
            expanded[i] = items[i];
        }
        delete[] items;
        items = expanded;
        capacity = newCapacity;
    }

public:
    explicit MinHeap(const Comparator& comparator = Comparator())
        : items(nullptr), itemCount(0), capacity(0), comesBefore(comparator) {}

    ~MinHeap() {
        clear();
    }

    MinHeap(const MinHeap&) = delete;
    MinHeap& operator=(const MinHeap&) = delete;

    void clear() {
        delete[] items;
        items = nullptr;
        itemCount = 0;
        capacity = 0;
    }

    bool isEmpty() const {
        return itemCount == 0;
    }

    int size() const {
        return itemCount;
    }

    const T& top() const {
        if (isEmpty()) {
            throw std::out_of_range("MinHeap::top called on an empty heap");
        }
        return items[0];
    }

    void push(const T& value) {
        if (itemCount == capacity) {
            grow();
        }

        int index = itemCount++;
        items[index] = value;
        while (index > 0) {
            const int parent = (index - 1) / 2;
            if (!comesBefore(items[index], items[parent])) {
                break;
            }
            T temp = items[index];
            items[index] = items[parent];
            items[parent] = temp;
            index = parent;
        }
    }

    void pop() {
        if (isEmpty()) {
            return;
        }

        --itemCount;
        if (itemCount == 0) {
            return;
        }

        items[0] = items[itemCount];
        int index = 0;
        while (true) {
            const int left = index * 2 + 1;
            const int right = left + 1;
            int best = index;

            if (left < itemCount && comesBefore(items[left], items[best])) {
                best = left;
            }
            if (right < itemCount && comesBefore(items[right], items[best])) {
                best = right;
            }
            if (best == index) {
                break;
            }

            T temp = items[index];
            items[index] = items[best];
            items[best] = temp;
            index = best;
        }
    }
};
