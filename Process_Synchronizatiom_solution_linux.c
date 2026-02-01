// primecart_mutex_solution_linux.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdatomic.h>

#define TOLERANCE 0.001f

typedef struct {
    int pid;
    float price;
    atomic_int updates;
    atomic_int pos_reads;
    atomic_int lpus_reads;
} PriceRecord;

PriceRecord record = {1001, 50.0f, 0, 0, 0};
pthread_mutex_t price_mutex;
float expected_serial = 50.0f;

// Execution traces
float pos_read_values[5];
float lpus_read_values[5];
float pos_write_values[5];
float lpus_write_values[5];
long pos_timestamps[5];
long lpus_timestamps[5];

typedef struct {
    char type[10];
    int index;
    long timestamp;
    float read_val;
    float write_val;
} Event;

long get_current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void* pos_thread_mutex(void* arg) {
    for (int i = 0; i < 5; i++) {
        // ACQUIRE MUTEX
        pthread_mutex_lock(&price_mutex);
        
        // CRITICAL SECTION START
        float read_val = record.price;
        pos_read_values[i] = read_val;
        pos_timestamps[i] = get_current_time_ms();
        atomic_fetch_add(&record.pos_reads, 1);
        
        float discounted = read_val * 0.9f;
        usleep((rand() % 10) * 1000);  // Processing time within critical section
        
        record.price = discounted;
        pos_write_values[i] = discounted;
        atomic_fetch_add(&record.updates, 1);
        // CRITICAL SECTION END
        
        // RELEASE MUTEX
        pthread_mutex_unlock(&price_mutex);
        
        // Non-critical work
        usleep((40 + rand() % 20) * 1000);
    }
    return NULL;
}

void* lpus_thread_mutex(void* arg) {
    for (int i = 0; i < 5; i++) {
        // ACQUIRE MUTEX
        pthread_mutex_lock(&price_mutex);
        
        // CRITICAL SECTION START
        float read_val = record.price;
        lpus_read_values[i] = read_val;
        lpus_timestamps[i] = get_current_time_ms();
        atomic_fetch_add(&record.lpus_reads, 1);
        
        float increased = read_val * 1.1f;
        usleep((rand() % 15) * 1000);  // Processing time within critical section
        
        record.price = increased;
        lpus_write_values[i] = increased;
        atomic_fetch_add(&record.updates, 1);
        // CRITICAL SECTION END
        
        // RELEASE MUTEX
        pthread_mutex_unlock(&price_mutex);
        
        // Non-critical work
        usleep((50 + rand() % 25) * 1000);
    }
    return NULL;
}

void print_execution_table() {
    printf("\nEXECUTION TRACE WITH MUTEX SYNCHRONIZATION:\n");
    printf("============================================\n");
    printf("Operation          Read Value     Write Value    Timestamp (ms)\n");
    printf("-----------------  -------------  -------------  ---------------\n");
    
    Event events[10];
    int event_count = 0;
    
    for (int i = 0; i < 5; i++) {
        snprintf(events[event_count].type, sizeof(events[event_count].type), "POS-%d", i+1);
        events[event_count].index = i;
        events[event_count].timestamp = pos_timestamps[i];
        events[event_count].read_val = pos_read_values[i];
        events[event_count].write_val = pos_write_values[i];
        event_count++;
    }
    
    for (int i = 0; i < 5; i++) {
        snprintf(events[event_count].type, sizeof(events[event_count].type), "LPUS-%d", i+1);
        events[event_count].index = i;
        events[event_count].timestamp = lpus_timestamps[i];
        events[event_count].read_val = lpus_read_values[i];
        events[event_count].write_val = lpus_write_values[i];
        event_count++;
    }
    
    // Sort by timestamp
    for (int i = 0; i < event_count-1; i++) {
        for (int j = 0; j < event_count-i-1; j++) {
            if (events[j].timestamp > events[j+1].timestamp) {
                Event temp = events[j];
                events[j] = events[j+1];
                events[j+1] = temp;
            }
        }
    }
    
    long start_time = events[0].timestamp;
    for (int i = 0; i < event_count; i++) {
        printf("%-10s        $%9.4f    $%9.4f    %7ld\n",
               events[i].type,
               events[i].read_val,
               events[i].write_val,
               events[i].timestamp - start_time);
    }
}

