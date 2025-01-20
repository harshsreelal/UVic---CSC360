#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define MAXCUSTOMERS 100

typedef struct {
    int id;
    int class_type; // 0 for economy, 1 for business
    int arrival_time;
    int service_time;
} Customer;

typedef struct {
    Customer customers[MAXCUSTOMERS];
    int front;
    int rear;
    int count;
} Queue;

Queue businessQueue;
Queue economyQueue;

pthread_mutex_t mutex;
pthread_cond_t conVar;
pthread_mutex_t clerk_mutexes[5];
struct timeval startTime; 
int customersServed = 0;
int totalCustomers = 0;
int stopClerks = 0;

double totalWaitingTimeAll = 0.0;
double totalWaitingTimeBusiness = 0.0;
double totalWaitingTimeEconomy = 0.0;
int businessCustomerCount = 0;
int economyCustomerCount = 0;
double queueEntryTime = 0.0;

void initQueue(Queue *q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

int isQueueEmpty(Queue *q) {
    return q->count == 0;
}

void enqueue(Queue *q, Customer customer) {
    if (q->count < MAXCUSTOMERS) {
        q->rear = (q->rear + 1) % MAXCUSTOMERS;
        q->customers[q->rear] = customer;
        q->count++;
    }
}

Customer dequeue(Queue *q) {
    Customer customer = {0};
    if (!isQueueEmpty(q)) {
        customer = q->customers[q->front];
        q->front = (q->front + 1) % MAXCUSTOMERS;
        q->count--;
    }
    return customer;
}

double getCurrentSimulationTime() {
    struct timeval cur_time;
    double cur_secs, init_secs;

    init_secs = (startTime.tv_sec + (double)startTime.tv_usec / 1000000);
    gettimeofday(&cur_time, NULL);
    cur_secs = (cur_time.tv_sec + (double)cur_time.tv_usec / 1000000);

    return cur_secs - init_secs;
}

void *customer_thread(void *arg) {
    Customer *customer = (Customer *)arg;

    // Simulate arrival time
    usleep(customer->arrival_time * 100000); // Convert to microseconds

    printf("A customer arrives: customer ID %2d.\n", customer->id);

    pthread_mutex_lock(&mutex);

    queueEntryTime = getCurrentSimulationTime();

    // Enqueue customer
    if (customer->class_type == 1) {
        enqueue(&businessQueue, *customer);
        printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d.\n", 1, businessQueue.count);
    } else {
        enqueue(&economyQueue, *customer);
        printf("A customer enters a queue: the queue ID %1d, and length of the queue %2d.\n", 0, economyQueue.count);
    }

    pthread_cond_signal(&conVar);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *clerk_thread(void *arg) {
    int clerk_id = *(int *)arg;

    while (1) {
        Customer customer;

        pthread_mutex_lock(&mutex);

        // Wait for a customer to be available
        while (isQueueEmpty(&businessQueue) && isQueueEmpty(&economyQueue) && !stopClerks) {
            pthread_cond_wait(&conVar, &mutex);
        }

        // Break the loop if all customers have been served
        if (stopClerks && isQueueEmpty(&businessQueue) && isQueueEmpty(&economyQueue)) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Serve a business class customer if available
        if (!isQueueEmpty(&businessQueue)) {
            customer = dequeue(&businessQueue);
        } else if (!isQueueEmpty(&economyQueue)) {
            customer = dequeue(&economyQueue);
        }

        pthread_mutex_unlock(&mutex);

        // Serve the customer
        double startServiceTime = getCurrentSimulationTime();
        printf("A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d.\n", 
               startServiceTime, customer.id, clerk_id);

        // Calculate the total waiting times
        double waitingTime = startServiceTime - queueEntryTime;
        totalWaitingTimeAll += waitingTime;
        if (customer.class_type == 1) {
            totalWaitingTimeBusiness += waitingTime;
            businessCustomerCount++;
        } else {
            totalWaitingTimeEconomy += waitingTime;
            economyCustomerCount++;
        }

        // Simulate service time
        usleep(customer.service_time * 100000); // Convert to microseconds

        double endServiceTime = getCurrentSimulationTime();
        printf("A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d.\n", 
               endServiceTime, customer.id, clerk_id);

        // Increment the number of served customers
        pthread_mutex_lock(&mutex);
        customersServed++;
        if (customersServed == totalCustomers) {
            stopClerks = 1;
            pthread_cond_broadcast(&conVar); // Wake up all clerks
        }
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Error: File not found");
        return 1;
    }

    // Initialize queues
    initQueue(&businessQueue);
    initQueue(&economyQueue);

    // Initialize mutexes and condition variables
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&conVar, NULL);
    for (int i = 0; i < 5; i++) {
        pthread_mutex_init(&clerk_mutexes[i], NULL);
    }

    // Read the input fptr
    FILE *fptr = fopen(argv[1], "r");
    if (fptr == NULL) {
        perror("Error opening fptr");
        return 1;
    }

    fscanf(fptr, "%d", &totalCustomers);
    Customer customers[totalCustomers];
    for (int i = 0; i < totalCustomers; i++) {
        fscanf(fptr, "%d:%d,%d,%d", &customers[i].id, &customers[i].class_type, &customers[i].arrival_time, &customers[i].service_time);
    }
    fclose(fptr);

    // Get the start time
    gettimeofday(&startTime, NULL); // record simulation start time

    // Create customer threads
    pthread_t customer_threads[totalCustomers];
    for (int i = 0; i < totalCustomers; i++) {
        pthread_create(&customer_threads[i], NULL, customer_thread, &customers[i]);
    }

    // Create clerk threads
    pthread_t clerk_threads[5];
    int clerk_ids[5] = {0, 1, 2, 3, 4};
    for (int i = 0; i < 5; i++) {
        pthread_create(&clerk_threads[i], NULL, clerk_thread, &clerk_ids[i]);
    }

    // Join customer threads
    for (int i = 0; i < totalCustomers; i++) {
        pthread_join(customer_threads[i], NULL);
    }

    // Join clerk threads
    for (int i = 0; i < 5; i++) {
        pthread_join(clerk_threads[i], NULL);
    }

    // Calculate the average waiting times
    double avgWaitingTimeAll = totalWaitingTimeAll / totalCustomers;
    double avgWaitingTimeBusiness = totalWaitingTimeBusiness / businessCustomerCount;
    double avgWaitingTimeEconomy = totalWaitingTimeEconomy / economyCustomerCount;

    printf("Average waiting time for all customers: %.2f\n", avgWaitingTimeAll);
    printf("Average waiting time for business-class customers: %.2f\n", avgWaitingTimeBusiness);
    printf("Average waiting time for economy-class customers: %.2f\n", avgWaitingTimeEconomy);


    // Destroy mutexes and condition variables
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conVar);
    for (int i = 0; i < 5; i++) {
        pthread_mutex_destroy(&clerk_mutexes[i]);
    }

    return 0;
}
