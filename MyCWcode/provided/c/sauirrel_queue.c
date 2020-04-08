#include <stdio.h>
#include <stdlib.h>
#include "sauirrel_queue.h"




/**
 * Operation result: construct an empty queue Q
 */
Status InitQueue(LinkQueue *Q) {
    if (!(Q->front = Q->rear = (QueuePtr)malloc(sizeof(struct QNode)))) {
        exit(OVERFLOW);
    }
    Q->front->next = NULL;
    return OK;
}


/**
 * Initial condition: queue Q exists
 * Operation result: the queue Q is destroyed
 */
Status DestroyQueue(LinkQueue *Q) {
    while (Q->front) {  
        Q->rear = Q->front->next;  
        free(Q->front); 
        Q->front = Q->rear;  
    }
    return OK;
}

/**
 * Initial condition: queue Q exists
 * Operation result: Clear queue Q
 */
Status ClearQueue(LinkQueue *Q) {
    QueuePtr p, q;
    Q->rear = Q->front; 
    p = Q->front->next;  
    Q->front->next = NULL; 
    while (p) {
        q = p->next;
        free(p);
        p = q;
    }
    return OK;

}

/**
 * Initial condition: queue Q exists
 * Operation result: true if Q is an empty queue, false otherwise
 */
Status QueueEmpty(LinkQueue Q) {
    if (Q.front->next == NULL) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * Initial condition: queue Q exists
 * Operation result: return the number of elements in Q, namely the queue length
 */
int QueueLength(LinkQueue Q) {
    int i = 0; 
    QueuePtr p = Q.front->next; 
    while (p) {
        i++;
        p = p->next;
    }
    return i;
}

/**
 * Initial condition: queue Q exists and is not empty
 * Operation result: return Q squadron element
 */
Status GetHead(LinkQueue Q, QElemType *e) {
    if (Q.front != Q.rear) {
        *e = Q.front->next->data;
        return OK;
    }
    return INFEASIBLE;
}
/**
 * Initial condition: queue Q exists and is not empty
 * Operation result: delete the head element of Q and return with e
 */
Status DeQueue(LinkQueue *Q, QElemType *e) {
    QueuePtr p;
    if (Q->front == Q->rear) { 
        return ERROR;
    }
    p = Q->front->next; 
    *e = p->data;
    Q->front->next = p->next; 
    if (Q->rear == p) {  
        Q->rear = Q->front;  
    }
    free(p);
    return OK;
}

/**
 * Initial condition: queue Q exists
 * Operation result: insert the new element tail element with element e as Q
 */
Status EnQueue(LinkQueue *Q, QElemType e) {
    LinkQueue tQ = *Q;
    int len = QueueLength(tQ);
    if (len == MAXSIZE) {
        QElemType heade;
        GetHead(tQ, &heade);
        DeQueue(Q, &heade);
    }
    QueuePtr p;
    p = (QueuePtr)malloc(sizeof(struct QNode)); 
    p->data = e;
    p->next = NULL;
    Q->rear->next = p; 
    Q->rear = p;  
    return OK;
}



/**
 * Initial condition: queue Q exists and is not empty
 * Operation result: traverse each element in the queue from the beginning to the end
 */
void QueueTraverse(LinkQueue Q) {
    QueuePtr p = Q.front->next; 
    while (p) {
        printf("%d ", p->data);  
        p = p->next;
    }
    printf("\n");
}

int QueueCount(LinkQueue Q) {
    QueuePtr p = Q.front->next;
    int count = 0;
    while (p) {
        count += p->data;
        p = p->next;
    }
    return count;
}

