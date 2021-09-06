#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

struct node
{
  int value;
  node *next;        // POINTER FOR NODE NEXT
  pthread_mutex_t m; // MUTEX FOR NODE
};

struct list
{
  node *head;             // POINTER FOR LIST
  pthread_mutex_t list_m; // MUTEX FOR LIST
};

void print_node(node *node)
{
  // DO NOT DELETE
  if (node)
  {
    printf("%d ", node->value);
  }
}

list *create_list() // CREATING NEW LIST
{
  list *newList = (list *)malloc(sizeof(list)); // ALLOCATE MEMORY FOR NEW LIST
  if (newList == NULL)                          // MALLOC FAILED
  {
    printf("allocating new list failed!"); // MALLOC FAILED
    return NULL;
  }
  newList->head = NULL;

  if (pthread_mutex_init(&(newList->list_m), NULL))
  { // INITIALIZING LIST MUTEX LOCK
    printf("intializing mutex failed");
    return NULL;
  }

  return newList;
}

void delete_list(list *list) // DELETING EXISTING LIST
{
  if (list == NULL) // LIST IS INVALID OR DOESN'T EXIST
  {
    return;
  }
  else
  {
    pthread_mutex_lock(&(list->list_m)); // LOCK LIST MUTEX
    node *tmp;
    if (list->head == NULL)
    {
      pthread_mutex_unlock(&(list->list_m)); // UNLOCK LIST MUTEX
      if (pthread_mutex_destroy(&(list->list_m)))
      {
        printf("dstroying mutex failed!");
        return;
      } // DESTROY LIST MUTEX
      free(list);
      return;
    }

    node *current = list->head;
    pthread_mutex_lock(&(current->m)); // LOCK HEAD MUTEX
    while (current->next != NULL)      // LOOP WHILE LIST IS NOT EMPTY
    {
      pthread_mutex_lock(&(current->next->m)); // LOCK HEAD MUTEX
      tmp = current;
      current = current->next;
      pthread_mutex_unlock(&(tmp->m)); // UNLOCK HEAD MUTEX
      if (pthread_mutex_destroy(&(tmp->m)))
      { // DESTROY HEAD MUTEX
        printf("dstroying mutex failed!");
        return;
      }
      free(tmp);
      pthread_mutex_unlock(&(list->list_m)); // UNLOCK LIST MUTEX
      if (pthread_mutex_destroy(&(list->list_m)))
      {
        printf("dstroying mutex failed!");
        return;
      } // DESTROY LIST MUTEX
    }
    if (current->next == NULL)
    {
      pthread_mutex_unlock(&(current->m)); // UNLOCK HEAD MUTEX
      if (pthread_mutex_destroy(&(current->m)))
      { // DESTROY HEAD MUTEX
        printf("dstroying mutex failed!");
        return;
      }
      free(current);
    }
  }
  free(list); // FREE SPACE ALLOCATING
}

