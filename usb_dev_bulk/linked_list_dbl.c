/*
 * linked_list_dbl.cpp
 *
 * Purpose:  Implement a linked list of strings with ops Insert,
 *           Print, Member, Delete, Free_list.
 *           The list nodes are doubly linked
 *
 *  Created on: 17.05.2015
 *      Author: macload1
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */

#include "linked_list_dbl.h"



/*-----------------------------------------------------------------*/
/* Function:   bufcpy
 * Purpose:    copy a source buffer of a certain length to a destination buffer
 * Input arg:  length = length of the buffer
 * 			   bufDest = destination buffer
 * 			   bufSrc = source buffer
 * Return val: void
 */
void bufcpy(char* bufDest, char* bufSrc, uint8_t length)
{
    while(length--)
    {
        *bufDest++ = *bufSrc++;
    }
    return;
}


/*-----------------------------------------------------------------*/
/* Function:   Allocate_node
 * Purpose:    Allocate storage for a list node
 * Input arg:  void
 * Return val: Pointer to the new node
 */
struct list_node_s* Allocate_node(void) {
    struct list_node_s* temp_p;

    temp_p = (struct list_node_s*) malloc(sizeof(struct list_node_s));
    temp_p->ms_time_start = 0;
    temp_p->ms_time_stop = 0;
    temp_p->position = 0;
    temp_p->prev_p = NULL;
    temp_p->next_p = NULL;
    return temp_p;
}  /* Allocate_node */


/*-----------------------------------------------------------------*/
/* Function:   Insert
 * Purpose:    Insert new node in correct chronological order in list
 * Input arg:  ms_time_stamp = time stamp at which motor's position needs to be reached
 *             position = position of motor at the end of ms_time_stamp
 * In/out arg: list_p = pointer to struct storing head and tail ptrs
 */
void Insert(struct list_s* list_p,
            uint32_t ms_time_start,
            uint32_t ms_time_stop,
			uint32_t position)
{
    struct list_node_s* curr_p = list_p->h_p;
    struct list_node_s* temp_p;

    while (curr_p != NULL)
    {
        if (ms_time_start == curr_p->ms_time_start) {
            break;
        } else if (ms_time_start < curr_p->ms_time_start) {
            break;  /* string alphabetically precedes node */
        } else {
            curr_p = curr_p->next_p;
        }
    }

#  ifdef DEBUG
   Print_node("Exited Insert loop: curr_p", curr_p);
#  endif

   temp_p = Allocate_node();
   temp_p->ms_time_start = ms_time_start;
   temp_p->ms_time_stop = ms_time_stop;
   temp_p->position = position;

   if ( list_p->h_p == NULL ) {
      /* list is empty */
      list_p->h_p = list_p->t_p = temp_p;
   } else if ( curr_p == NULL) {
      /* insert at end of list */
      temp_p->prev_p = list_p->t_p;
      list_p->t_p->next_p = temp_p;
      list_p->t_p = temp_p;
   } else if (curr_p == list_p->h_p) {
      /* insert at head of list */
      temp_p->next_p = list_p->h_p;
      list_p->h_p->prev_p = temp_p;
      list_p->h_p = temp_p;
   } else {
      /* middle of list, string < curr_p->data */
      temp_p->next_p = curr_p;
      temp_p->prev_p = curr_p->prev_p;
      curr_p->prev_p = temp_p;
      temp_p->prev_p->next_p = temp_p;
   }
}  /* Insert */


/*-----------------------------------------------------------------*/
/* Function:   Print
 * Purpose:    Print the contents of the nodes in the list
 * Input arg:  list_p = pointers to first and last nodes in list
 */
