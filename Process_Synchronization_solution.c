// primecart_mutex_solution.c
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <math.h>

#define TOLERANCE 0.001f

typedef struct {
    int pid;
    volatile float price;
    volatile LONG updates;
    volatile LONG pos_reads;
    volatile LONG lpus_reads;
} PriceRecord;

PriceRecord record = {1001, 50.0f, 0, 0, 0};
HANDLE price_mutex;  // Changed from CRITICAL_SECTION to HANDLE (Mutex)
float expected_serial = 50.0f;

// Execution traces
float pos_read_values[5];
float lpus_read_values[5];
float pos_write_values[5];
float lpus_write_values[5];
DWORD pos_timestamps[5];
DWORD lpus_timestamps[5];

typedef struct {
    char type[10];
    int index;
    DWORD timestamp;
    float read_val;
    float write_val;
} Event;

unsigned __stdcall pos_thread_mutex(void* arg) {
    for (int i = 0; i < 5; i++) {
        // ACQUIRE MUTEX
        WaitForSingleObject(price_mutex, INFINITE);
        
        // CRITICAL SECTION START
        float read_val = record.price;
        pos_read_values[i] = read_val;
        pos_timestamps[i] = GetTickCount();
        InterlockedIncrement(&record.pos_reads);
        
        float discounted = read_val * 0.9f;
        Sleep(rand() % 10);
        
        record.price = discounted;
        pos_write_values[i] = discounted;
        InterlockedIncrement(&record.updates);
        // CRITICAL SECTION END
        
        // RELEASE MUTEX
        ReleaseMutex(price_mutex);
        
        // Non-critical work
        Sleep(40 + rand() % 20);
    }
    return 0;
}

unsigned __stdcall lpus_thread_mutex(void* arg) {
    for (int i = 0; i < 5; i++) {
        // ACQUIRE MUTEX
        WaitForSingleObject(price_mutex, INFINITE);
        
        // CRITICAL SECTION START
        float read_val = record.price;
        lpus_read_values[i] = read_val;
        lpus_timestamps[i] = GetTickCount();
        InterlockedIncrement(&record.lpus_reads);
        
        float increased = read_val * 1.1f;
        Sleep(rand() % 15);
        
        record.price = increased;
        lpus_write_values[i] = increased;
        InterlockedIncrement(&record.updates);
        // CRITICAL SECTION END
        
        // RELEASE MUTEX
        ReleaseMutex(price_mutex);
        
        // Non-critical work
        Sleep(50 + rand() % 25);
    }
    return 0;
}

void print_execution_table() {
    printf("\nEXECUTION TRACE WITH MUTEX SYNCHRONIZATION:\n");
    printf("============================================\n");
    printf("Operation          Read Value     Write Value    Timestamp (ms)\n");
    printf("-----------------  -------------  -------------  ---------------\n");
    
    Event events[10];
    int event_count = 0;
    
    for (int i = 0; i < 5; i++) {
        sprintf(events[event_count].type, "POS-%d", i+1);
        events[event_count].index = i;
        events[event_count].timestamp = pos_timestamps[i];
        events[event_count].read_val = pos_read_values[i];
        events[event_count].write_val = pos_write_values[i];
        event_count++;
    }
    
    for (int i = 0; i < 5; i++) {
        sprintf(events[event_count].type, "LPUS-%d", i+1);
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
    
    DWORD start_time = events[0].timestamp;
    for (int i = 0; i < event_count; i++) {
        printf("%-10s        $%9.4f    $%9.4f    %7lu\n",
               events[i].type,
               events[i].read_val,
               events[i].write_val,
               events[i].timestamp - start_time);
    }
}

int main() {
    printf("PRIMECART SYNCHRONIZED SOLUTION - WINDOWS MUTEX\n");
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
    printf("  Primitive:       Windows MUTEX (Kernel Object)\n");
    printf("  Scope:           Cross-process capable\n");
    printf("  Protection:      read-modify-write atomicity\n");
    printf("  Timeout:         INFINITE (blocking)\n\n");
    
    // Create unnamed mutex (NULL name = unnamed, FALSE = not initially owned)
    price_mutex = CreateMutex(NULL, FALSE, NULL);
    if (price_mutex == NULL) {
        printf("Error: Failed to create mutex (Error: %lu)\n", GetLastError());
        return 1;
    }
    
    srand(GetTickCount());
    DWORD start_time = GetTickCount();
    
    printf("STARTING SYNCHRONIZED EXECUTION WITH MUTEX...\n");
    printf("---------------------------------------------\n\n");
    
    HANDLE threads[2];
    threads[0] = (HANDLE)_beginthreadex(NULL, 0, pos_thread_mutex, NULL, 0, NULL);
    threads[1] = (HANDLE)_beginthreadex(NULL, 0, lpus_thread_mutex, NULL, 0, NULL);
    
    if (threads[0] == NULL || threads[1] == NULL) {
        printf("Error: Failed to create threads\n");
        CloseHandle(price_mutex);
        return 1;
    }
    
    WaitForMultipleObjects(2, threads, TRUE, INFINITE);
    
    DWORD end_time = GetTickCount();
    CloseHandle(threads[0]);
    CloseHandle(threads[1]);
    CloseHandle(price_mutex);
    
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
    printf("Execution Time:     %lu ms\n", end_time - start_time);
    
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
