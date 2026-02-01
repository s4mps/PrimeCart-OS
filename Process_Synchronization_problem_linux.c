// primecart_race_problem_linux.c
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

void* pos_thread_unsafe(void* arg) {
    for (int i = 0; i < 5; i++) {
        // UNSAFE: No synchronization
        float read_val = record.price;
        pos_read_values[i] = read_val;
        pos_timestamps[i] = get_current_time_ms();
        atomic_fetch_add(&record.pos_reads, 1);
        
        float discounted = read_val * 0.9f;
        usleep((rand() % 20) * 1000);  // Processing time
        
        // UNSAFE: Concurrent write possible
        record.price = discounted;
        pos_write_values[i] = discounted;
        atomic_fetch_add(&record.updates, 1);
        
        usleep((40 + rand() % 20) * 1000);
    }
    return NULL;
}

void* lpus_thread_unsafe(void* arg) {
    for (int i = 0; i < 5; i++) {
        // UNSAFE: No synchronization
        float read_val = record.price;
        lpus_read_values[i] = read_val;
        lpus_timestamps[i] = get_current_time_ms();
        atomic_fetch_add(&record.lpus_reads, 1);
        
        float increased = read_val * 1.1f;
        usleep((rand() % 25) * 1000);  // Processing time
        
        // UNSAFE: Concurrent write possible
        record.price = increased;
        lpus_write_values[i] = increased;
        atomic_fetch_add(&record.updates, 1);
        
        usleep((50 + rand() % 25) * 1000);
    }
    return NULL;
}

void calculate_valid_states(float* valid_states, int* count) {
    float base = 50.0f;
    int idx = 0;
    
    valid_states[idx++] = base;
    
    // Serial execution: all discounts then all increases
    for (int i = 1; i <= 5; i++) {
        base *= 0.9f;
        valid_states[idx++] = base;
    }
    
    for (int i = 1; i <= 5; i++) {
        base *= 1.1f;
        valid_states[idx++] = base;
    }
    
    *count = idx;
}

int is_valid_state(float value, float* valid_states, int count) {
    for (int i = 0; i < count; i++) {
        if (fabsf(value - valid_states[i]) < TOLERANCE) {
            return 1;
        }
    }
    return 0;
}

void print_execution_table() {
    printf("\nEXECUTION TRACE WITHOUT SYNCHRONIZATION:\n");
    printf("==========================================\n");
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
    printf("PRIMECART RACE CONDITION PROBLEM - UNSYNCHRONIZED\n");
    printf("==================================================\n\n");
    
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
    printf("  Primitive:       NONE (Race Condition)\n");
    printf("  Scope:           Process-wide data corruption\n");
    printf("  Risk:            Lost updates, inconsistent data\n\n");
    
    // Calculate valid intermediate states for serial execution
    float valid_states[20];
    int valid_count = 0;
    calculate_valid_states(valid_states, &valid_count);
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand(ts.tv_nsec);
    
    long start_time = get_current_time_ms();
    
    printf("STARTING UNSYNCHRONIZED EXECUTION...\n");
    printf("-------------------------------------\n\n");
    
    pthread_t threads[2];
    pthread_create(&threads[0], NULL, pos_thread_unsafe, NULL);
    pthread_create(&threads[1], NULL, lpus_thread_unsafe, NULL);
    
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    
    long end_time = get_current_time_ms();
    
    print_execution_table();
    
    // Race condition analysis
    int race_detected = 0;
    int invalid_pos_reads = 0;
    int invalid_lpus_reads = 0;
    
    printf("\nRACE CONDITION ANALYSIS:\n");
    printf("-------------------------\n");
    
    printf("POS Thread Reads:\n");
    for (int i = 0; i < 5; i++) {
        printf("  Update %d: $%.4f ", i+1, pos_read_values[i]);
        if (!is_valid_state(pos_read_values[i], valid_states, valid_count)) {
            printf("-> INVALID (Race!)\n");
            race_detected = 1;
            invalid_pos_reads++;
        } else {
            printf("-> Valid\n");
        }
    }
    
    printf("\nLPUS Thread Reads:\n");
    for (int i = 0; i < 5; i++) {
        printf("  Update %d: $%.4f ", i+1, lpus_read_values[i]);
        if (!is_valid_state(lpus_read_values[i], valid_states, valid_count)) {
            printf("-> INVALID (Race!)\n");
            race_detected = 1;
            invalid_lpus_reads++;
        } else {
            printf("-> Valid\n");
        }
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
    
    printf("\nRACE CONDITION METRICS:\n");
    printf("=======================\n");
    printf("Invalid POS Reads:    %d/5\n", invalid_pos_reads);
    printf("Invalid LPUS Reads:   %d/5\n", invalid_lpus_reads);
    printf("Total Invalid Reads:  %d/10\n", invalid_pos_reads + invalid_lpus_reads);
    printf("Price Error:          $%.6f\n", fabsf(record.price - expected_serial));
    
    printf("\nVALIDATION SUMMARY:\n");
    printf("===================\n");
    printf("%-30s: ", "Race Conditions Detected");
    printf("%s\n", race_detected ? "YES ✗" : "NO ✓");
    
    printf("%-30s: ", "Price Matches Expected");
    if (fabsf(record.price - expected_serial) < TOLERANCE) {
        printf("YES ✓ (Error: $%.6f)\n", fabsf(record.price - expected_serial));
    } else {
        printf("NO ✗ (Error: $%.6f)\n", fabsf(record.price - expected_serial));
    }
    
    printf("\nRACE CONDITION VERIFICATION:\n");
    printf("=============================\n");
    
    if (race_detected || fabsf(record.price - expected_serial) > TOLERANCE) {
        
        printf("\nEXAMPLE RACE SCENARIO:\n");
        printf("----------------------\n");
        printf("1. POS reads price: $50.00\n");
        printf("2. LPUS reads SAME price: $50.00 (concurrent read)\n");
        printf("3. POS writes discounted price: $45.00\n");
        printf("4. LPUS writes increased price: $55.00 (overwrites discount)\n");
        printf("5. Result: Customer overcharged, discount lost\n");
        printf("6. Expected: $50.00 * 0.9 * 1.1 = $49.50\n");
    } else {
        printf("? No race detected in this execution\n");
        printf("  Note: Race conditions are timing-dependent\n");
        printf("        Run multiple times to observe\n");
    } 
    return 0;
}
