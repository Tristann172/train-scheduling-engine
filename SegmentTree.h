#pragma once

template <typename T>
class SegmentTree {
private:
    T* minimums;
    T* lazyValues;
    int segmentCount;

    void build(int node, int left, int right, const T& initialValue) {
        minimums[node] = initialValue;
        lazyValues[node] = T();
        if (left == right) {
            return;
        }
        const int middle = left + (right - left) / 2;
        build(node * 2, left, middle, initialValue);
        build(node * 2 + 1, middle + 1, right, initialValue);
    }

    void apply(int node, const T& value) {
        minimums[node] += value;
        lazyValues[node] += value;
    }

    void pushDown(int node) {
        if (lazyValues[node] == T()) {
            return;
        }
        apply(node * 2, lazyValues[node]);
        apply(node * 2 + 1, lazyValues[node]);
        lazyValues[node] = T();
    }

    void update(int node, int left, int right, int queryLeft, int queryRight,
                const T& value) {
        if (queryLeft <= left && right <= queryRight) {
            apply(node, value);
            return;
        }

        pushDown(node);
        const int middle = left + (right - left) / 2;
        if (queryLeft <= middle) {
            update(node * 2, left, middle, queryLeft, queryRight, value);
        }
        if (queryRight > middle) {
            update(node * 2 + 1, middle + 1, right, queryLeft, queryRight, value);
        }
        minimums[node] = minimums[node * 2] < minimums[node * 2 + 1]
            ? minimums[node * 2] : minimums[node * 2 + 1];
    }

    T query(int node, int left, int right, int queryLeft, int queryRight) {
        if (queryLeft <= left && right <= queryRight) {
            return minimums[node];
        }

        pushDown(node);
        const int middle = left + (right - left) / 2;
        bool hasResult = false;
        T result = T();

        if (queryLeft <= middle) {
            result = query(node * 2, left, middle, queryLeft, queryRight);
            hasResult = true;
        }
        if (queryRight > middle) {
            const T rightResult = query(node * 2 + 1, middle + 1, right,
                                        queryLeft, queryRight);
            if (!hasResult || rightResult < result) {
                result = rightResult;
            }
        }
        return result;
    }

public:
    SegmentTree() : minimums(nullptr), lazyValues(nullptr), segmentCount(0) {}

    explicit SegmentTree(int count, const T& initialValue = T())
        : minimums(nullptr), lazyValues(nullptr), segmentCount(0) {
        initialize(count, initialValue);
    }

    ~SegmentTree() {
        clear();
    }

    SegmentTree(const SegmentTree&) = delete;
    SegmentTree& operator=(const SegmentTree&) = delete;

    void clear() {
        delete[] minimums;
        delete[] lazyValues;
        minimums = nullptr;
        lazyValues = nullptr;
        segmentCount = 0;
    }

    void initialize(int count, const T& initialValue = T()) {
        clear();
        if (count <= 0) {
            return;
        }
        segmentCount = count;
        minimums = new T[4 * count];
        lazyValues = new T[4 * count];
        build(1, 0, count - 1, initialValue);
    }

    int size() const {
        return segmentCount;
    }

    T queryMin(int left, int right) {
        if (segmentCount == 0 || left < 0 || right >= segmentCount || left > right) {
            return T();
        }
        return query(1, 0, segmentCount - 1, left, right);
    }

    void updateRange(int left, int right, const T& value) {
        if (segmentCount == 0 || left < 0 || right >= segmentCount || left > right) {
            return;
        }
        update(1, 0, segmentCount - 1, left, right, value);
    }
};
