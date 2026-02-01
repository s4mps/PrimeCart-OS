// primecart_linux_rr.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define CONTEXT_SWITCH_LINUX 0.004
#define INTERRUPT_LATENCY_LINUX 0.075  
#define SCHEDULING_JITTER_LINUX 0.0005 
#define IPC_THROUGHPUT_LINUX 950.0    
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
} LinuxProcess;

LinuxProcess processes[] = {
    {"P1", "LPUS Batch Update: SQL DB Write", "Background", 0, 20, 20, 4, -1, 0, 0, 0, -1, 0, 0},
    {"P2", "POS Scan Validation: Barcode Check", "Foreground", 1, 6, 6, 1, -1, 0, 0, 0, -1, 0, 0},
    {"P3", "POS Price Lookup: GUI Display", "Foreground", 2, 4, 4, 1, -1, 0, 0, 0, -1, 0, 0},
    {"P4", "LPUS Inventory Sync: Stock Upload", "Background", 4, 12, 12, 4, -1, 0, 0, 0, -1, 0, 0},
    {"P5", "POS Payment Auth: Data Encryption", "Foreground", 5, 8, 8, 2, -1, 0, 0, 0, -1, 0, 0},
    {"P6", "LPUS Metadata Refresh: Cache Update", "Background", 7, 10, 10, 5, -1, 0, 0, 0, -1, 0, 0},
    {"P7", "POS Receipt Gen: Log Transaction", "Foreground", 9, 4, 4, 1, -1, 0, 0, 0, -1, 0, 0}
};

#define NUM_PROCESSES (sizeof(processes)/sizeof(processes[0]))

typedef struct Node {
    int process_index;
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

void enqueue(Queue* q, int process_index) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->process_index = process_index;
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
    int process_index = temp->process_index;
    q->front = q->front->next;
    
    if (q->front == NULL) {
        q->rear = NULL;
    }
    
    free(temp);
    q->size--;
    return process_index;
}

int is_queue_empty(Queue* q) {
    return q->front == NULL;
}

void print_gantt_chart(int *gantt_pid, int *gantt_start, int *gantt_end, int gantt_size) {
    printf("\n================================================================================\n");
    printf("GANTT CHART - ROUND ROBIN SCHEDULING (5ms Quantum)\n");
    printf("================================================================================\n\n");
    
    // First group consecutive executions of same process
    int grouped_pid[50];
    int grouped_start[50];
    int grouped_end[50];
    int grouped_size = 0;
    
    for (int i = 0; i < gantt_size; i++) {
        if (i == 0 || gantt_pid[i] != gantt_pid[i-1]) {
            grouped_pid[grouped_size] = gantt_pid[i];
            grouped_start[grouped_size] = gantt_start[i];
            grouped_end[grouped_size] = gantt_end[i];
            grouped_size++;
        } else {
            grouped_end[grouped_size-1] = gantt_end[i];
        }
    }
    
    // Print timeline header
    printf("Time(ms) ");
    for (int i = 0; i < grouped_size; i++) {
        printf("|");
        int duration = grouped_end[i] - grouped_start[i];
        int scaled_length = duration;  // Scale factor
        
        for (int j = 0; j < scaled_length; j++) {
            printf("-");
        }
    }
    printf("|\n");
    
    // Print process labels
    printf("         ");
    for (int i = 0; i < grouped_size; i++) {
        printf("|");
        int duration = grouped_end[i] - grouped_start[i];
        int scaled_length = duration;
        
        // Center PID in block
        if (scaled_length >= 3) {
            int padding = (scaled_length - 2) / 2;
            for (int j = 0; j < padding; j++) printf(" ");
            printf("%s", processes[grouped_pid[i]].pid);
            for (int j = 0; j < scaled_length - 2 - padding; j++) printf(" ");
        } else {
            printf("%-*s", scaled_length, processes[grouped_pid[i]].pid);
        }
    }
    printf("|\n");
    
    // Print time markers
    printf("        0");
    int cumulative = 0;
    for (int i = 0; i < grouped_size; i++) {
        cumulative += (grouped_end[i] - grouped_start[i]);
        int duration = grouped_end[i] - grouped_start[i];
        int scaled_length = duration;
        
        int spacing = scaled_length + 1;
        printf("%*d", spacing, cumulative);
    }
    printf("\n");
    
    // Clean execution sequence
    printf("\nExecution Sequence: ");
    for (int i = 0; i < grouped_size; i++) {
        printf("%s", processes[grouped_pid[i]].pid);
        if (i < grouped_size - 1) {
            printf(" -> ");
        }
    }
    printf("\n");
    
    printf("\nNote: Each '-' character represents 1ms of execution time\n");
    printf("      Consecutive executions grouped together\n");
}