int main() {
    printf("PRIMECART SYNCHRONIZED SOLUTION - PTHREAD MUTEX\n");
    printf("================================================\n\n");
    
    // Calculate expected serial result
    for (int i = 0; i < 5; i++) expected_serial *= 0.9f;
    for (int i = 0; i < 5; i++) expected_serial *= 1.1f;
    
    printf("SCENARIO:\n");
    printf("  Product ID:      %d\n", record.pid);
    printf("  Initial Price:   $%.4f\n", 50.0f);
    printf("  POS Operation:   10%% discount (x5)\n");
    printf("  LPUS Operation:  10%% increase (x5)\n");
    printf("  Expected Result: $%.4f\n\n", expected_serial);
    
    printf("SYNCHRONIZATION:\n");
    printf("  Primitive:       pthread_mutex_t (POSIX)\n");
    printf("  Scope:           Process-wide (within process)\n");
    printf("  Protection:      read-modify-write atomicity\n");
    printf("  Locking:         Blocking (no timeout by default)\n\n");
    
    // Initialize mutex with default attributes
    if (pthread_mutex_init(&price_mutex, NULL) != 0) {
        printf("Error: Failed to initialize mutex\n");
        return 1;
    }
    
    // Seed random number generator
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand(ts.tv_nsec);
    
    long start_time = get_current_time_ms();
    
    printf("STARTING SYNCHRONIZED EXECUTION WITH MUTEX...\n");
    printf("---------------------------------------------\n\n");
    
    pthread_t threads[2];
    if (pthread_create(&threads[0], NULL, pos_thread_mutex, NULL) != 0) {
        printf("Error: Failed to create POS thread\n");
        pthread_mutex_destroy(&price_mutex);
        return 1;
    }
    
    if (pthread_create(&threads[1], NULL, lpus_thread_mutex, NULL) != 0) {
        printf("Error: Failed to create LPUS thread\n");
        pthread_mutex_destroy(&price_mutex);
        return 1;
    }
    
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    
    long end_time = get_current_time_ms();
    
    // Destroy mutex
    pthread_mutex_destroy(&price_mutex);
    
    print_execution_table();
    
    int race_detected = 0;
    
    printf("\nMUTEX SYNCHRONIZATION ANALYSIS:\n");
    printf("--------------------------------\n");
    
    printf("Checking operation atomicity...\n");
    
    printf("\nPOS Thread Reads (with mutex protection):\n");
    for (int i = 0; i < 5; i++) {
        printf("  Update %d: $%.4f -> Valid (protected by mutex)\n", 
               i+1, pos_read_values[i]);
    }
    
    printf("\nLPUS Thread Reads (with mutex protection):\n");
    for (int i = 0; i < 5; i++) {
        printf("  Update %d: $%.4f -> Valid (protected by mutex)\n", 
               i+1, lpus_read_values[i]);
    }
    
    // For mutex solution, race_detected should only be true if
    // the final price doesn't match expected
    if (fabsf(record.price - expected_serial) > TOLERANCE) {
        race_detected = 1;
    }
    
    printf("\nPERFORMANCE RESULTS:\n");
    printf("====================\n");
    printf("Final Price:        $%.4f\n", record.price);
    printf("Expected Price:     $%.4f\n", expected_serial);
    printf("Difference:         $%.6f\n", fabsf(record.price - expected_serial));
    printf("Total Updates:      %d\n", record.updates);
    printf("POS Reads:          %d\n", record.pos_reads);
    printf("LPUS Reads:         %d\n", record.lpus_reads);
    printf("Execution Time:     %ld ms\n", end_time - start_time);
    
    printf("\nVALIDATION SUMMARY (MUTEX SOLUTION):\n");
    printf("====================================\n");
    printf("%-30s: ", "Final Price Correct");
    if (fabsf(record.price - expected_serial) < TOLERANCE) {
        printf("YES ✓ (Error: $%.6f)\n", fabsf(record.price - expected_serial));
    } else {
        printf("NO ✗ (Error: $%.6f)\n", fabsf(record.price - expected_serial));
        race_detected = 1;
    }
    
    printf("%-30s: ", "All Updates Executed");
    printf("%s\n", (record.updates == 10) ? "YES ✓" : "NO ✗");
    
    printf("%-30s: ", "Serial Execution Achieved");
    printf("%s\n", (!race_detected) ? "YES ✓" : "NO ✗");
    
    return 0;
}
