#ifdef __cplusplus
extern "C" {
#endif

#ifndef UNTITLED_SAUIRREL_QUEUE_H
#define UNTITLED_SAUIRREL_QUEUE_H

#define TRUE 1
#define FALSE 0
#define OK 1
#define ERROR 0
#define OVERFLOW -1
#define INFEASIBLE 0
#define MAXSIZE 5

typedef int Status;
typedef int QElemType;

typedef struct QNode {
    QElemType data;
    struct QNode *next;
}QNode, *QueuePtr;

typedef struct {
    QueuePtr front; 
    QueuePtr rear;  
}LinkQueue;

// Init am empty queue
Status InitQueue(LinkQueue *);
// Delete queue
Status DestroyQueue(LinkQueue *);
// Clear queue
Status ClearQueue(LinkQueue *);
// Whether the queue is empty
Status QueueEmpty(LinkQueue );
// Get the length of the queue
int QueueLength(LinkQueue);
// Get the head of queue
Status GetHead(LinkQueue, QElemType *);
// Insert an element to the end of queue
Status EnQueue(LinkQueue *, QElemType);
// Delete the first element of queue
Status DeQueue(LinkQueue *, QElemType *);
// Get all elements of queue
void QueueTraverse(LinkQueue);
// Get the sum of queue
int QueueCount(LinkQueue);
#endif //UNTITLED_SAUIRREL_QUEUE_H

#ifdef __cplusplus
}
#endif