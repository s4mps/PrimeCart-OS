// Linux_FIFO_IPC.c
// LPUS Service (P1) -> FIFO -> POS Terminal (P2)
// OVERHEAD: File I/O + kernel buffering + double data copying

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>

#define FIFO_NAME "/tmp/primecart_fifo"
#define NUM_ITEMS 1000
#define ITEMS_PER_BATCH 1000

typedef struct {
    int item_id;
    float price;
    time_t timestamp;
} PriceUpdate;

// LPUS Service - Producer
void lpus_producer() {
    printf("\nStarting LPUS Service...\n");
    
    // Create the FIFO (named pipe)
    mkfifo(FIFO_NAME, 0666);
    
    printf("LPUS: Waiting for POS connection...\n");
    int fd = open(FIFO_NAME, O_WRONLY);
    
    if (fd < 0) {
        perror("LPUS: Failed to open FIFO");
        return;
    }
    
    PriceUpdate batch[ITEMS_PER_BATCH];
    
    // Get start time
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    for (int i = 0; i < NUM_ITEMS; i += ITEMS_PER_BATCH) {
        // Generate price updates
        for (int j = 0; j < ITEMS_PER_BATCH; j++) {
            batch[j].item_id = i + j + 1000;
            batch[j].price = 10.0f + (rand() % 1000) / 100.0f;
            batch[j].timestamp = time(NULL);
        }
        
        // Write to FIFO - data copied to kernel buffer
        if (write(fd, batch, sizeof(batch)) < 0) {
            perror("LPUS: Write failed");
            break;
        }
        fsync(fd);  // Force data to consumer
    }
    
    gettimeofday(&end, NULL);
    
    // Calculate latency in milliseconds
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double latency = seconds * 1000.0 + microseconds / 1000.0;
    
    printf("LPUS: Sent %d updates via FIFO\n", NUM_ITEMS);
    printf("Latency for %d updates: %.2f ms (%.4f ms per update)\n", 
           NUM_ITEMS, latency, latency / NUM_ITEMS);
    
    close(fd);
    unlink(FIFO_NAME);
    
    printf("\nPress Enter to exit...\n");
    getchar();
}

// POS Terminal - Consumer
void pos_consumer() {
    printf("\nStarting POS Terminal...\n");
    
    // Open FIFO for reading
    printf("POS: Connecting to LPUS...\n");
    int fd = open(FIFO_NAME, O_RDONLY);
    
    if (fd < 0) {
        perror("POS: Failed to open FIFO");
        printf("     Make sure LPUS producer is running first!\n");
        return;
    }
    
    printf("POS: Connected to LPUS service\n");
    
    PriceUpdate batch[ITEMS_PER_BATCH];
    ssize_t bytesRead;
    int totalItems = 0;
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // Read from FIFO until EOF
    while ((bytesRead = read(fd, batch, sizeof(batch))) > 0) {
        totalItems += bytesRead / sizeof(PriceUpdate);
    }
    
    gettimeofday(&end, NULL);
    
    // Calculate reading time
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double readTime = seconds * 1000.0 + microseconds / 1000.0;
    
    printf("POS: Received %d price updates\n", totalItems);
    printf("POS: Reading took %.2f ms (%.4f ms per update)\n", 
           readTime, readTime / totalItems);
    
    if (totalItems == NUM_ITEMS) {
        printf("POS: All %d updates received successfully\n", NUM_ITEMS);
    }
    
    close(fd);
    
    printf("\nPress Enter to exit...\n");
    getchar();
}

int main() {
    printf("========================================\n");
    printf("   PrimeCart Linux FIFO IPC (Traditional)\n");
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
    
    srand(time(NULL));
    
    if (choice == 1) {
        lpus_producer();
    } else if (choice == 2) {
        pos_consumer();
    } else {
        printf("Invalid choice\n");
    }
    
    return 0;
}