void insert_value(list *list, int value) // INSERTING NEW VALUE TO EXISTING LIST
{
  if (list == NULL) // LIST DOESN'T EXIST
  {
    return;
  }

  node *newNode = (node *)malloc(sizeof(node)); // ALLOCATING MEMORY FOR NEW NODE
  if (newNode == NULL)
  {
    printf("allocating new node failed!"); // MALLOC FAILED
    return;
  }
  else
  {
    newNode->value = value;
    if (pthread_mutex_init(&(newNode->m), NULL))
    { // INTIALIZING NODE MUTEX LOCK
      printf("intializing mutex failed");
      return;
    }
    pthread_mutex_lock(&(newNode->m)); // LOCK NEW NODE MUTEX
  }

  pthread_mutex_lock(&(list->list_m)); // LOCK LIST MUTEX

  if (list->head == NULL) // LIST IS EMPTY
  {
    list->head = newNode;
    newNode->next = NULL;
    pthread_mutex_unlock(&(newNode->m)); // UNLOCK NEWNODE MUTEX
    pthread_mutex_unlock(&(list->list_m)); // UNLOCK LIST MUTEX
  }

  else if (newNode->value < list->head->value) // NEW NODE IS FIRST
  {
    pthread_mutex_lock(&(list->head->m)); // LOCK OLD FIRST EXISTING NODE
    newNode->next = list->head;
    list->head = newNode;
    pthread_mutex_unlock(&(newNode->m)); // UNLOCK NEWNODE MUTEX
    pthread_mutex_unlock(&(list->head->next->m)); // UNLOCK OLD FIRST EXISTING NODE
    pthread_mutex_unlock(&(list->list_m));        // UNLOCK LIST MUTEX
  }

  else
  {                                        // NEW NODE IS NOT FIRST
    node *tmp = list->head;                // POINTER FOR LOOP
    node *swap=NULL;                            // POINTER FOR SWAPPING
    pthread_mutex_lock(&(tmp->m)); // LOCK HEAD
    while (tmp != NULL)                    // LOOP FOR FINDING NEW NODE'S PLACE
    {
      if(tmp==list->head){
        pthread_mutex_unlock(&(list->list_m)); // UNLOCK LIST MUTEX
      }

      if ((tmp->value < newNode->value || tmp->value == newNode->value) && tmp->next == NULL)
      { // NEW NODE IS LAST
        tmp->next = newNode;
        newNode->next = NULL;
        pthread_mutex_unlock(&(tmp->m)); // UNLOCK HEAD
        pthread_mutex_unlock(&(newNode->m)); // UNLOCK NEWNODE
        break;
      }

      else
      {
        if (tmp->next && (tmp->value < newNode->value || tmp->value == newNode->value) && (tmp->next->value > newNode->value || tmp->next->value == newNode->value) )
        {
          pthread_mutex_lock(&(tmp->next->m)); // LOCK CURRENT->NEXT NODE
          newNode->next = tmp->next;
          tmp->next = newNode;
          pthread_mutex_unlock(&(tmp->m));           // UNLOCK OLD CURRENT
          pthread_mutex_unlock(&(newNode->next->m)); // UNLOCK OLD CURRENT->NEXT
          pthread_mutex_unlock(&(newNode->m)); // UNLOCK NEWNODE MUTEX
          break;
        }
        else if (!(tmp->next) && (tmp->value < newNode->value || tmp->value == newNode->value))
        {
          newNode->next = tmp->next;
          tmp->next = newNode;
          pthread_mutex_unlock(&(tmp->m)); // UNLOCK OLD CURRENT
          pthread_mutex_unlock(&(newNode->m)); // UNLOCK NEWNODE MUTEX
          break;
        }

        else // NEW NODE'S PLACE HASN'T BEEN FOUND YET
        {
          swap = tmp;
          tmp = tmp->next;
          pthread_mutex_lock(&(tmp->m));  // LOCK OLD CURRENT NEXT
          pthread_mutex_unlock(&(swap->m)); // UNLOCK OLD CURRENT
        }
      }
    }
  }
  return;
}

