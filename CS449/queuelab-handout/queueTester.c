#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

void print_str(char *s);

int main(){
  queue_t *line = q_new();
  q_insert_head(line, "hello");
  q_insert_head(line, "hi");
  q_insert_head(line, "birju");
  q_insert_head(line, "petrucci");

  char *str = line->head->value;
  print_str(str);
  str = line->head->next->value;
  print_str(str);
  str = line->head->next->next->value;
  print_str(str);
  str = line->head->next->next->next->value;
  print_str(str);

  q_free(line);
  printf("%p\n",line);
}

void print_str(char *s){
  while(*s != '\0'){
    printf("%c", *s);
    s = s + 1;
  }
  printf("\n");
}

/*
  Create empty queue.
  Return NULL if could not allocate space.
*/
queue_t *q_new()
{
  //allocates memory for new queue
  queue_t *q =  malloc(sizeof(queue_t));

  //if space was allocated, sets head and tail to null and size to zero
  //if space was not allocated, returns null pointer
  if(q != NULL){
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
  }

  return q;
}

/* Free all storage used by queue */
void q_free(queue_t *q)
{
  //if q is null, nothing happens
  if(q != NULL){
    //if the next item is not null, current item gets deleted
    //and function recurses to delete the next item
    if(q->head->next != NULL){
      list_ele_t *curr_node = q->head;
      q->head = q->head->next;

      //deletes all values in current node
      curr_node->next = NULL;
      free(curr_node->value);
      curr_node->value = NULL;
      free(curr_node);
      curr_node = NULL;

      //continues process for next node in list
      q_free(q);
    }
    //if the next item is null, reached end of list
    //deletes last item and the queue data
    else{
      //deletes head node
      free(q->head->value);
      q->head->value = NULL;
      free(q->head);
      q->head = NULL;
      //deletes queue and sets to null
      free(q);
      q = NULL;
    }
  }
}

/*
 * takes pointer to first character of a string as input
 * calculates the length of that string
 * adds 1 to account for null terminator
 * returns length of string
 */
int get_str_length(char *s){
  int str_len = 0;
  char *curr_char = s;

  //traverses string until it hits the null terminator
  //increments str_len on each pass
  while(*curr_char != '\0'){
    curr_char = curr_char + 1;
    str_len = str_len + 1;
  }
  str_len = str_len + 1; //+1 to account for null terminator

  return str_len;
}

/*
 * copies n-1 characters from src string to dst string
 * adds null terminator to end of dst
 * dst is a string of n length, ending in null terminator
 */
void copy_n_chars(int n, char *src, char *dst){
  //only executes if there is a valid value for n
  //and if both src and dst do not contain null
  if(n > 0 && src != NULL && dst != NULL){
    int i = 0;
    while(i < n - 1){
      *(dst + i) = *(src + i);
      i = i + 1;
    }
    *(dst + (n - 1)) = '\0';
  }
}

/*
  Attempt to insert element at head of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(queue_t *q, char *s)
{
  //returns false if q is NULL
  if(q == NULL) {return false;}

  //allocates space in memory for next node
  //returns false if there is no space in memory for a new node
  list_ele_t *newh;
  newh = malloc(sizeof(list_ele_t));
  if(newh == NULL) {return false;}

  //calculates the length of input string
  //allocates space in memory for node string
  //returns false if there is no space in memory for the string
  int str_len = get_str_length(s);
  newh->value = malloc(sizeof(char) * str_len);
  if(newh->value == NULL){return false;}

  /* If program reaches here, there is space in memory
   * allocated for the new node and its string data.
   * It is safe to add the new node to list,
   * and safe to add characters to the string. */

   //copies all characters from input string into node string
   copy_n_chars(str_len, s, newh->value);

   //if tail has no value, the list is empty
   //newh is the first and last element in the list
   if(q->tail == NULL){
     newh->next = NULL; //since newh is last element, its next element is null
     q->tail = newh;
   }

  //places new node at the head of the list
  newh->next = q->head;
  q->head = newh;

  //increases size of list by 1
  q->size = q->size + 1;

  return true;
}

