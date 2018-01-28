/*
 * File:   linked_list_dbl.h
 * Author: macload1
 *
 * Created on 17. Mai 2015, 17:38
 */

#ifndef LINKED_LIST_DBL_H
#define	LINKED_LIST_DBL_H

#ifdef	__cplusplus
extern "C" {
#endif

struct list_node_s {
   uint32_t ms_time_start;
   uint32_t ms_time_stop;
   uint32_t position;
   struct list_node_s* prev_p;
   struct list_node_s* next_p;
};

struct list_s {
   struct list_node_s* h_p;
   struct list_node_s* t_p;
};


struct list_node_s* Allocate_node(void);
void Insert(struct list_s* list_p,
            uint32_t ms_time_start,
            uint32_t ms_time_stop,
			uint32_t position);
void Print(struct list_s* list_p);
int Member(struct list_s* list_p, uint32_t ms_time_stamp);
void Free_node(struct list_node_s* node_p);
void listDelete(struct list_s* list_p, bool first);
void Free_list(struct list_s* list_p);
void Print_node(struct list_node_s* node_p);
bool listIsEmpty(struct list_s* list_p);


#ifdef	__cplusplus
}
#endif

#endif	/* LINKED_LIST_DBL_H */

