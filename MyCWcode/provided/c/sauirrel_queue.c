#include <stdio.h>
#include <stdlib.h>
#include "sauirrel_queue.h"




/**
 * 操作结果：构造一个空队列 Q
 * @param Q
 * @return
 */
Status InitQueue(LinkQueue *Q) {
    if (!(Q->front = Q->rear = (QueuePtr)malloc(sizeof(struct QNode)))) {
        exit(OVERFLOW);
    }
    Q->front->next = NULL;
    printf("200 OK\n");
    return OK;
}

/**
 * 遍历函数
 * @param e
 */
//void vi(QElemType e) {
//    printf("%d ",e);
//}

/**
 * 初始条件：队列 Q 存在
 * 操作结果：队列 Q 被销毁
 * @param Q
 * @return
 */
Status DestroyQueue(LinkQueue *Q) {
    while (Q->front) {  //指向队尾时结束循环
        Q->rear = Q->front->next;  // 队尾指针指向队头指针的下一个结点
        free(Q->front);  //释放队头结点
        Q->front = Q->rear;  //修改队头指针
    }
    return OK;
}

/**
 * 初始条件：队列 Q 存在
 * 操作结果：清空队列 Q
 * @param Q
 * @return
 */
Status ClearQueue(LinkQueue *Q) {
    QueuePtr p, q;
    Q->rear = Q->front;  //队尾指针指向头结点
    p = Q->front->next;  //p 指向队列第一个元素
    Q->front->next = NULL;  //队头指针 next 域置空
    while (p) {
        q = p->next;
        free(p);
        p = q;
    }
    return OK;

}

/**
 * 初始条件：队列 Q 存在
 * 操作结果：若 Q 为空队列，则返回 true，否则返回 false
 * @param Q
 * @return
 */
Status QueueEmpty(LinkQueue Q) {
    if (Q.front->next == NULL) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * 初始条件：队列 Q 存在
 * 操作结果：返回 Q 中元素个数，即队列长度
 * @param Q
 * @return
 */
int QueueLength(LinkQueue Q) {
    int i = 0;  //计数器
    QueuePtr p = Q.front->next;  //p 指向队头元素
    while (p) {
        i++;
        p = p->next;
    }
    return i;
}

/**
 * 初始条件：队列 Q 存在且非空
 * 操作结果：返回 Q 中队头元素
 * @param Q
 * @param e
 * @return
 */
Status GetHead(LinkQueue Q, QElemType *e) {
    if (Q.front != Q.rear) {
        *e = Q.front->next->data;
        return OK;
    }
    return INFEASIBLE;
}
/**
 * 初始条件：队列 Q 存在且非空
 * 操作结果：删除 Q 的队头元素，且用 e 返回
 * @param Q
 * @param e
 * @return
 */
Status DeQueue(LinkQueue *Q, QElemType *e) {
    QueuePtr p;
    if (Q->front == Q->rear) {  //判断是否为空队列
        return ERROR;
    }
    p = Q->front->next;  //p 指向队头元素
    *e = p->data;
    Q->front->next = p->next;  //修改头指针
    if (Q->rear == p) {  //如果删除的是队列最后一个元素
        Q->rear = Q->front;  //队尾指针指向头结点
    }
    free(p);
    return OK;
}

/**
 * 初始条件：队列 Q 存在
 * 操作结果：插入入元素 e 为 Q 的新队尾元素
 * @param Q
 * @param e
 * @return
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
    p = (QueuePtr)malloc(sizeof(struct QNode));  //开辟新结点
    p->data = e;
    p->next = NULL;
    Q->rear->next = p;  //将新结点插入到队尾
    Q->rear = p;  //修改队尾指针
    return OK;
}



/**
 * 初始条件：队列 Q 存在且非空
 * 操作结果：从队头到队尾，依次对遍历队列中每个元素
 * @param Q
 * @param vi
 */
void QueueTraverse(LinkQueue Q) {
    QueuePtr p = Q.front->next;  //p 指向队头元素
    while (p) {
        printf("%d ", p->data);  //遍历
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

//int main() {
//    LinkQueue q;
//    QElemType e;
//
//    InitQueue(&q);
//    printf("队列的长度：%d\n", QueueLength(q));
//    printf("队列是否为空：%d\n", QueueEmpty(q));
//    EnQueue(&q, 3);
//    EnQueue(&q, 4);
//    EnQueue(&q, 5);
//    QueueTraverse(q, vi);
//    printf("Count = %d\n", QueueCount(q));
//    EnQueue(&q, 6);
//    QueueTraverse(q, vi);
//    EnQueue(&q, 7);
//    printf("Count = %d\n", QueueCount(q));
//    QueueTraverse(q, vi);
//    EnQueue(&q, 8);
//    printf("Count = %d\n", QueueCount(q));
//    QueueTraverse(q, vi);
//    EnQueue(&q, 9);
//    QueueTraverse(q, vi);
//    printf("队列的长度：%d\n", QueueLength(q));
//    printf("队列是否为空：%d\n", QueueEmpty(q));
//    GetHead(q, &e);
//    printf("队列的头元素：%d\n", e);
//    DeQueue(&q, &e);
//    QueueTraverse(q, vi);
//    ClearQueue(&q);
//    printf("队列的长度：%d\n", QueueLength(q));
//    printf("队列是否为空：%d\n", QueueEmpty(q));
//
//    DestroyQueue(&q);
//}
