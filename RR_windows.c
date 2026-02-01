// primecart_windows_rr.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

#define CONTEXT_SWITCH_WINDOWS 0.010
#define INTERRUPT_LATENCY_WINDOWS 0.095  
#define SCHEDULING_JITTER_WINDOWS 0.0008 
#define IPC_THROUGHPUT_WINDOWS 850.0    
#define TIME_QUANTUM 5
#define MAX_PROCESSES 7

typedef struct {
    char pid[4];
    char description[50];
    char process_type[12];
    int arrival_time;
    int burst_time;
    int remaining_time;
    int priority;
    int start_time;
    int exit_time;
    int waiting_time;
    int turnaround_time;
    int response_time;
    int completed;
    int context_switches;
} WindowsThread;

WindowsThread threads[] = {
    {"P1", "LPUS Batch Update: SQL DB Write", "Background", 0, 20, 20, 4, -1, 0, 0, 0, -1, 0, 0},
    {"P2", "POS Scan Validation: Barcode Check", "Foreground", 1, 6, 6, 1, -1, 0, 0, 0, -1, 0, 0},
    {"P3", "POS Price Lookup: GUI Display", "Foreground", 2, 4, 4, 1, -1, 0, 0, 0, -1, 0, 0},
    {"P4", "LPUS Inventory Sync: Stock Upload", "Background", 4, 12, 12, 4, -1, 0, 0, 0, -1, 0, 0},
    {"P5", "POS Payment Auth: Data Encryption", "Foreground", 5, 8, 8, 2, -1, 0, 0, 0, -1, 0, 0},
    {"P6", "LPUS Metadata Refresh: Cache Update", "Background", 7, 10, 10, 5, -1, 0, 0, 0, -1, 0, 0},
    {"P7", "POS Receipt Gen: Log Transaction", "Foreground", 9, 4, 4, 1, -1, 0, 0, 0, -1, 0, 0}
};

#define NUM_THREADS (sizeof(threads)/sizeof(threads[0]))

typedef struct Node {
    int thread_index;
    struct Node* next;
} Node;

typedef struct {
    Node* front;
    Node* rear;
    int size;
} Queue;

Queue* create_queue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}

void enqueue(Queue* q, int thread_index) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->thread_index = thread_index;
    new_node->next = NULL;
    
    if (q->rear == NULL) {
        q->front = q->rear = new_node;
    } else {
        q->rear->next = new_node;
        q->rear = new_node;
    }
    q->size++;
}

int dequeue(Queue* q) {
    if (q->front == NULL) return -1;
    
    Node* temp = q->front;
    int thread_index = temp->thread_index;
    q->front = q->front->next;
    
    if (q->front == NULL) {
        q->rear = NULL;
    }
    
    free(temp);
    q->size--;
    return thread_index;
}

int is_queue_empty(Queue* q) {
    return q->front == NULL;
}

