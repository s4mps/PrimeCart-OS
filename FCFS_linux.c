#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Linux performance characteristics
#define CONTEXT_SWITCH_LINUX 0.004    // 4 μs in ms (more efficient than typical systems)
#define INTERRUPT_LATENCY_LINUX 0.080 // 80 μs in ms
#define SCHEDULING_JITTER_LINUX 0.0015 // 1.5 ms
#define IPC_THROUGHPUT_LINUX 950.0    // MB/s (Linux IPC outperforms typical systems)

typedef struct {
    char pid[4];
    char description[50];
    char process_type[12];
    int arrival_time;    // ms
    int burst_time;      // ms
    int priority;        // 1=highest, 5=lowest
    int start_time;      // ms
    int exit_time;       // ms
    int waiting_time;    // ms
    int turnaround_time; // ms
    int response_time;   // ms
} LinuxProcess;

LinuxProcess processes[] = {
    {"P1", "LPUS Batch Update: SQL DB Write", "Background", 0, 20, 4, 0, 0, 0, 0, 0},
    {"P2", "POS Scan Validation: Barcode Check", "Foreground", 1, 6, 1, 0, 0, 0, 0, 0},
    {"P3", "POS Price Lookup: GUI Display", "Foreground", 2, 4, 1, 0, 0, 0, 0, 0},
    {"P4", "LPUS Inventory Sync: Stock Upload", "Background", 4, 12, 4, 0, 0, 0, 0, 0},
    {"P5", "POS Payment Auth: Data Encryption", "Foreground", 5, 8, 2, 0, 0, 0, 0, 0},
    {"P6", "LPUS Metadata Refresh: Cache Update", "Background", 7, 10, 5, 0, 0, 0, 0, 0},
    {"P7", "POS Receipt Gen: Log Transaction", "Foreground", 9, 4, 1, 0, 0, 0, 0, 0}                                                                                                                                                       
};                                                                                                                                                                                                                                         
                                                                                                                                                                                                                                           
#define NUM_PROCESSES (sizeof(processes)/sizeof(processes[0]))                                                                                                                                                                             
                                                                                                                                                                                                                                           
void sort_by_arrival_time() {                                                                                                                                                                                                              
    for (int i = 0; i < NUM_PROCESSES - 1; i++) {                                                                                                                                                                                          
        for (int j = 0; j < NUM_PROCESSES - i - 1; j++) {                                                                                                                                                                                  
            if (processes[j].arrival_time > processes[j + 1].arrival_time) {                                                                                                                                                               
                LinuxProcess temp = processes[j];                                                                                                                                                                                          
                processes[j] = processes[j + 1];                                                                                                                                                                                           
                processes[j + 1] = temp;                                                                                                                                                                                                   
            }
        }
    }
}

void print_gantt_chart() {
    printf("\n================================================================================\n");
    printf("GANTT CHART - FCFS SCHEDULING SEQUENCE\n");
    printf("================================================================================\n\n");
    
    // Print timeline header
    printf("Time(ms) ");
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("|");
        // Scale: each character = 2ms
        int scaled_length = processes[i].burst_time / 2;
        for (int j = 0; j < scaled_length; j++) {
            printf("-");
        }
    }
    printf("|\n");
    
    // Print process labels
    printf("         ");
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("|");
        int scaled_length = processes[i].burst_time / 2;
        printf("%-*s", scaled_length, processes[i].pid);
    }
    printf("|\n");
    
    // Print time markers
    printf("        0");
    int cumulative = 0;
    for (int i = 0; i < NUM_PROCESSES; i++) {
        cumulative += processes[i].burst_time;
        int spacing = (processes[i].burst_time / 2) + 1;
        printf("%*d", spacing, cumulative);
    }
    printf("\n");
    
    printf("\nExecution Sequence: ");
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("%s", processes[i].pid);
        if (i < NUM_PROCESSES - 1) {
            printf(" -> ");
        }
    }
    printf("\n");
    
    printf("Note: Each '-' character represents 2ms of execution time\n");
    printf("      Context switches (0.1ms) shown as gaps between processes\n");
}

