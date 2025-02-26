#### *Task done as the third assignment in the Concurrent Programming course at MIMUW.*

The task is to implement multiple versions of a non-blocking queue with multiple readers and writers: **SimpleQueue, RingsQueue, LLQueue, BLQueue**. Two implementations rely on standard mutexes, while the other two use atomic operations, including the key **compare_exchange** operation.  

Each implementation consists of a **queue structure** and the following methods:  

- `<queue>* <queue>_new(void)` – allocates (`malloc`) and initializes a new queue.  
- `void <queue>_delete(<queue>* queue)` – frees all memory allocated by the queue methods.  
- `void <queue>_push(<queue>* queue, Value value)` – adds a value to the end of the queue.  
- `Value <queue>_pop(<queue>* queue)` – retrieves a value from the front of the queue or returns `EMPTY_VALUE` if the queue is empty.  
- `bool <queue>_is_empty(<queue>* queue)` – checks whether the queue is empty.  

(For example, the first implementation should define the structure **SimpleQueue** and methods such as `SimpleQueue* SimpleQueue_new(void)`, etc.)  

---

### Additional Rules  

- Queue users do not use the values `EMPTY_VALUE=0` or `TAKEN_VALUE=-1` (these can be used as special markers).  
- Queue users guarantee that they execute `new/delete` exactly **once**, before/after all `push/pop/is_empty` operations across all threads.  

---

## Queue Implementations  

### **SimpleQueue: Single-linked list queue with two mutexes**  
This is one of the simpler queue implementations. A separate mutex for producers and consumers allows for better parallelization of operations.  

### **RingsQueue: Queue based on a list of cyclic buffers**  
This combines **SimpleQueue** with a queue implemented on a **circular buffer**, merging the **unbounded size** of the first with the **efficiency** of the second (since singly linked lists are relatively slow due to constant memory allocations).  

### **LLQueue: Lock-free queue using a singly linked list**  
This is one of the simplest **lock-free queue** implementations.  

### **BLQueue: Lock-free queue with buffer lists**  
This is a **simple yet highly efficient lock-free queue** implementation.  
The idea behind this queue is to **combine a singly linked list with an array-based queue** that uses **atomic indices** for inserting and retrieving elements (though the number of operations would be limited by the array length).  
To **combine the advantages of both**, the queue consists of a **list of arrays**; a new list node is created **only when the current array is full**. However, the array **is not** a circular buffer—each field in it is filled **at most once** (a circular buffer variant would be significantly harder to implement).  

---

## **Hazard Pointer**  

A **Hazard Pointer** is a technique for safely **freeing memory** in **data structures shared across multiple threads** and dealing with the **ABA problem**.  

The idea is that each thread can **reserve** a memory address for a node (one per queue instance) that it needs to protect from deletion (or ABA replacement) during **push/pop/is_empty** operations.  

Instead of calling `free()` to delete a node, a thread **adds its address to a retired list** and later **periodically checks** this list, freeing addresses that are **no longer reserved**.
