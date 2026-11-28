#ifndef EMERGENCY_LANDING_H
#define EMERGENCY_LANDING_H

#include "flight.h"
#include <iostream>
#include <string>
using namespace std;

// Person 1 stub: normal FIFO queue + min-heap for emergencies
class emergency_landing {
private:
    flight* normal_front;
    flight* normal_rear;
    flight** heap_array;
    int heap_size;
    int heap_capacity;

    bool is_higher_priority(flight* a, flight* b) {
        if (a->priority_level == b->priority_level)
            return a->arrival_tick < b->arrival_tick;
        return a->priority_level < b->priority_level;
    }
    // Grow heap capacity when full
    void grow_heap() {
        int new_capacity = heap_capacity * 2;
        if (new_capacity < 4) {
            new_capacity = 4;
        }
        flight** new_array = new flight*[new_capacity];
        int i = 0;
        while (i < heap_size) {
            new_array[i] = heap_array[i];
            i = i + 1;
        }
        delete[] heap_array;
        heap_array = new_array;
        heap_capacity = new_capacity;
    }

    // Bubble last element up by priority_level
    void heapify_up(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (is_higher_priority(heap_array[index], heap_array[parent])) {
                flight* temp = heap_array[index];
                heap_array[index] = heap_array[parent];
                heap_array[parent] = temp;
                index = parent;
            }
            else {
                break;
            }
        }
    }

    // Bubble root down by priority_level
    void heapify_down(int index) {
        while (true) {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int smallest = index;
            if (left < heap_size &&
                is_higher_priority(heap_array[left], heap_array[smallest])) {
                smallest = left;
            }
            if (right < heap_size &&
                is_higher_priority(heap_array[right], heap_array[smallest])) {
                smallest = right;
            }
            if (smallest == index)
                break;
            flight* temp = heap_array[index];
            heap_array[index] = heap_array[smallest];
            heap_array[smallest] = temp;
            index = smallest;
        }
    }

public:
    emergency_landing() {
        normal_front = 0;
        normal_rear = 0;
        heap_capacity = 8;
        heap_size = 0;
        heap_array = new flight*[heap_capacity];
        int i = 0;
        while (i < heap_capacity) {
            heap_array[i] = 0;
            i = i + 1;
        }
    }

    // Free queue and heap flight nodes
    ~emergency_landing() {
        flight* current = normal_front;
        while (current != 0) {
            flight* temp = current;
            current = current->next;
            delete temp;
        }
        int i = 0;
        while (i < heap_size) {
            delete heap_array[i];
            i = i + 1;
        }
        delete[] heap_array;
        heap_array = 0;
        normal_front = 0;
        normal_rear = 0;
    }

    // Enqueue a COPY into the normal FIFO queue
    void enqueue_normal(flight* f) {
        if (f == 0) {
            return;
        }
        flight* node = new flight;
        *node = *f;
        node->next = 0;
        if (normal_rear == 0) {
            normal_front = node;
            normal_rear = node;
        } else {
            normal_rear->next = node;
            normal_rear = node;
        }
    }

    // Dequeue from normal queue; caller owns returned pointer
    flight* dequeue_normal() {
        if (normal_front == 0) {
            return 0;
        }
        flight* node = normal_front;
        normal_front = normal_front->next;
        if (normal_front == 0) {
            normal_rear = 0;
        }
        node->next = 0;
        return node;
    }

    // Peek front of normal queue without removing
    flight* front_normal() {
        return normal_front;
    }

    // True if normal queue has no flights
    bool is_empty_normal() {
        return normal_front == 0;
    }

    // Insert a COPY into the emergency min-heap
    void insert_emergency(flight* f) {
        if (f == 0) {
            return;
        }
        if (heap_size >= heap_capacity) {
            grow_heap();
        }
        flight* node = new flight;
        *node = *f;
        node->next = 0;
        heap_array[heap_size] = node;
        heapify_up(heap_size);
        heap_size = heap_size + 1;
    }

    // Extract highest-priority (lowest number) emergency; caller owns pointer
    flight* extract_min() {
        if (heap_size == 0) {
            return 0;
        }
        flight* result = heap_array[0];
        heap_size = heap_size - 1;
        heap_array[0] = heap_array[heap_size];
        heap_array[heap_size] = 0;
        if (heap_size > 0) {
            heapify_down(0);
        }
        return result;
    }

    // Peek min without removing
    flight* peek_min() {
        if (heap_size == 0) {
            return 0;
        }
        return heap_array[0];
    }

    // True if emergency heap is empty
    bool is_empty_heap() {
        return heap_size == 0;
    }

    // Process next landing: NORMAL queue first if non-empty, else emergency heap.
    // Heap order is priority_level first, then earlier arrival_tick.
    flight* process_next_landing() {
        if (!is_empty_normal()) {
            return dequeue_normal();
        }
        if (!is_empty_heap()) {
            return extract_min();
        }
        return 0;
    }

    // Print normal queue
    void display_normal_queue() {
        cout << "=== Normal Landing Queue ===" << endl;
        flight* current = normal_front;
        if (current == 0) {
            cout << "Queue empty." << endl;
            return;
        }
        while (current != 0) {
            cout << current->callsign << " | " << current->status << endl;
            current = current->next;
        }
    }

    // Print emergency heap contents
    void display_heap() {
        cout << "=== Emergency Landing Heap ===" << endl;
        if (heap_size == 0) {
            cout << "Heap empty." << endl;
            return;
        }
        int i = 0;
        while (i < heap_size) {
            cout << heap_array[i]->callsign
                 << " | priority=" << heap_array[i]->priority_level << endl;
            i = i + 1;
        }
    }
};

#endif
