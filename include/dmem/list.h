/* vim: set et sts=4 ts=4 sw=4: */

#ifndef DMEM_LIST_H
#define DMEM_LIST_H

#include "common.h"

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

/* This is a doubly linked circular list.
 *
 * - The list can optionally hold onto the next value when iterating, so that
 *   you can remove items from the list whilst iterating. Iterating in this
 *   manner however is non-reentrant.
 * - The list must be cleared before using.
 *
 * To iterate over the list:
 */
#if 0
typedef struct SomeType SomeType;
struct SomeType {
    d_ListNode(SomeType)   hl;
};

d_List(SomeType) list;
dl_clear(SomeType, hl, &list);
dl_append(hl, &list, calloc(1, sizeof(SomeType)));

/* Storing the iterator in the list */
SomeType* i;

for (i = dl_begin(&list); i != dl_end(&list); i = dl_getiter(&list)) {
    dl_setiter(hl, &list, i->hl.next);
    dowork(i);
    dl_remove(hl, i);
    free(i);
}

/* Iterating in a standard manner */
for (i = dl_begin(&list); i != dl_end(&list); i = i->hl.next) {
    dowork(i);
}

#endif




/* Define the list structs
 * @type: the type of node held in the list
 */
#define DLIST_INIT(type)                                                      \
    typedef struct d_List_##type {                                            \
        type*   next;                                                         \
        type*   prev;                                                         \
        type*   iter;                                                         \
        type*   fake_node;                                                    \
    } d_List_##type;                                                          \
                                                                              \
    typedef struct d_ListNode_##type {                                        \
        type*           next;                                                 \
        type*           prev;                                                 \
        d_List_##type*  list;                                                 \
    } d_ListNode_##type                                                       \

#ifdef __cplusplus
template <class T>
struct d_List {
    T* next;
    T* prev;
    T* iter;
    T* fake_node;
};

template <class T>
struct d_ListNode {
    d_ListNode() : next(NULL), prev(NULL), list(NULL) {}
    T* next;
    T* prev;
    d_List<T>* list;
};
#endif
 
/* List type
 * @type: the type of node held in the list
 */
#define d_List(type) d_List_##type

/* List header type - used as a member in each node
 * @type: the type of node held in the list
 */
#define d_ListNode(type) d_ListNode_##type


/* Removes a node from the list
 * @header: the d_ListHeader(type) member in @type
 * @v: pointer to the node to remove from the list
 */
#define dl_remove(header, v)                                                  \
    do {                                                                      \
        if ((v)->header.list) {                                               \
            (v)->header.next->header.prev = (v)->header.prev;                 \
            (v)->header.prev->header.next = (v)->header.next;                 \
                                                                              \
            if ((v)->header.list->iter == (v)) {                              \
                (v)->header.list->iter = (v)->header.next;                    \
            }                                                                 \
        }                                                                     \
                                                                              \
        (v)->header.next = NULL;                                              \
        (v)->header.prev = NULL;                                              \
        (v)->header.list = NULL;                                              \
    } while(0)                                                                \
 
/* Clears and initializes a list
 * @type: the type held in the list
 * @header: the d_ListHeader(type) member in @type
 * @plist: pointer to a d_List(type) which holds the list
 */
#define dl_clear(type, header, plist)                                         \
    do {                                                                      \
        (plist)->iter = NULL;                                                 \
        (plist)->prev = container_of(plist, type, header);                    \
        (plist)->next = container_of(plist, type, header);                    \
        (plist)->fake_node = container_of(plist, type, header);               \
        assert(&(plist)->next == &(plist)->fake_node->header.next);           \
    } while(0)                                                                \
 
/* Adds a node to the beginning of the list
 * @header: the d_ListHeader(type) member in @type
 * @plist: pointer to a d_List(type) which holds the list
 * @v: pointer to the node to prepend to the list
 */
#define dl_prepend(header, plist, v)                                          \
    do {                                                                      \
        (v)->header.list = plist;                                             \
        (v)->header.prev = (plist)->fake_node;                                \
        (v)->header.next = (plist)->fake_node;                                \
        (v)->header.prev->header.next = v;                                    \
        (v)->header.next->header.prev = v;                                    \
    } while(0)                                                                \
 
/* Adds a node to the end of the list
 * @header: the d_ListHeader(type) member in @type
 * @plist: pointer to a d_List(type) which holds the list
 * @v: pointer to the node to append to the list
 */
#define dl_append(header, plist, v)                                           \
    do {                                                                      \
        (v)->header.list = plist;                                             \
        (v)->header.prev = (plist)->prev;                                     \
        (v)->header.next = (plist)->fake_node;                                \
        (v)->header.prev->header.next = v;                                    \
        (v)->header.next->header.prev = v;                                    \
    } while(0)                                                                \
 
#define dl_setiter(plist, v)                                                  \
    do {                                                                      \
        (plist)->iter = v;                                                    \
    } while(0)                                                                \
 
#define dl_getiter(plist)            ((plist)->iter)

#define dl_isempty(plist)            ((plist)->next == (plist)->fake_node)
#define dl_begin(plist)              ((plist)->next)
#define dl_end(plist)                ((plist)->fake_node)

#endif /* DMEM_LIST_H */