void remove_value(list *list, int value) // REMOVING VALUE FRO EXISTING LIST
{
  if (list == NULL || list->head == NULL) // LIST IS INVALID OR DOESN'T EXIST
  {
    return;
  }

  pthread_mutex_lock(&(list->list_m)); // LOCK LIST MUTEX

  if (list->head->value == value) // VALUE IS FIRST NODE
  {
    node *tmp = list->head;        // POINTER FOR LIST
    pthread_mutex_lock(&(tmp->m)); // LOCK CURRENT NODE
    if (list->head->next != NULL)
    {
      pthread_mutex_lock(&(list->head->next->m)); // LOCK FIRST->NEXT NODE
    }
    list->head = list->head->next;
    if (list->head != NULL)
    {
      pthread_mutex_unlock(&(list->head->m)); // UNLOCK NEW FIRST NODE
    }
    pthread_mutex_unlock(&(tmp->m));
    if (pthread_mutex_destroy(&(tmp->m)))
    {
      printf("dstroying mutex failed!");
      return;
    } // DESTROY DELETED NODE
    free(tmp);
    pthread_mutex_unlock(&(list->list_m)); // UNLOCK LIST MUTEX
  }

  else // VALUE ISN'T FIRST NODE
  {
    node *help = NULL;
    node *current = list->head;
    pthread_mutex_lock(&(current->m)); // LOCK HEAD NODE
    while (current != NULL)                // WHILE LIST ISN'T EMPTY AND WE HAVEN'T FOUND VALUE
    {
      //pthread_mutex_lock(&(current->m)); // LOCK HEAD NODE
      if(current==list->head){
        pthread_mutex_unlock(&(list->list_m)); // UNLOCK LIST MUTEX
      }
      if (current->next == NULL) // NEXT NODE IS NULL
      {
        if (current->value == value) // VALUE IS CURRENT NODE
        {
          //pthread_mutex_lock(&(current->m)); // LOCK HEAD NODE
          help = current;
          current = NULL;
          //pthread_mutex_unlock(&(current->m)); // UNLOCK HEAD NODE
          pthread_mutex_unlock(&(help->m));

          if (pthread_mutex_destroy(&(help->m)))
          {
            printf("dstroying mutex failed!");
            return;
          } // DESTROY HEAD NODE
          free(help);
        }
        
        if(current->value != value){
          pthread_mutex_unlock(&(current->m)); // UNLOCK LAST NODE MUTEX
        }
        break;
      }

      //pthread_mutex_lock(&(current->m)); // LOCK CURRENT NODE
      if (current->next->value == value) // VALUE IS CURRENT->NEXT NODE
      {
        if (current->next->next != NULL)
        {
          pthread_mutex_lock(&(current->next->m));       // LOCK THE NODE WE WAN'T TO DELETE
          pthread_mutex_lock(&(current->next->next->m)); // LOCK THE NEXT NODE TO THE NODE WE WANT TO DELETE
          help = current->next;
          current->next = current->next->next;
          pthread_mutex_unlock(&(help->m)); // UNLOCK THE DELETED NODE
          if (pthread_mutex_destroy(&(help->m)))
          {
            printf("dstroying mutex failed!");
            return;
          } // DESTROY THE DELETED NODE
          free(help);
          pthread_mutex_unlock(&(current->next->m)); // UNLOCK CURRENT_HEAD->NEXT
        }
        else
        {
          pthread_mutex_lock(&(current->next->m)); // LOCK THE NODE WE WAN'T TO DELETE
          help = current->next;
          current->next = current->next->next;
          pthread_mutex_unlock(&(help->m)); // UNLOCK THE DELETED NODE
          if (pthread_mutex_destroy(&(help->m)))
          {
            printf("dstroying mutex failed!");
            return;
          } // DESTROY THE DELETED NODE
          free(help);
        }
        pthread_mutex_unlock(&(current->m)); // UNLOCK HEAD
        break;
      }
      else{
        node* newCurrent=current->next;
        pthread_mutex_unlock(&(current->m)); // UNLOCK HEAD
        current = newCurrent;
        if(current){
          pthread_mutex_lock(&(current->m));
        }
      }
    }
  }
  return;
}

void print_list(list *list)
{
  if (list == NULL || list->head==NULL) // LIST IS INVALID OR DOESN'T EXIST
  {
    printf("\n");
    return;
  }
  else
  {
    pthread_mutex_lock(&(list->list_m)); // LOCK LIST MUTEX
    node *tmp;
    node *current = list->head;
    pthread_mutex_lock(&(current->m)); // LOCKING HEAD MUTEX
    while (current != NULL) // LOOP WHILE LIST IS NOT EMPTY
    {
      tmp = current;
      print_node(current);
      node* newCurrent = current->next;
      current = newCurrent;
      if(current){
        pthread_mutex_lock(&(current->m)); // LOCKING NEW CURRENT MUTEX
      }
      pthread_mutex_unlock(&(tmp->m)); // UNLOCKING HEAD MUTEX
      if (tmp == list->head)
      {
        pthread_mutex_unlock(&(list->list_m)); // UNLOCK LIST MUTEX
      }
    }
  }
  printf("\n"); // DO NOT DELETE
}


void count_list(list *list, int (*predicate)(int))
{
  int count = 0;    // DO NOT DELETE
  if (list == NULL) // LIST IS INVALID OR DOESN'T EXIST
  {
    return;
  }
  else
  {
    pthread_mutex_lock(&(list->list_m)); // LOCK LIST MUTEX
    node *tmp;
    node *current = list->head;
    pthread_mutex_lock(&(current->m)); // LOCKING HEAD MUTEX
    while (current != NULL) // LOOP WHILE LIST IS NOT EMPTY
    {
      if ((*predicate)(current->value))  // CHECK PREDICATE FUNCTION VALUE FOR CURRENT NODE
      {
        count++;
      }
      tmp = current;
      node* newcurrent = current->next;
      current = newcurrent;
      if(current){
        pthread_mutex_lock(&(newcurrent->m)); // LOCKING NEW CURRENT MUTEX
      }
      pthread_mutex_unlock(&(tmp->m)); // UNLOCKING HEAD MUTEX
      if (tmp == list->head)
      {
        pthread_mutex_unlock(&(list->list_m)); // UNLOCK LIST MUTEX
      }
    }
  }

  printf("%d items were counted\n", count); // DO NOT DELETE
}