void run_linux_fcfs_analysis() {
    printf("================================================================================\n");
    printf("PRIMECART RETAIL - LINUX LPUS BACKEND FCFS ANALYSIS\n");
    printf("Ubuntu 22.04 LTS Server | Non-Preemptive FCFS Scheduling\n");
    printf("================================================================================\n");
    
    sort_by_arrival_time();
    
    // Print Process Execution Order Table
    printf("\nPROCESS EXECUTION ORDER (Sorted by Arrival Time - FCFS):\n");
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
    
    // Execute FCFS Simulation
    int current_time = 0;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;
    int total_response_time = 0;
    int total_burst_time = 0;
    int total_idle_time = 0;
    
    printf("\nEXECUTION TIMELINE (All times in milliseconds):\n");
    printf("================================================================================\n\n");
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        // Handle idle time - process waits until arrival
        if (current_time < processes[i].arrival_time) {
            total_idle_time += (processes[i].arrival_time - current_time);
            current_time = processes[i].arrival_time;
        }
        
        // Record start time
        processes[i].start_time = current_time;
        
        // Calculate response time (time from arrival to first execution)
        processes[i].response_time = processes[i].start_time - processes[i].arrival_time;
        
        // Show convoy effect for POS tasks
        if (i > 0 && processes[i].arrival_time < processes[i-1].exit_time) {
            printf("[Time %dms] %s ARRIVED but WAITING for %s to complete (Convoy Effect)\n", 
                   processes[i].arrival_time, processes[i].pid, processes[i-1].pid);
        }
        
        printf("[Time %dms] Starting %s\n", current_time, processes[i].pid);
        printf("[Linux] Executing %s - %s\n", processes[i].pid, processes[i].description);
        
        // Simulate execution (non-preemptive)
        usleep(processes[i].burst_time * 1000);
        
        // Record completion time
        processes[i].exit_time = current_time + processes[i].burst_time;
        
        // Calculate turnaround time (time from arrival to completion)
        processes[i].turnaround_time = processes[i].exit_time - processes[i].arrival_time;
        
        // Calculate waiting time (time spent waiting in ready queue)
        processes[i].waiting_time = processes[i].start_time - processes[i].arrival_time;
        
        printf("[Time %dms] Completed %s\n", processes[i].exit_time, processes[i].pid);
        printf("         Waiting Time: %dms | Response Time: %dms | Turnaround Time: %dms\n\n",
               processes[i].waiting_time,
               processes[i].response_time,
               processes[i].turnaround_time);
        
        // Accumulate totals
        total_waiting_time += processes[i].waiting_time;
        total_turnaround_time += processes[i].turnaround_time;
        total_response_time += processes[i].response_time;
        total_burst_time += processes[i].burst_time;
        
        current_time = processes[i].exit_time;
        
        // Add context switch overhead (Linux is more efficient)
        if (i < NUM_PROCESSES - 1) {
            current_time += 1; // 0.1ms context switch
        }
    }
    
    // Print Gantt Chart
    print_gantt_chart();
    
    // Calculate averages (similar to other systems due to identical FCFS logic)
    float avg_waiting_time = total_waiting_time / (float)NUM_PROCESSES;
    float avg_turnaround_time = total_turnaround_time / (float)NUM_PROCESSES;
    float avg_response_time = total_response_time / (float)NUM_PROCESSES;
    
    // Calculate CPU utilization
    int total_execution_time = current_time;
    float cpu_utilization = (total_burst_time * 100.0) / total_execution_time;
    
    // Print System-Wide Performance Metrics
    printf("\n================================================================================\n");
    printf("SYSTEM-WIDE PERFORMANCE METRICS\n");
    printf("================================================================================\n");
    
    printf("\nAverage Waiting Time: %.1f ms\n", avg_waiting_time);
    printf("Average Response Time: %.1f ms\n", avg_response_time);
    printf("Average Turnaround Time: %.1f ms\n", avg_turnaround_time);
    printf("CPU Utilization: %.1f%%\n", cpu_utilization);
    printf("Throughput: %.2f processes/second\n", NUM_PROCESSES / (total_execution_time / 1000.0));
    printf("Total Execution Time: %d ms\n", total_execution_time);
    printf("Total CPU Busy Time: %d ms\n", total_burst_time);
    printf("Total Idle Time: %d ms\n", total_idle_time);
    
    // Performance Analysis Table
    printf("\n================================================================================\n");
    printf("PROCESS PERFORMANCE METRICS\n");
    printf("================================================================================\n");
    
    printf("\n+-----+----------+----------+----------+----------+----------+------------+\n");
    printf("| PID | Arrival  | Burst    | Start    | Exit     | Wait     | Turnaround |\n");
    printf("+-----+----------+----------+----------+----------+----------+------------+\n");
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("| %-3s | %-8d | %-8d | %-8d | %-8d | %-8d | %-10d |\n",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].burst_time,
               processes[i].start_time,
               processes[i].exit_time,
               processes[i].waiting_time,
               processes[i].turnaround_time);
    }
    printf("+-----+----------+----------+----------+----------+----------+------------+\n");
    // PrimeCart Threshold Analysis (Non-table format)
    printf("\n================================================================================\n");
    printf("PRIMECART THRESHOLD ANALYSIS (Ubuntu 22.04 LTS Server)\n");
    printf("================================================================================\n");
    printf("Metric                 Target Threshold    Linux Value     Status   Why It Matters for PrimeCart\n");
    printf("--------------------------------------------------------------------------------\n");
    
    // Interrupt Latency
    const char* int_status = INTERRUPT_LATENCY_LINUX < 0.100 ? "PASS" : "FAIL";
    printf("Interrupt Latency      < 0.100 ms          %-12s %-6s   Ensures POS scanner requests are processed immediately\n", 
           "0.080 ms", int_status);
    
    // Scheduling Jitter
    const char* jitter_status = SCHEDULING_JITTER_LINUX < 2.000 ? "PASS" : "FAIL";
    printf("Scheduling Jitter      < 2.000 ms          %-12s %-6s   Prevents inconsistent LPUS response times to POS terminals\n", 
           "0.001 ms", jitter_status);
    
    // Context Switch Time
    const char* cs_status = CONTEXT_SWITCH_LINUX <= 0.010 ? "PASS" : "FAIL";
    printf("Context Switch Time    < 0.010 ms          %-12s %-6s   Linux's task_struct optimization enables highly efficient context switching\n", 
           "0.004 ms", cs_status);

    // CPU Utilization
    const char* cpu_status = cpu_utilization < 75.0 ? "PASS" : "FAIL";
    char cpu_value[20];
    sprintf(cpu_value, "%.1f%%", cpu_utilization);
    printf("CPU Utilization        < 75.0%%             %-12s %-6s   Fails to maintain headroom for traffic spikes, risking checkout delays\n", 
           cpu_value, cpu_status);
    
    // IPC Throughput
    const char* ipc_status = IPC_THROUGHPUT_LINUX > 500.0 ? "PASS" : "FAIL";
    char ipc_value[20];
    sprintf(ipc_value, "%.0f MB/s", IPC_THROUGHPUT_LINUX);
    printf("IPC Throughput         > 500 MB/s          %-12s %-6s   Linux's efficient IPC mechanisms ensure fast POS-to-LPUS communication\n", 
           ipc_value, ipc_status);
    
    printf("--------------------------------------------------------------------------------\n");    
    // Convoy Effect Analysis
    printf("\n================================================================================\n");
    printf("CONVOY EFFECT ANALYSIS\n");
    printf("================================================================================\n");
    
    printf("\nCritical Issue Detected: P1 (LPUS Batch Update) blocks all POS requests:\n\n");
    printf("• P2 (POS Scan) arrived at 1ms but waited %dms for P1 to complete\n", processes[1].waiting_time);
    printf("• P3 (POS Lookup) arrived at 2ms but waited %dms for P1 to complete\n", processes[2].waiting_time);
    printf("• P5 (POS Payment) arrived at 5ms but waited %dms\n", processes[4].waiting_time);
    printf("• P7 (POS Receipt) arrived at 9ms but waited %dms\n\n", processes[6].waiting_time);
    
    printf("Note: Linux handles context switches more efficiently (0.004ms) than many systems,\n");
    printf("but the convoy effect from FCFS scheduling dominates performance degradation.\n");
    
    printf("\n================================================================================\n");
    printf("ANALYSIS COMPLETE\n");
    printf("================================================================================\n");
}

int main() {
    run_linux_fcfs_analysis();
    return 0;
