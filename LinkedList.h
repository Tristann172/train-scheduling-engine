#pragma once

template <typename T>
struct Node {
    T data;
    Node<T>* next;

    explicit Node(const T& value) : data(value), next(nullptr) {}
};

template <typename T>
class LinkedList {
public:
    typedef ::Node<T> Node;

private:
    Node* head;
    Node* tail;
    int size;

public:
    LinkedList() : head(nullptr), tail(nullptr), size(0) {}

    ~LinkedList() {
        clear();
    }

    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;

    void pushBack(const T& value) {
        Node* node = new Node(value);
        if (!tail) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        ++size;
    }

    void clear() {
        while (head) {
            Node* node = head;
            head = head->next;
            delete node;
        }
        tail = nullptr;
        size = 0;
    }

    int getSize() const {
        return size;
    }

    Node* getHead() const {
        return head;
    }

    Node* getTail() const {
        return tail;
    }

    bool removeNode(Node* target) {
        if (!target) {
            return false;
        }
        Node* previous = nullptr;
        Node* current = head;
        while (current && current != target) {
            previous = current;
            current = current->next;
        }
        if (!current) {
            return false;
        }
        if (previous) {
            previous->next = current->next;
        } else {
            head = current->next;
        }
        if (tail == current) {
            tail = previous;
        }
        delete current;
        --size;
        return true;
    }
};