void print_gantt_chart(int *gantt_pid, int *gantt_start, int *gantt_end, int gantt_size) {
    printf("\n================================================================================\n");
    printf("GANTT CHART - ROUND ROBIN SCHEDULING (5ms Quantum)\n");
    printf("================================================================================\n\n");
    
    // Print timeline header
    printf("Time(ms) ");
    for (int i = 0; i < gantt_size; i++) {
        printf("|");
        int duration = gantt_end[i] - gantt_start[i];
        int scaled_length = duration;  // Each ms = 1 dash
        
        for (int j = 0; j < scaled_length; j++) {
            printf("-");
        }
    }
    printf("|\n");
    
    // Print process labels - center them in their execution blocks
    printf("         ");
    for (int i = 0; i < gantt_size; i++) {
        printf("|");
        int duration = gantt_end[i] - gantt_start[i];
        int scaled_length = duration;
        
        // Center the PID in the block
        if (scaled_length >= 3) {
            // For blocks >= 3, center the PID
            int padding = (scaled_length - 2) / 2;
            for (int j = 0; j < padding; j++) printf(" ");
            printf("%s", threads[gantt_pid[i]].pid);
            for (int j = 0; j < scaled_length - 2 - padding; j++) printf(" ");
        } else {
            // For small blocks, left align
            printf("%-*s", scaled_length, threads[gantt_pid[i]].pid);
        }
    }
    printf("|\n");
    
    // Print time markers - ONLY at the end of each process execution
    printf("        0");
    int cumulative = 0;
    
    // We'll print time markers at significant points
    for (int i = 0; i < gantt_size; i++) {
        cumulative += (gantt_end[i] - gantt_start[i]);
        
        // Only print time markers at certain intervals or when process ends
        if ((i == gantt_size - 1) || 
            (gantt_end[i] % 10 == 0) || 
            (i < gantt_size - 1 && gantt_pid[i] != gantt_pid[i+1])) {
            
            int duration = gantt_end[i] - gantt_start[i];
            int scaled_length = duration;
            
            // Position the time marker at the end of the block
            printf("%*d", scaled_length + 1, cumulative);
        }
    }
    printf("\n");
    
    // Clean execution sequence without duplicates
    printf("\nExecution Sequence: ");
    char last_pid[4] = "";
    for (int i = 0; i < gantt_size; i++) {
        if (strcmp(threads[gantt_pid[i]].pid, last_pid) != 0) {
            if (i > 0) printf(" -> ");
            printf("%s", threads[gantt_pid[i]].pid);
            strcpy(last_pid, threads[gantt_pid[i]].pid);
        }
    }
    printf("\n");
    
    printf("\nNote: Each '-' character represents 1ms of execution time\n");
    printf("      Context switches (0.010ms) shown as gaps between processes\n");
}
void run_windows_rr_analysis() {
    printf("================================================================================\n");
    printf("PRIMECART RETAIL - WINDOWS POS FRONTEND ROUND ROBIN ANALYSIS\n");
    printf("Windows 10 IoT Enterprise | Preemptive Round Robin (Quantum: 5ms)\n");
    printf("================================================================================\n");
    
    // Print Process Table
    printf("\nPROCESS TABLE (Sorted by Arrival Time):\n");
    printf("+-----+----------------------------------+--------------+----------+----------+----------+\n");
    printf("| PID | Task Description                 | Type         | Arrival  | Burst    | Priority |\n");
    printf("+-----+----------------------------------+--------------+----------+----------+----------+\n");
    
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("| %-3s | %-32s | %-12s | %-8d | %-8d | %-8d |\n",
               threads[i].pid,
               threads[i].description,
               threads[i].process_type,
               threads[i].arrival_time,
               threads[i].burst_time,
               threads[i].priority);
    }
    printf("+-----+----------------------------------+--------------+----------+----------+----------+\n");
    
    // Initialize variables
    Queue* ready_queue = create_queue();
    int current_time = 0;
    int completed_count = 0;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;
    int total_response_time = 0;
    int total_burst_time = 0;
    int total_context_switches = 0;
    int total_idle_time = 0;  // Added to track idle time
    
    // Arrays for Gantt chart
    int gantt_pid[100];
    int gantt_start[100];
    int gantt_end[100];
    int gantt_index = 0;
    
    printf("\nEXECUTION TIMELINE (All times in milliseconds):\n");
    printf("================================================================================\n\n");
    
    // Main scheduling loop
    while (completed_count < NUM_THREADS) {
        // Add newly arrived threads to ready queue
        for (int i = 0; i < NUM_THREADS; i++) {
            if (!threads[i].completed && 
                threads[i].arrival_time <= current_time && 
                threads[i].remaining_time > 0) {
                
                int in_queue = 0;
                Node* current = ready_queue->front;
                while (current != NULL) {
                    if (current->thread_index == i) {
                        in_queue = 1;
                        break;
                    }
                    current = current->next;
                }
                
                if (!in_queue) {
                    enqueue(ready_queue, i);
                }
            }
        }
        
        // If ready queue is empty, advance time
        if (is_queue_empty(ready_queue)) {
            current_time++;
            total_idle_time++;  // Track idle time
            continue;
        }
        
        // Get next thread from ready queue
        int current_thread = dequeue(ready_queue);
        
        // Record start time if this is first execution
        if (threads[current_thread].start_time == -1) {
            threads[current_thread].start_time = current_time;
            threads[current_thread].response_time = current_time - threads[current_thread].arrival_time;
            printf("[Time %dms] %s started (Response Time: %dms)\n", 
                   current_time, threads[current_thread].pid, threads[current_thread].response_time);
        }
        
        // Determine execution time (minimum of quantum and remaining time)
        int execution_time = (threads[current_thread].remaining_time < TIME_QUANTUM) ? 
                             threads[current_thread].remaining_time : TIME_QUANTUM;
        
        // Record Gantt chart entry - FIXED: store actual thread index
        gantt_pid[gantt_index] = current_thread;  // FIX: Removed +1
        gantt_start[gantt_index] = current_time;
        gantt_end[gantt_index] = current_time + execution_time;
        gantt_index++;
        
        // Execute the thread (simulated - don't actually sleep)
        // Sleep(execution_time);  // REMOVED: This causes actual delays
        
        // Update thread state
        threads[current_thread].remaining_time -= execution_time;
        current_time += execution_time;
        
        // Check if completed
        if (threads[current_thread].remaining_time == 0) {
            threads[current_thread].completed = 1;
            threads[current_thread].exit_time = current_time;
            threads[current_thread].turnaround_time = current_time - threads[current_thread].arrival_time;
            threads[current_thread].waiting_time = threads[current_thread].turnaround_time - threads[current_thread].burst_time;
            
            completed_count++;
            
            printf("[Time %dms] %s completed\n", current_time, threads[current_thread].pid);
            printf("      Turnaround: %dms, Waiting: %dms\n\n",
                   threads[current_thread].turnaround_time,
                   threads[current_thread].waiting_time);
            
            total_waiting_time += threads[current_thread].waiting_time;
            total_turnaround_time += threads[current_thread].turnaround_time;
            total_response_time += threads[current_thread].response_time;
            total_burst_time += threads[current_thread].burst_time;
        } else {
            // Thread preempted
            printf("[Time %dms] %s preempted (%dms remaining)\n",
                   current_time, threads[current_thread].pid, threads[current_thread].remaining_time);
            
            // Add to ready queue if not completed
            enqueue(ready_queue, current_thread);
            threads[current_thread].context_switches++;
            total_context_switches++;
        }
        
        // Add context switch overhead
        if (completed_count < NUM_THREADS && !is_queue_empty(ready_queue)) {
            current_time += CONTEXT_SWITCH_WINDOWS;
        }
    }
    
    // Print Gantt Chart
    print_gantt_chart(gantt_pid, gantt_start, gantt_end, gantt_index);
    
    // Calculate averages
    float avg_waiting_time = total_waiting_time / (float)NUM_THREADS;
    float avg_turnaround_time = total_turnaround_time / (float)NUM_THREADS;
    float avg_response_time = total_response_time / (float)NUM_THREADS;
    
    // Calculate CPU utilization
    int total_execution_time = current_time;
    float cpu_utilization = (total_burst_time * 100.0) / total_execution_time;
    
    // Print System-Wide Performance Metrics
    printf("\n================================================================================\n");
    printf("SYSTEM-WIDE PERFORMANCE METRICS (Round Robin Scheduling)\n");
    printf("================================================================================\n");
    
    printf("\nAverage Waiting Time:    %.2f ms\n", avg_waiting_time);
    printf("Average Response Time:   %.2f ms\n", avg_response_time);
    printf("Average Turnaround Time: %.2f ms\n", avg_turnaround_time);
    printf("CPU Utilization:         %.1f%%\n", cpu_utilization);
    printf("Total Context Switches:  %d\n", total_context_switches);
    printf("Context Switch Overhead: %.3f ms\n", total_context_switches * CONTEXT_SWITCH_WINDOWS);
    printf("Total Execution Time:    %d ms\n", total_execution_time);
    printf("Total CPU Busy Time:     %d ms\n", total_burst_time);
    printf("Total Idle Time:         %d ms\n", total_idle_time);
    
    // Performance Analysis Table
    printf("\n================================================================================\n");
    printf("PROCESS PERFORMANCE METRICS (Round Robin Execution)\n");
    printf("================================================================================\n");
    
    printf("\n+-----+----------+----------+----------+----------+----------+------------+------------+\n");
    printf("| PID | Arrival  | Burst    | Start    | Exit     | Response | Wait       | Turnaround |\n");
    printf("+-----+----------+----------+----------+----------+----------+------------+------------+\n");
    
    // Print in execution order
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("| %-3s | %-8d | %-8d | %-8d | %-8d | %-8d | %-10d | %-10d |\n",
               threads[i].pid,
               threads[i].arrival_time,
               threads[i].burst_time,
               threads[i].start_time,
               threads[i].exit_time,
               threads[i].response_time,
               threads[i].waiting_time,
               threads[i].turnaround_time);
    }
    printf("+-----+----------+----------+----------+----------+----------+------------+------------+\n");
    
       // PrimeCart Threshold Analysis - FIXED table formatting
    printf("\n================================================================================\n");
    printf("PRIMECART THRESHOLD ANALYSIS (Windows 10 IoT Enterprise - Round Robin)\n");
    printf("================================================================================\n");
    printf("Metric                 Target Threshold    Windows Value   Status   Why It Matters for PrimeCart\n");
    printf("--------------------------------------------------------------------------------\n");
    
    // Interrupt Latency
    const char* int_status = INTERRUPT_LATENCY_WINDOWS < 0.100 ? "PASS" : "FAIL";
    printf("Interrupt Latency      < 0.100 ms          %-12s %-6s   Ensures barcode scanner is detected immediately\n", 
           "0.095 ms", int_status);
    
    // Scheduling Jitter
    const char* jitter_status = SCHEDULING_JITTER_WINDOWS < 2.000 ? "PASS" : "FAIL";
    printf("Scheduling Jitter      < 2.000 ms          %-12s %-6s   Prevents cashier screen stuttering\n", 
           "0.001 ms", jitter_status);
    
    // Context Switch Time
    const char* cs_status = CONTEXT_SWITCH_WINDOWS <= 0.010 ? "PASS" : "FAIL";
    printf("Context Switch Time    < 0.010 ms          %-12s %-6s   Maintains smooth task switching during checkout.\n", 
           "0.010 ms", cs_status);
    
    // CPU Utilization - ADDED MISSING METRIC
    const char* cpu_status = cpu_utilization < 75.0 ? "PASS" : "FAIL";
    char cpu_value[20];
    sprintf(cpu_value, "%.1f%%", cpu_utilization);
    printf("CPU Utilization        < 75.0%%             %-12s %-6s   Leaves insufficient headroom for peak traffic\n", 
           cpu_value, cpu_status);  // ADDED THIS LINE
       
    // IPC Throughput
    const char* ipc_status = IPC_THROUGHPUT_WINDOWS > 500.0 ? "PASS" : "FAIL";
    char ipc_value[20];
    sprintf(ipc_value, "%.0f MB/s", IPC_THROUGHPUT_WINDOWS);
    printf("IPC Throughput         > 500 MB/s          %-12s %-6s   Enables fast communication between POS and backend systems\n", 
           ipc_value, ipc_status);
    
    printf("--------------------------------------------------------------------------------\n");
    
    printf("--------------------------------------------------------------------------------\n");
    
    // Round Robin Algorithm Analysis
    printf("\n================================================================================\n");
    printf("ROUND ROBIN ALGORITHM ANALYSIS\n");
    printf("================================================================================\n");
    
    printf("\nRR Selection Logic:\n");
    printf("• Each thread gets 5ms CPU time (quantum)\n");
    printf("• Preempted threads go to back of ready queue\n");
    printf("• New arrivals added to end of queue\n");
    printf("• Ensures fair CPU allocation for all threads\n");
    
    printf("\nImpact on PrimeCart Operations:\n");
    printf("✓ POS threads (P2, P3, P5, P7) get quick initial response\n");
    printf("✓ No thread starves - all make progress\n");
    printf("✓ Background tasks (P1, P4, P6) don't block POS operations\n");
    printf("✓ Predictable worst-case response time\n");
    printf("✗ Higher context switch overhead due to frequent preemptions\n");
    printf("✗ Throughput slightly lower than SJF\n");
        
    printf("\n================================================================================\n");
    printf("ANALYSIS COMPLETE\n");
    printf("================================================================================\n");
    
    free(ready_queue);
}

int main() {
    run_windows_rr_analysis();
    return 0;
}
