#include <windows.h>
#include <stdio.h>
#include <time.h>

#define PIPE_NAME "\\\\.\\pipe\\PrimeCartPriceUpdates"
#define NUM_ITEMS 1000
#define ITEMS_PER_BATCH 1000

#pragma pack(push, 1)
typedef struct {
    int item_id;
    float price;
    DWORD timestamp;
} PriceUpdate;
#pragma pack(pop)

void lpus_producer() {
    HANDLE hPipe = CreateNamedPipeA(
        PIPE_NAME,
        PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        1,
        65536,
        65536,
        0,
        NULL
    );
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        printf("Failed to create pipe (Error: %lu)\n", GetLastError());
        return;
    }
    
    printf("LPUS: Waiting for POS connection...\n");
    
    // Wait for connection (blocks until consumer connects)
    if (!ConnectNamedPipe(hPipe, NULL)) {
        printf("ConnectNamedPipe failed (Error: %lu)\n", GetLastError());
        CloseHandle(hPipe);
        return;
    }
    
    printf("LPUS: POS Connected! Starting transmission...\n");
    
    PriceUpdate batch[ITEMS_PER_BATCH];
    
    // Use QueryPerformanceCounter for high-resolution timing
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    
    for (int i = 0; i < NUM_ITEMS; i += ITEMS_PER_BATCH) {
        // Generate price updates
        for (int j = 0; j < ITEMS_PER_BATCH; j++) {
            batch[j].item_id = i + j;
            batch[j].price = 10.0f + (rand() % 1000) / 100.0f;
            batch[j].timestamp = GetTickCount();
        }
        
        DWORD bytesWritten;
        if (!WriteFile(hPipe, batch, sizeof(batch), &bytesWritten, NULL)) {
            printf("WriteFile failed (Error: %lu)\n", GetLastError());
            break;
        }
        
        // Force flush to consumer (optional, slows down but more realistic)
        FlushFileBuffers(hPipe);
    }
    
    QueryPerformanceCounter(&end);
    
    // Calculate latency in milliseconds
    double latency = ((double)(end.QuadPart - start.QuadPart) * 1000.0) / frequency.QuadPart;
    
    printf("LPUS: Sent %d updates via Named Pipe\n", NUM_ITEMS);
    printf("Latency for %d updates: %.2f ms (%.4f ms per update)\n", 
           NUM_ITEMS, latency, latency / NUM_ITEMS);
    
    // Signal end of transmission
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
}

void pos_consumer() {
    HANDLE hPipe = CreateFileA(
        PIPE_NAME,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (hPipe == INVALID_HANDLE_VALUE) {
        printf("POS: Failed to connect to pipe (Error: %lu)\n", GetLastError());
        printf("Make sure LPUS producer is running first!\n");
        return;
    }
    
    printf("POS: Connected to LPUS service\n");
    
    PriceUpdate batch[ITEMS_PER_BATCH];
    DWORD bytesRead;
    int totalItems = 0;
    
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    
    // Read until pipe closes
    while (ReadFile(hPipe, batch, sizeof(batch), &bytesRead, NULL) && bytesRead > 0) {
        totalItems += bytesRead / sizeof(PriceUpdate);
        
        // Simulate some processing time (optional)
        // Sleep(1); // 1ms delay to simulate real POS processing
    }
    
    QueryPerformanceCounter(&end);
    double readTime = ((double)(end.QuadPart - start.QuadPart) * 1000.0) / frequency.QuadPart;
    
    printf("POS: Received %d price updates\n", totalItems);
    printf("POS: Reading took %.2f ms (%.4f ms per update)\n", 
           readTime, readTime / totalItems);
    
    // Verify data integrity
    if (totalItems == NUM_ITEMS) {
        printf("POS: ✓ All %d updates received successfully\n", NUM_ITEMS);
    } else {
        printf("POS: ✗ Missing %d updates\n", NUM_ITEMS - totalItems);
    }
    
    CloseHandle(hPipe);
}

int main() {
    printf("========================================\n");
    printf("   PrimeCart Retail IPC Simulation\n");
    printf("========================================\n");
    printf("1. Run LPUS Producer (Price Update Service)\n");
    printf("2. Run POS Consumer (Checkout Terminal)\n");
    printf("Choice: ");
    
    int choice;
    scanf("%d", &choice);
    
    if (choice == 1) {
        printf("\nStarting LPUS Service...\n");
        lpus_producer();
    } else if (choice == 2) {
        printf("\nStarting POS Terminal...\n");
        pos_consumer();
    } else {
        printf("Invalid choice\n");
    }
    
    printf("\nPress Enter to exit...");
    getchar(); getchar(); // Wait for key press
    return 0;
}
