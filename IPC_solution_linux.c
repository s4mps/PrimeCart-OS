// Linux_SharedMemory_IPC.c
// LPUS Service (P1) ⇄ /dev/shm (RAM filesystem) ⇄ POS Terminal (P2)
// ZERO-COPY: mmap() creates direct virtual→physical mapping

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <errno.h>

#define SHM_NAME "/primecart_shm"
#define SEM_NAME "/primecart_sem"
#define NUM_ITEMS 1000
#define BUFFER_SIZE (sizeof(PriceUpdate) * NUM_ITEMS)

typedef struct {
    int item_id;
    float price;
    time_t timestamp;
    volatile int is_updated;
} PriceUpdate;

// LPUS Service - Producer
void lpus_producer() {
    printf("\nStarting LPUS Service...\n");
    
    // Create shared memory object in /dev/shm (RAM filesystem)
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd < 0) {
        perror("LPUS: shm_open failed");
        return;
    }
    
    // Set the size of shared memory
    if (ftruncate(shm_fd, BUFFER_SIZE) < 0) {
        perror("LPUS: ftruncate failed");
        close(shm_fd);
        return;
    }
    
    // Map shared memory into process address space
    PriceUpdate* shared_data = (PriceUpdate*)mmap(
        NULL,
        BUFFER_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );
    
    if (shared_data == MAP_FAILED) {
        perror("LPUS: mmap failed");
        close(shm_fd);
        return;
    }
    
    printf("LPUS: Shared memory created at %p\n", (void*)shared_data);
    printf("LPUS: Using /dev/shm (tmpfs) - no disk I/O\n");
    printf("LPUS: Waiting for POS connection...\n");
    
    // Create semaphore for synchronization
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    if (sem == SEM_FAILED) {
        perror("LPUS: sem_open failed");
    } else {
        printf("LPUS: Synchronization semaphore created\n");
    }
    
    // Initialize shared memory to zeros
    memset(shared_data, 0, BUFFER_SIZE);
    
    // Wait for POS to signal readiness
    printf("LPUS: Waiting for POS signal...\n");
    if (sem != SEM_FAILED) {
        // Wait for POS to post the semaphore
        if (sem_wait(sem) == 0) {
            printf("LPUS: POS Connected! Starting transmission...\n");
        }
    } else {
        printf("LPUS: Proceeding without synchronization...\n");
        sleep(1);  // Give POS time to connect
    }
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // Write data directly to shared memory
    printf("LPUS: Writing price updates...\n");
    for (int i = 0; i < NUM_ITEMS; i++) {
        shared_data[i].item_id = i + 1000;
        shared_data[i].price = 10.0f + (rand() % 1000) / 100.0f;
        shared_data[i].timestamp = time(NULL);
        shared_data[i].is_updated = 1;
        
        // Memory barrier for cache coherence
        __sync_synchronize();
    }
    
    gettimeofday(&end, NULL);
    
    // Calculate latency
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double latency = seconds * 1000.0 + microseconds / 1000.0;
    
    printf("LPUS: Wrote %d updates via Shared Memory\n", NUM_ITEMS);
    printf("Latency for %d updates: %.2f ms (%.4f ms per update)\n", 
           NUM_ITEMS, latency, latency / NUM_ITEMS);
    
    printf("\nLPUS: Data ready. Waiting for POS to read...\n");
    sleep(5);  // Wait for POS to read
    
    // Cleanup
    munmap(shared_data, BUFFER_SIZE);
    close(shm_fd);
    if (sem != SEM_FAILED) {
        sem_close(sem);
        sem_unlink(SEM_NAME);
    }
    shm_unlink(SHM_NAME);
    
    printf("LPUS: Shared memory resources released\n");
    printf("\nPress Enter to exit...\n");
    getchar();
}

// POS Terminal - Consumer
void pos_consumer() {
    printf("\nStarting POS Terminal...\n");
    
    // Signal LPUS that we're ready
    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem != SEM_FAILED) {
        sem_post(sem);
        printf("POS: Signaled LPUS we're ready\n");
    }
    
    // Small delay to ensure LPUS has created shared memory
    sleep(1);
    
    // Open existing shared memory
    int shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd < 0) {
        perror("POS: shm_open failed");
        printf("     Make sure LPUS producer is running first!\n");
        return;
    }
    
    // Map shared memory into our address space
    PriceUpdate* shared_data = (PriceUpdate*)mmap(
        NULL,
        BUFFER_SIZE,
        PROT_READ,
        MAP_SHARED,
        shm_fd,
        0
    );
    
    if (shared_data == MAP_FAILED) {
        perror("POS: mmap failed");
        close(shm_fd);
        return;
    }
    
    printf("POS: Connected to shared memory at %p\n", (void*)shared_data);
    printf("POS: Same physical RAM as LPUS (zero-copy)\n");
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // Read data from shared memory
    int updates_received = 0;
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (shared_data[i].is_updated) {
            updates_received++;
        }
    }
    
    gettimeofday(&end, NULL);
    
    // Calculate reading time
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double readTime = seconds * 1000.0 + microseconds / 1000.0;
    
    printf("POS: Received %d price updates\n", updates_received);
    printf("POS: Reading took %.2f ms (%.4f ms per update)\n", 
           readTime, readTime / updates_received);
    
    if (updates_received == NUM_ITEMS) {
        printf("POS: All %d updates received successfully\n", NUM_ITEMS);
    }
    
    // Display sample data
    if (updates_received > 0) {
        printf("\nPOS: Sample data (first 3 items):\n");
        for (int i = 0; i < 3 && i < updates_received; i++) {
            printf("  Item %d: $%.2f (Updated: %ld)\n",
                   shared_data[i].item_id,
                   shared_data[i].price,
                   shared_data[i].timestamp);
        }
    }
    
    // Cleanup
    munmap(shared_data, BUFFER_SIZE);
    close(shm_fd);
    if (sem != SEM_FAILED) {
        sem_close(sem);
    }
    
    printf("\nPress Enter to exit...\n");
    getchar();
}

int main() {
    printf("==============================================\n");
    printf("   PrimeCart Linux Shared Memory IPC (Zero-Copy)\n");
    printf("==============================================\n");
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
