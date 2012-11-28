#include "linkedlist.h"
    
    
void list_insert(s3dirent_t entry, struct node **tail) {
     struct node *newnode = malloc(sizeof(struct node));
     (*tail)->next = newnode;
     newnode->entry=entry;
     *tail = newnode;
	 (*tail)->next =NULL;
}
 
void list_clear(struct node *list) {
    while (list != NULL) {	
        struct node *tmp = list;
        list = list->next;
        free(tmp);
    }
}

void list_remove( struct node **head, struct node **tail) {
    
	struct node* holder = (*head)->next;
	(*head)->next = ((*head)->next)->next;

	if((*head)->next == NULL){
		*tail = *head;
	}

	free(holder);
	return;
}

