#pragma once

#include <string>

struct IntHash {
    unsigned long operator()(int key) const {
        return static_cast<unsigned long>(static_cast<unsigned int>(key));
    }
};

struct StringHash {
    unsigned long operator()(const std::string& key) const {
        unsigned long hash = 2166136261UL;
        for (std::string::size_type i = 0; i < key.size(); ++i) {
            hash ^= static_cast<unsigned char>(key[i]);
            hash *= 16777619UL;
        }
        return hash;
    }
};

template <typename K, typename V, typename HashFunc>
class HashMap {
private:
    struct Entry {
        K key;
        V value;
        Entry* next;

        Entry(const K& entryKey, const V& entryValue)
            : key(entryKey), value(entryValue), next(nullptr) {}
    };

    Entry** buckets;
    int capacity;
    int size;
    HashFunc hash;

    int bucketFor(const K& key, int bucketCount) const {
        return static_cast<int>(hash(key) % static_cast<unsigned long>(bucketCount));
    }

    void rehash(int newCapacity) {
        Entry** newBuckets = new Entry*[newCapacity];
        for (int i = 0; i < newCapacity; ++i) {
            newBuckets[i] = nullptr;
        }

        for (int i = 0; i < capacity; ++i) {
            Entry* entry = buckets[i];
            while (entry) {
                Entry* next = entry->next;
                const int index = bucketFor(entry->key, newCapacity);
                entry->next = newBuckets[index];
                newBuckets[index] = entry;
                entry = next;
            }
        }

        delete[] buckets;
        buckets = newBuckets;
        capacity = newCapacity;
    }

public:
    explicit HashMap(int initialCapacity = 16, const HashFunc& hashFunc = HashFunc())
        : buckets(nullptr), capacity(initialCapacity < 4 ? 4 : initialCapacity),
          size(0), hash(hashFunc) {
        buckets = new Entry*[capacity];
        for (int i = 0; i < capacity; ++i) {
            buckets[i] = nullptr;
        }
    }

    ~HashMap() {
        clear();
        delete[] buckets;
    }

    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

    void clear() {
        for (int i = 0; i < capacity; ++i) {
            Entry* entry = buckets[i];
            while (entry) {
                Entry* next = entry->next;
                delete entry;
                entry = next;
            }
            buckets[i] = nullptr;
        }
        size = 0;
    }

    bool insert(const K& key, const V& value) {
        Entry* existing = findEntry(key);
        if (existing) {
            existing->value = value;
            return false;
        }

        if ((size + 1) * 4 > capacity * 3) {
            rehash(capacity * 2);
        }

        const int index = bucketFor(key, capacity);
        Entry* entry = new Entry(key, value);
        entry->next = buckets[index];
        buckets[index] = entry;
        ++size;
        return true;
    }

    V* find(const K& key) {
        Entry* entry = findEntry(key);
        return entry ? &entry->value : nullptr;
    }

    const V* find(const K& key) const {
        const Entry* entry = findEntry(key);
        return entry ? &entry->value : nullptr;
    }

    bool remove(const K& key) {
        const int index = bucketFor(key, capacity);
        Entry* previous = nullptr;
        Entry* entry = buckets[index];
        while (entry) {
            if (entry->key == key) {
                if (previous) {
                    previous->next = entry->next;
                } else {
                    buckets[index] = entry->next;
                }
                delete entry;
                --size;
                return true;
            }
            previous = entry;
            entry = entry->next;
        }
        return false;
    }

    int getSize() const {
        return size;
    }

private:
    Entry* findEntry(const K& key) {
        Entry* entry = buckets[bucketFor(key, capacity)];
        while (entry) {
            if (entry->key == key) {
                return entry;
            }
            entry = entry->next;
        }
        return nullptr;
    }

    const Entry* findEntry(const K& key) const {
        const Entry* entry = buckets[bucketFor(key, capacity)];
        while (entry) {
            if (entry->key == key) {
                return entry;
            }
            entry = entry->next;
        }
        return nullptr;
    }
};
