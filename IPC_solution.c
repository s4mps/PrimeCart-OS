// Windows_SharedMemory_IPC_Fixed.c
// LPUS Service (P1) ⇄ Shared Memory (RAM) ⇄ POS Terminal (P2)
// ZERO-COPY: Both processes access same physical memory via mapped views
// FIXED: Proper synchronization and output format

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define SHM_NAME "PrimeCartPriceMemory"
#define NUM_ITEMS 1000
#define BUFFER_SIZE (sizeof(PriceUpdate) * NUM_ITEMS)

#pragma pack(push, 1)
typedef struct {
    int item_id;
    float price;
    DWORD timestamp;
    volatile LONG is_updated;
} PriceUpdate;
#pragma pack(pop)

// LPUS Service - Producer
void lpus_producer() {
    printf("\nStarting LPUS Service...\n");
    
    // Create shared memory
    HANDLE hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        BUFFER_SIZE,
        SHM_NAME
    );
    
    if (!hMapFile) {
        printf("LPUS: Failed to create shared memory (Error: %lu)\n", GetLastError());
        return;
    }
    
    PriceUpdate* shared_data = (PriceUpdate*)MapViewOfFile(
        hMapFile,
        FILE_MAP_WRITE,
        0, 0,
        BUFFER_SIZE
    );
    
    if (!shared_data) {
        printf("LPUS: Failed to map view\n");
        CloseHandle(hMapFile);
        return;
    }
    
    printf("LPUS: Waiting for POS connection...\n");
    
    // Create an event to signal when POS connects
    HANDLE hEvent = CreateEventA(NULL, FALSE, FALSE, "PrimeCartReadyEvent");
    if (hEvent) {
        // Wait a reasonable time for POS to connect
        if (WaitForSingleObject(hEvent, 10000) == WAIT_OBJECT_0) {
            printf("LPUS: POS Connected! Starting transmission...\n");
        } else {
            printf("LPUS: Timeout waiting for POS. Proceeding anyway...\n");
        }
    }
    
    // Clear the shared memory
    ZeroMemory(shared_data, BUFFER_SIZE);
    
    // Use QueryPerformanceCounter for better timing
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    
    // Write data to shared memory
    for (int i = 0; i < NUM_ITEMS; i++) {
        shared_data[i].item_id = i + 1000;
        shared_data[i].price = 10.0f + (rand() % 1000) / 100.0f;
        shared_data[i].timestamp = GetTickCount();
        shared_data[i].is_updated = 1;
    }
    
    QueryPerformanceCounter(&end);
    double latency = ((double)(end.QuadPart - start.QuadPart) * 1000.0) / frequency.QuadPart;
    
    printf("LPUS: Wrote %d updates via Shared Memory\n", NUM_ITEMS);
    printf("Latency for %d updates: %.2f ms (%.4f ms per update)\n", 
           NUM_ITEMS, latency, latency / NUM_ITEMS);
    
    printf("\nLPUS: Data ready in shared memory. Waiting for POS to read...\n");
    printf("      (Shared memory will remain available for 5 seconds)\n");
    
    // Wait for POS to read (with timeout)
    Sleep(5000);
    
    if (hEvent) CloseHandle(hEvent);
    UnmapViewOfFile(shared_data);
    CloseHandle(hMapFile);
    
    printf("\nPress Enter to exit...\n");
    getchar(); getchar();
}

// POS Terminal - Consumer
void pos_consumer() {
    printf("\nStarting POS Terminal...\n");
    
    // Signal LPUS that we're connecting
    HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "PrimeCartReadyEvent");
    if (hEvent) {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }
    
    // Small delay to ensure LPUS is ready
    Sleep(100);
    
    // Open shared memory
    HANDLE hMapFile = OpenFileMappingA(
        FILE_MAP_READ,
        FALSE,
        SHM_NAME
    );
    
    if (!hMapFile) {
        printf("POS: Failed to open shared memory (Error: %lu)\n", GetLastError());
        printf("     Make sure LPUS producer is running first!\n");
        return;
    }
    
    printf("POS: Connected to shared memory\n");
    
    PriceUpdate* shared_data = (PriceUpdate*)MapViewOfFile(
        hMapFile,
        FILE_MAP_READ,
        0, 0,
        BUFFER_SIZE
    );
    
    if (!shared_data) {
        printf("POS: Failed to map view\n");
        CloseHandle(hMapFile);
        return;
    }
    
    // Time the reading process
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    
    int updates_received = 0;
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (shared_data[i].is_updated) {
            updates_received++;
        }
    }
    
    QueryPerformanceCounter(&end);
    double readTime = ((double)(end.QuadPart - start.QuadPart) * 1000.0) / frequency.QuadPart;
    
    printf("POS: Received %d price updates\n", updates_received);
    printf("POS: Reading took %.2f ms (%.4f ms per update)\n", 
           readTime, readTime / updates_received);
    
    if (updates_received == NUM_ITEMS) {
        printf("POS: All %d updates received successfully\n", NUM_ITEMS);
    }
    
    printf("\nZERO-COPY ADVANTAGE: Both processes access same physical RAM\n");
    
    UnmapViewOfFile(shared_data);
    CloseHandle(hMapFile);
    
    printf("\nPress Enter to exit...\n");
    getchar(); getchar();
}

int main() {
    printf("========================================\n");
    printf("   PrimeCart Shared Memory IPC (Zero-Copy)\n");
    printf("========================================\n");
    printf("WARNING: Run in separate terminals\n");
    printf("         1. First run LPUS Producer\n");
    printf("         2. Then run POS Consumer\n");
    printf("\nSelect mode:\n");
    printf("1. LPUS Producer (Price Update Service)\n");
    printf("2. POS Consumer (Checkout Terminal)\n");
    printf("Choice: ");
    
    int choice;
    scanf("%d", &choice);
    
    // Clear input buffer
    while (getchar() != '\n');
    
    srand((unsigned)time(NULL));
    
    if (choice == 1) {
        lpus_producer();
    } else if (choice == 2) {
        pos_consumer();
    } else {
        printf("Invalid choice\n");
    }
    
    return 0;
}