void Print(struct list_s* list_p) {
    struct list_node_s* curr_p = list_p->h_p;

    printf("list = { ");

    if(!listIsEmpty(list_p))
    {
        while (curr_p != NULL)
        {
            printf("timestart: %d - timestop: %d - position: %d",
                    curr_p->ms_time_start,
                    curr_p->ms_time_stop,
                    curr_p->position);
            if(curr_p->next_p == NULL)
                printf("}");
            else
                printf(",\n");
            curr_p = curr_p->next_p;
        }
    }
    printf("\n");
}  /* Print */


/*-----------------------------------------------------------------*/
/* Function:   Member
 * Purpose:    Search list for datetime
 * Input args: ms_time_stamp = ms_time_stamp to search for
 *             list_p = pointers to first and last nodes in list
 * Return val: 1, if string is in the list, 0 otherwise
 */
int Member(struct list_s* list_p, uint32_t ms_time_start) {
   struct list_node_s* curr_p;

   curr_p = list_p->h_p;
   while (curr_p != NULL)
      if (ms_time_start = curr_p->ms_time_start)
         return 1;
      else if (ms_time_start < curr_p->ms_time_start)
         return 0;
      else
         curr_p = curr_p->next_p;
   return 0;
}  /* Member */


/*-----------------------------------------------------------------*/
/* Function:   Free_node
 * Purpose:    Free storage used by a node of the list
 * In/out arg: node_p = pointer to node to be freed
 */
void Free_node(struct list_node_s* node_p) {
   free(node_p);
}  /* Free_node */


/*-----------------------------------------------------------------*/
/* Function:   listDelete
 * Purpose:    Delete node at the beginning or the end of the list
 * Input arg:  first = boolean to indicate if first node or
 *                     last node has to be deleted
 * In/out arg  list_p = pointers to head and tail of list
 */
void listDelete(struct list_s* list_p, bool first) {
    struct list_node_s* curr_p;

    if(list_p->h_p != NULL)
    {
        if(list_p ->h_p->next_p == NULL)
        {
            /* Only node in list */
            curr_p = list_p->h_p;
            list_p->h_p = list_p->t_p = NULL;
        }
        else if(first)
        {
            /* First node in list */
            curr_p = list_p->h_p;
            list_p->h_p = curr_p->next_p;
            list_p->h_p->prev_p = NULL;
        }
        else
        {
            /* Last node in list */
            curr_p = list_p->t_p;
            list_p->t_p = curr_p->prev_p;
            list_p->t_p->next_p = NULL;
        }
        Free_node(curr_p);
    }
}  /* Delete */


/*-----------------------------------------------------------------*/
/* Function:   Free_list
 * Purpose:    Free storage used by list
 * In/out arg: list_p = pointers to head and tail of list
 */
void Free_list(struct list_s* list_p) {
   struct list_node_s* curr_p;
   struct list_node_s* following_p;

   curr_p = list_p->h_p;
   while (curr_p != NULL) {
      following_p = curr_p->next_p;
#     ifdef DEBUG
      printf("Freeing %s\n", curr_p->data);
#     endif
      Free_node(curr_p);
      curr_p = following_p;
   }

   list_p->h_p = list_p->t_p = NULL;
}  /* Free_list */


/*-----------------------------------------------------------------*/
/* Function:  Print_node
 * Purpose:   Print the data member in a node or NULL if the
 *            pointer is NULL
 * In args:   title:  name of the node
 *            node_p:  pointer to node
 */
void Print_node(struct list_node_s* node_p) {
    if (node_p != NULL)
    {
        printf("timestart: %d - timestop: %d - position: %d",
                node_p->ms_time_start,
                node_p->ms_time_stop,
                node_p->position);
    }
    else
        printf("NULL\n");
}  /* Print_node */


/*-----------------------------------------------------------------*/
/* Function:  listIsEmpty
 * Purpose:   Checks if the list is empty
 * In args:   list_p = pointers to head and tail of list
 * Return val: boolean true if list is empty, false otherwise
 */
bool listIsEmpty(struct list_s* list_p)
{
    return (list_p->h_p == NULL);
}