void run_linux_rr_analysis() {
    printf("================================================================================\n");
    printf("PRIMECART RETAIL - LINUX LPUS BACKEND ROUND ROBIN ANALYSIS\n");
    printf("Ubuntu 22.04 LTS Server | Preemptive Round Robin (Quantum: 5ms)\n");
    printf("================================================================================\n");
    
    // Print Process Table
    printf("\nPROCESS TABLE (Sorted by Arrival Time):\n");
    printf("+-----+----------------------------------+--------------+----------+----------+----------+\n");
    printf("| PID | Task Description                 | Type         | Arrival  | Burst    | Priority |\n");
    printf("+-----+----------------------------------+--------------+----------+----------+----------+\n");
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("| %-3s | %-32s | %-12s | %-8d | %-8d | %-8d |\n",
               processes[i].pid,
               processes[i].description,
               processes[i].process_type,
               processes[i].arrival_time,
               processes[i].burst_time,
               processes[i].priority);
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
    int total_idle_time = 0;
    
    // Arrays for Gantt chart
    int gantt_pid[100];
    int gantt_start[100];
    int gantt_end[100];
    int gantt_index = 0;
    
    printf("\nEXECUTION TIMELINE (All times in milliseconds):\n");
    printf("================================================================================\n\n");
    
    // Main scheduling loop
    while (completed_count < NUM_PROCESSES) {
        // Add newly arrived processes to ready queue
        for (int i = 0; i < NUM_PROCESSES; i++) {
            if (!processes[i].completed && 
                processes[i].arrival_time <= current_time && 
                processes[i].remaining_time > 0) {
                
                int in_queue = 0;
                Node* current = ready_queue->front;
                while (current != NULL) {
                    if (current->process_index == i) {
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
            total_idle_time++;
            continue;
        }
        
        // Get next process from ready queue
        int current_process = dequeue(ready_queue);
        
        // Record start time if this is first execution
        if (processes[current_process].start_time == -1) {
            processes[current_process].start_time = current_time;
            processes[current_process].response_time = current_time - processes[current_process].arrival_time;
            printf("[Time %dms] %s started (Response Time: %dms)\n", 
                   current_time, processes[current_process].pid, processes[current_process].response_time);
        }
        
        // Determine execution time
        int execution_time = (processes[current_process].remaining_time < TIME_QUANTUM) ? 
                             processes[current_process].remaining_time : TIME_QUANTUM;
        
        // Record Gantt chart entry - FIXED: no +1
        gantt_pid[gantt_index] = current_process;
        gantt_start[gantt_index] = current_time;
        gantt_end[gantt_index] = current_time + execution_time;
        gantt_index++;
        
        // Update process state (simulated execution)
        processes[current_process].remaining_time -= execution_time;
        current_time += execution_time;
        
        // Check if completed
        if (processes[current_process].remaining_time == 0) {
            processes[current_process].completed = 1;
            processes[current_process].exit_time = current_time;
            processes[current_process].turnaround_time = current_time - processes[current_process].arrival_time;
            processes[current_process].waiting_time = processes[current_process].turnaround_time - processes[current_process].burst_time;
            
            completed_count++;
            
            printf("[Time %dms] %s completed\n", current_time, processes[current_process].pid);
            printf("      Turnaround: %dms, Waiting: %dms\n\n",
                   processes[current_process].turnaround_time,
                   processes[current_process].waiting_time);
            
            total_waiting_time += processes[current_process].waiting_time;
            total_turnaround_time += processes[current_process].turnaround_time;
            total_response_time += processes[current_process].response_time;
            total_burst_time += processes[current_process].burst_time;
        } else {
            // Process preempted
            printf("[Time %dms] %s preempted (%dms remaining)\n",
                   current_time, processes[current_process].pid, processes[current_process].remaining_time);
            
            enqueue(ready_queue, current_process);
            processes[current_process].context_switches++;
            total_context_switches++;
        }
        
        // Add context switch overhead
        if (completed_count < NUM_PROCESSES && !is_queue_empty(ready_queue)) {
            current_time += CONTEXT_SWITCH_LINUX;
        }
    }
    
    // Print Gantt Chart
    print_gantt_chart(gantt_pid, gantt_start, gantt_end, gantt_index);
    
    // Calculate averages
    float avg_waiting_time = total_waiting_time / (float)NUM_PROCESSES;
    float avg_turnaround_time = total_turnaround_time / (float)NUM_PROCESSES;
    float avg_response_time = total_response_time / (float)NUM_PROCESSES;
    
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
    printf("Context Switch Overhead: %.3f ms\n", total_context_switches * CONTEXT_SWITCH_LINUX);
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
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("| %-3s | %-8d | %-8d | %-8d | %-8d | %-8d | %-10d | %-10d |\n",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].burst_time,
               processes[i].start_time,
               processes[i].exit_time,
               processes[i].response_time,
               processes[i].waiting_time,
               processes[i].turnaround_time);
    }
    printf("+-----+----------+----------+----------+----------+----------+------------+------------+\n");
    
    // PrimeCart Threshold Analysis - FIXED FORMAT
    printf("\n================================================================================\n");
    printf("PRIMECART THRESHOLD ANALYSIS (Ubuntu 22.04 LTS Server - Round Robin)\n");
    printf("================================================================================\n");
    printf("Metric                 Target Threshold    Linux Value     Status   Why It Matters for PrimeCart\n");
    printf("--------------------------------------------------------------------------------\n");
    
    // Interrupt Latency
    const char* int_status = INTERRUPT_LATENCY_LINUX < 0.100 ? "PASS" : "FAIL";
    printf("Interrupt Latency      < 0.100 ms          %-12s %-6s   Faster interrupt handling for POS devices\n", 
           "0.075 ms", int_status);
    
    // Scheduling Jitter
    const char* jitter_status = SCHEDULING_JITTER_LINUX < 2.000 ? "PASS" : "FAIL";
    printf("Scheduling Jitter      < 2.000 ms          %-12s %-6s   Stable backend processing\n", 
           "0.001 ms", jitter_status);
    
    // Context Switch Time
    const char* cs_status = CONTEXT_SWITCH_LINUX <= 0.010 ? "PASS" : "FAIL";
    printf("Context Switch Time    < 0.010 ms          %-12s %-6s   Efficient context switching keeps POS transactions responsive under load.\n", 
           "0.004 ms", cs_status);
    
    // CPU Utilization - FIXED: Changed from >90.0% to <75.0% for consistency
    const char* cpu_status = cpu_utilization < 75.0 ? "PASS" : "FAIL";
    char cpu_value[20];
    sprintf(cpu_value, "%.1f%%", cpu_utilization);
    printf("CPU Utilization        < 75.0%%             %-12s %-6s   Leaves insufficient headroom for peak traffic\n", 
           cpu_value, cpu_status);  // CHANGED THRESHOLD AND EXPLANATION
    
    // IPC Throughput
    const char* ipc_status = IPC_THROUGHPUT_LINUX > 500.0 ? "PASS" : "FAIL";
    char ipc_value[20];
    sprintf(ipc_value, "%.0f MB/s", IPC_THROUGHPUT_LINUX);
    printf("IPC Throughput         > 500 MB/s          %-12s %-6s   Fast POS-backend communication via shared memory\n", 
           ipc_value, ipc_status);
    
    printf("--------------------------------------------------------------------------------\n");    
    // Round Robin Algorithm Analysis
    printf("\n================================================================================\n");
    printf("ROUND ROBIN ALGORITHM ANALYSIS\n");
    printf("================================================================================\n");
    
    printf("\nRR Selection Logic:\n");
    printf("• Each process gets equal 5ms CPU quantum\n");
    printf("• Preempted processes return to end of ready queue\n");
    printf("• New arrivals appended to queue\n");
    printf("• Guarantees maximum waiting time bounded by (n-1)*q\n");
    
    printf("\nImpact on LPUS Backend Operations:\n");
    printf("✓ POS requests get guaranteed quick response\n");
    printf("✓ No background process can monopolize CPU\n");
    printf("✓ Predictable system behavior under load\n");
    printf("✓ All processes make steady progress\n");
    printf("✗ Some throughput loss compared to SJF\n");
    printf("✗ Context switch overhead accumulates\n");
    
    printf("\n================================================================================\n");
    printf("ANALYSIS COMPLETE\n");
    printf("================================================================================\n");
    
    free(ready_queue);
}

int main() {
    run_linux_rr_analysis();
    return 0;
}
