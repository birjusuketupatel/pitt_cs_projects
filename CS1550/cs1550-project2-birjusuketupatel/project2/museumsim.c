#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "museumsim.h"

#define VISITORS_PER_GUIDE 10
#define GUIDES_ALLOWED_INSIDE 2

//
// In all of the definitions below, some code has been provided as an example
// to get you started, but you do not have to use it. You may change anything
// in this file except the function signatures.
//

struct shared_data {
  pthread_mutex_t lock;
  pthread_cond_t visitor_can_enter;
  pthread_cond_t guide_can_enter;
  pthread_cond_t visitors_are_waiting;
  pthread_cond_t guide_can_leave;

  int tickets;
  int guides_inside;
  int visitors_inside;
  int visitors_waiting;
};

static struct shared_data shared;


/**
 * Set up the shared variables for your implementation.
 *
 * `museum_init` will be called before any threads of the simulation
 * are spawned.
 */
void museum_init(int num_guides, int num_visitors)
{
  pthread_mutex_init(&shared.lock, NULL);
  pthread_cond_init(&shared.visitor_can_enter, NULL);
  pthread_cond_init(&shared.guide_can_enter, NULL);
  pthread_cond_init(&shared.visitors_are_waiting, NULL);
  pthread_cond_init(&shared.guide_can_leave, NULL);

  shared.tickets = MIN(VISITORS_PER_GUIDE * num_guides, num_visitors);
  shared.guides_inside = 0;
  shared.visitors_inside = 0;
  shared.visitors_waiting = 0;
}//museum_init


/**
 * Tear down the shared variables for your implementation.
 *
 * `museum_destroy` will be called after all threads of the simulation
 * are done executing.
 */
void museum_destroy()
{
  pthread_mutex_destroy(&shared.lock);
  pthread_cond_destroy(&shared.visitor_can_enter);
  pthread_cond_destroy(&shared.guide_can_enter);
  pthread_cond_destroy(&shared.visitors_are_waiting);
  pthread_cond_destroy(&shared.guide_can_leave);
}//museum_destroy


/**
 * Implements the visitor arrival, touring, and leaving sequence.
 */
void visitor(int id)
{
  pthread_mutex_lock(&shared.lock);
  {
    visitor_arrives(id);

    if(shared.tickets > 0){
      //consume a ticket
      shared.tickets--;
    }
    else{
      //leave if no tickets remain
      visitor_leaves(id);
      pthread_mutex_unlock(&shared.lock);

      return;
    }

    //while museum is at capacity, wait outside
    shared.visitors_waiting++;
    do{
      //broadcast to guides there is a visitor waiting
      //sleep until a guide admits the visitor
      pthread_cond_broadcast(&shared.visitors_are_waiting);
      pthread_cond_wait(&shared.visitor_can_enter, &shared.lock);
    }while(shared.visitors_inside > VISITORS_PER_GUIDE * shared.guides_inside);
  }
  pthread_mutex_unlock(&shared.lock);

  //allows concurrent touring
  visitor_tours(id);

  //after tour is over, leave museum
  pthread_mutex_lock(&shared.lock);
  {
    shared.visitors_inside--;
    pthread_cond_broadcast(&shared.guide_can_leave);

    visitor_leaves(id);
  }
  pthread_mutex_unlock(&shared.lock);
}//visitor

/**
 * Implements the guide arrival, entering, admitting, and leaving sequence.
 */
void guide(int id)
{
  pthread_mutex_lock(&shared.lock);
  {
    guide_arrives(id);

    //enter museum only if fewer than GUIDES_ALLOWED_INSIDE are inside
    while(shared.guides_inside >= GUIDES_ALLOWED_INSIDE){
      pthread_cond_wait(&shared.guide_can_enter, &shared.lock);
    }

    //enter museum
    guide_enters(id);
    shared.guides_inside++;
  }
  pthread_mutex_unlock(&shared.lock);

  //each guide serves max of VISITORS_PER_GUIDE visitors
  int visitors_served = 0;
  while(visitors_served < VISITORS_PER_GUIDE){
    pthread_mutex_lock(&shared.lock);
    {
      //sleep if nobody is waiting outside
      //wakes when new visitor enters waiting state
      int flag = 0;
      while(shared.visitors_waiting <= 0){
        if(shared.tickets <= 0){
          flag = 1;
          break;
        }

        pthread_cond_wait(&shared.visitors_are_waiting, &shared.lock);
      }

      //no more visitors are coming and none are waiting
      if(flag == 1){
        pthread_mutex_unlock(&shared.lock);
        break;
      }

      //admit next visitor
      //signals that the next visitor can enter
      shared.visitors_waiting--;
      shared.visitors_inside++;

      guide_admits(id);
      pthread_cond_signal(&shared.visitor_can_enter);
    }
    pthread_mutex_unlock(&shared.lock);

    visitors_served++;
  }

  //guide can leave after all visitors have left
  pthread_mutex_lock(&shared.lock);
  {
    //wait until all visitors leave
    while(shared.visitors_inside > 0){
      pthread_cond_wait(&shared.guide_can_leave, &shared.lock);
    }

    //guide exits museum
    guide_leaves(id);
    shared.guides_inside--;

    //allow other guides to enter when all guides inside have left
    if(shared.guides_inside == 0){
      pthread_cond_signal(&shared.guide_can_enter);
    }

    //signal to other guides they may leave
    pthread_cond_broadcast(&shared.guide_can_leave);
  }
  pthread_mutex_unlock(&shared.lock);
}//guide