/*
  Attempt to insert element at tail of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(queue_t *q, char *s)
{
  //returns false if q equals NULL
  if(q == NULL) {return false;}

  //allocates space in memory for the new tail node
  //returns false if there is no memory available
  list_ele_t *newt;
  newt = malloc(sizeof(list_ele_t));
  if(newt == NULL) {return false;}

  //gets the length of the input string
  //allocates space in memory for node string
  //returns false if there is no space
  int str_len = get_str_length(s);
  newt->value = malloc(sizeof(char) * str_len);
  if(newt->value == NULL) {return false;}

  /* If program reaches here, there is space in memory
   * allocated for the new node and its string data.
   * It is safe to add the new node to list,
   * and safe to add characters to the string. */

   //copies all characters from input string into node string
   copy_n_chars(str_len, s, newt->value);

   //since newt is at the end of the list,
   //it has no next value, so it is set to null
   newt->next = NULL;

   //places new node at tail of list
   //if head and tail have no value (i.e. list is empty), both set to new node
   if(q->head == NULL && q->tail == NULL){
     //both head and tail point to newt
     //newt is the only value in the list
     q->head = newt;
     q->tail = newt;
   }
   else{
     q->tail->next = newt;
     q->tail = newt;
   }

   //increases size by 1
   q->size = q->size + 1;

  return true;
}

/*
  Attempt to remove element from head of queue.
  Return true if successful.
  Return false if queue is NULL or empty.
  If sp is non-NULL and an element is removed, copy the removed string to *sp
  (up to a maximum of bufsize-1 characters, plus a null terminator.)
  The space used by the list element and the string should be freed.
*/
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
  //returns false if q equals null
  //returns false if queue is empty
  //returns false if sp is null
  if(q == NULL) {return false;}
  if(q->head == NULL) {return false;}
  if(sp == NULL) {return false;}

  //to_remove contains copy of head
  list_ele_t *to_remove;
  to_remove = q->head;

  //stores pointer to the start of the string to be removed
  char *removed_string = to_remove->value;

  //max that can be stored in sp is bufsize
  //str_len is the length of string removed from queue
  //if str_len > bufsize, stores bufsize-1 chars in sp, plus null terminator
  //else stores str_len-1 characters in sp, plus null terminator
  int str_len = get_str_length(removed_string);
  if(str_len > bufsize){
    copy_n_chars(bufsize, removed_string, sp);
  }
  else{
    copy_n_chars(str_len, removed_string, sp);
  }

  //sets new head to the next item in the list
  //if next is null, head is only item, so head and tail are both set to null
  if(q->head->next != NULL){
    q->head = q->head->next;
  }
  else{
    q->head = NULL;
    q->tail = NULL;
  }

  //size is decreased by 1
  q->size = q->size - 1;

  //the removed string and node are both deallocated from memory
  free(removed_string);
  free(to_remove); //to_remove contains reference to old head

  return true;
}

/*
  Return number of elements in queue.
  Return 0 if q is NULL or empty
 */
int q_size(queue_t *q)
{
  return q->size;
}

/*
  Reverse elements in queue
  No effect if q is NULL or empty
  This function should not allocate or free any list elements
  (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
  It should rearrange the existing ones.
 */
void q_reverse(queue_t *q)
{
  if(q != NULL && !(q->head == NULL && q ->tail == NULL)){
    reverse_node(q, q->head);
  }
}

/*
 * helper function
 * reverses nodes recursively
 * reaches tail, then reverses its way back to head
 */
void reverse_node(queue_t *q, list_ele_t *curr_node){
  //stores next node in list
  list_ele_t *next_node = curr_node->next;

  //checks if at end of list
  //if not, keeps going
  if(next_node != NULL){
    reverse_node(q, next_node);

    //reverses curr_node and next_node
    next_node->next = curr_node;
  }

  //if curr_node = head, we are on the last step
  if(curr_node == q->head){
    //since old head is the new tail, it must have no next element
    curr_node->next = NULL;

    //swaps head and tail
    list_ele_t *temp = q->head;
    q->head = q->tail;
    q->tail = temp;

  }
}
