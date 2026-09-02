#include <iostream>
#include <vector>
#include <algorithm>

template<typename T>
class Heap {
private:
    std::vector<T> heap;
    bool isMaxHeap; // true表示大顶堆，false表示小顶堆
    
    void heapifyUp(int index) {
        if (index == 0) return;
        
        int parent = (index - 1) / 2;
        bool condition = isMaxHeap ? (heap[index] > heap[parent]) : (heap[index] < heap[parent]);
        
        if (condition) {
            std::swap(heap[index], heap[parent]);
            heapifyUp(parent);
        }
    }
    
    void heapifyDown(int index) {
        int leftChild = 2 * index + 1;
        int rightChild = 2 * index + 2;
        int target = index;
        
        if (leftChild < heap.size()) {
            bool condition = isMaxHeap ? (heap[leftChild] > heap[target]) : (heap[leftChild] < heap[target]);
            if (condition) target = leftChild;
        }
        
        if (rightChild < heap.size()) {
            bool condition = isMaxHeap ? (heap[rightChild] > heap[target]) : (heap[rightChild] < heap[target]);
            if (condition) target = rightChild;
        }
        
        if (target != index) {
            std::swap(heap[index], heap[target]);
            heapifyDown(target);
        }
    }

public:
    // 构造函数，默认为大顶堆
    explicit Heap(bool maxHeap = true) : isMaxHeap(maxHeap) {}
    
    // 从数组构建堆
    Heap(const std::vector<T>& arr, bool maxHeap = true) : isMaxHeap(maxHeap) {
        heap = arr;
        buildHeap();
    }
    
    // 构建堆
    void buildHeap() {
        for (int i = heap.size() / 2 - 1; i >= 0; i--) {
            heapifyDown(i);
        }
    }
    
    // 插入元素
    void push(const T& value) {
        heap.push_back(value);
        heapifyUp(heap.size() - 1);
    }
    
    // 弹出堆顶元素
    T pop() {
        if (heap.empty()) {
            throw std::runtime_error("Heap is empty");
        }
        
        T top = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        
        if (!heap.empty()) {
            heapifyDown(0);
        }
        
        return top;
    }
    
    // 获取堆顶元素
    T top() const {
        if (heap.empty()) {
            throw std::runtime_error("Heap is empty");
        }
        return heap[0];
    }
    
    // 检查堆是否为空
    bool empty() const {
        return heap.empty();
    }
    
    // 获取堆大小
    size_t size() const {
        return heap.size();
    }
    
    // 清空堆
    void clear() {
        heap.clear();
    }
    
    // 打印堆（用于调试）
    void print() const {
        for (const auto& item : heap) {
            std::cout << item << " ";
        }
        std::cout << std::endl;
    }
    
    // 获取底层容器（只读）
    const std::vector<T>& getContainer() const {
        return heap;
    }
};

// 堆排序实现
template<typename T>
std::vector<T> heapSort(std::vector<T> arr, bool ascending = true) {
    if (arr.size() <= 1) return arr;
    
    // 创建大顶堆用于升序排序，小顶堆用于降序排序
    Heap<T> heap(!ascending); // ascending=true时用大顶堆，ascending=false时用小顶堆
    
    // 将所有元素插入堆中
    for (const auto& item : arr) {
        heap.push(item);
    }
    
    std::vector<T> result;
    result.reserve(arr.size());
    
    // 依次弹出堆顶元素
    while (!heap.empty()) {
        result.push_back(heap.pop());
    }
    
    return result;
}

// 另一种堆排序实现（原地排序）
template<typename T>
void heapSortInPlace(std::vector<T>& arr, bool ascending = true) {
    int n = arr.size();
    if (n <= 1) return;
    
    // 构建堆
    for (int i = n / 2 - 1; i >= 0; i--) {
        heapifyDown(arr, i, n, !ascending); // ascending=true时用大顶堆
    }
    
    // 逐个提取最大/最小元素
    for (int i = n - 1; i > 0; i--) {
        std::swap(arr[0], arr[i]); // 将堆顶移到末尾
        heapifyDown(arr, 0, i, !ascending); // 重新调整堆（不包括已排序部分）
    }
}

// 辅助函数：对单个数组执行下沉操作
template<typename T>
void heapifyDown(std::vector<T>& arr, int start, int end, bool isMaxHeap) {
    int parent = start;
    int child;
    
    while ((child = parent * 2 + 1) < end) {
        // 选择较大的子节点（对于大顶堆）或较小的子节点（对于小顶堆）
        if (child + 1 < end) {
            bool condition = isMaxHeap ? (arr[child] < arr[child + 1]) : (arr[child] > arr[child + 1]);
            if (condition) child++;
        }
        
        bool condition = isMaxHeap ? (arr[parent] < arr[child]) : (arr[parent] > arr[child]);
        if (condition) {
            std::swap(arr[parent], arr[child]);
            parent = child;
        } else {
            break;
        }
    }
}

// 测试代码
int main() {
    // 测试堆的基本功能
    std::cout << "=== 测试堆的基本功能 ===" << std::endl;
    Heap<int> maxHeap(true); // 大顶堆
    
    std::vector<int> testValues = {4, 1, 3, 2, 16, 9, 10, 14, 8, 7};
    std::cout << "插入元素: ";
    for (int val : testValues) {
        std::cout << val << " ";
        maxHeap.push(val);
    }
    std::cout << std::endl;
    
    std::cout << "堆顶元素: " << maxHeap.top() << std::endl;
    std::cout << "弹出元素序列: ";
    while (!maxHeap.empty()) {
        std::cout << maxHeap.pop() << " ";
    }
    std::cout << std::endl;
    
    // 测试堆排序
    std::cout << "\n=== 测试堆排序 ===" << std::endl;
    std::vector<int> unsorted = {64, 34, 25, 12, 22, 11, 90, 5};
    
    std::cout << "原始数组: ";
    for (int val : unsorted) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    // 升序排序
    auto sortedAsc = heapSort(unsorted, true);
    std::cout << "升序排序: ";
    for (int val : sortedAsc) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    // 降序排序
    auto sortedDesc = heapSort(unsorted, false);
    std::cout << "降序排序: ";
    for (int val : sortedDesc) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    // 测试原地排序
    std::cout << "\n=== 测试原地堆排序 ===" << std::endl;
    std::vector<int> unsorted2 = {64, 34, 25, 12, 22, 11, 90, 5};
    std::cout << "排序前: ";
    for (int val : unsorted2) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    heapSortInPlace(unsorted2, true);
    std::cout << "排序后: ";
    for (int val : unsorted2) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    // 测试字符串排序
    std::cout << "\n=== 测试字符串排序 ===" << std::endl;
    std::vector<std::string> words = {"banana", "apple", "cherry", "date", "elderberry"};
    std::cout << "原始: ";
    for (const auto& word : words) {
        std::cout << word << " ";
    }
    std::cout << std::endl;
    
    auto sortedWords = heapSort(words, true);
    std::cout << "排序后: ";
    for (const auto& word : sortedWords) {
        std::cout << word << " ";
    }
    std::cout << std::endl;
    
    return 0;
}