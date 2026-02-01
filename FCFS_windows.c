#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

// Windows performance characteristics (hybrid kernel)
#define CONTEXT_SWITCH_WINDOWS 0.010  // 10 μs in ms
#define INTERRUPT_LATENCY_WINDOWS 0.095  // 95 μs in ms
#define SCHEDULING_JITTER_WINDOWS 0.0008 // 0.8 ms
#define IPC_THROUGHPUT_WINDOWS 850.0    // MB/s (Lower than Linux's 950 MB/s)

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
} WindowsThread;

WindowsThread threads[] = {
    {"P1", "LPUS Batch Update: SQL DB Write", "Background", 0, 20, 4, 0, 0, 0, 0, 0},
    {"P2", "POS Scan Validation: Barcode Check", "Foreground", 1, 6, 1, 0, 0, 0, 0, 0},
    {"P3", "POS Price Lookup: GUI Display", "Foreground", 2, 4, 1, 0, 0, 0, 0, 0},
    {"P4", "LPUS Inventory Sync: Stock Upload", "Background", 4, 12, 4, 0, 0, 0, 0, 0},
    {"P5", "POS Payment Auth: Data Encryption", "Foreground", 5, 8, 2, 0, 0, 0, 0, 0},
    {"P6", "LPUS Metadata Refresh: Cache Update", "Background", 7, 10, 5, 0, 0, 0, 0, 0},
    {"P7", "POS Receipt Gen: Log Transaction", "Foreground", 9, 4, 1, 0, 0, 0, 0, 0}
};

#define NUM_THREADS (sizeof(threads)/sizeof(threads[0]))

void sort_by_arrival_time() {
    for (int i = 0; i < NUM_THREADS - 1; i++) {
        for (int j = 0; j < NUM_THREADS - i - 1; j++) {
            if (threads[j].arrival_time > threads[j + 1].arrival_time) {
                WindowsThread temp = threads[j];
                threads[j] = threads[j + 1];
                threads[j + 1] = temp;
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
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("|");
        // Scale: each character = 2ms
        int scaled_length = threads[i].burst_time / 2;
        for (int j = 0; j < scaled_length; j++) {
            printf("-");
        }
    }
    printf("|\n");
    
    // Print process labels
    printf("         ");
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("|");
        int scaled_length = threads[i].burst_time / 2;
        printf("%-*s", scaled_length, threads[i].pid);
    }
    printf("|\n");
    
    // Print time markers
    printf("        0");
    int cumulative = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        cumulative += threads[i].burst_time;
        int spacing = (threads[i].burst_time / 2) + 1;
        printf("%*d", spacing, cumulative);
    }
    printf("\n");
    
    printf("\nExecution Sequence: ");
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("%s", threads[i].pid);
        if (i < NUM_THREADS - 1) {
            printf(" -> ");
        }
    }
    printf("\n");
    
}

void run_windows_fcfs_analysis() {
    printf("================================================================================\n");
    printf("PRIMECART RETAIL - WINDOWS POS FRONTEND FCFS ANALYSIS\n");
    printf("Windows 10 IoT Enterprise | Non-Preemptive FCFS Scheduling\n");
    printf("================================================================================\n");
    
    sort_by_arrival_time();
    
    // Print Process Execution Order Table
    printf("\nPROCESS EXECUTION ORDER (Sorted by Arrival Time - FCFS):\n");
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
    
    // Execute FCFS Simulation
    int current_time = 0;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;
    int total_response_time = 0;
    int total_burst_time = 0;
    int total_idle_time = 0;
    
    printf("\nEXECUTION TIMELINE (All times in milliseconds):\n");
    printf("================================================================================\n\n");
    
    for (int i = 0; i < NUM_THREADS; i++) {
        // Handle idle time - process waits until arrival
        if (current_time < threads[i].arrival_time) {
            total_idle_time += (threads[i].arrival_time - current_time);
            current_time = threads[i].arrival_time;
        }
        
        // Record start time
        threads[i].start_time = current_time;
        
        // Calculate response time (time from arrival to first execution)
        threads[i].response_time = threads[i].start_time - threads[i].arrival_time;
        
        // Show convoy effect for POS tasks
        if (i > 0 && threads[i].arrival_time < threads[i-1].exit_time) {
            printf("[Time %dms] %s ARRIVED but WAITING for %s to complete (Convoy Effect)\n", 
                   threads[i].arrival_time, threads[i].pid, threads[i-1].pid);
        }
        
        printf("[Time %dms] Starting %s\n", current_time, threads[i].pid);
        printf("[Windows] Executing %s - %s\n", threads[i].pid, threads[i].description);
        
        threads[i].exit_time = current_time + threads[i].burst_time;
        threads[i].turnaround_time = threads[i].exit_time - threads[i].arrival_time;
        
        // Calculate waiting time (time spent waiting in ready queue)
        threads[i].waiting_time = threads[i].start_time - threads[i].arrival_time;
        
        printf("[Time %dms] Completed %s\n", threads[i].exit_time, threads[i].pid);
        printf("         Waiting Time: %dms | Response Time: %dms | Turnaround Time: %dms\n\n",
               threads[i].waiting_time,
               threads[i].response_time,
               threads[i].turnaround_time);
        
        // Accumulate totals
        total_waiting_time += threads[i].waiting_time;
        total_turnaround_time += threads[i].turnaround_time;
        total_response_time += threads[i].response_time;
        total_burst_time += threads[i].burst_time;
        
        current_time = threads[i].exit_time;
        
        // Add context switch overhead
        if (i < NUM_THREADS - 1) {
            current_time += 1; // 0.1ms context switch
        }
    }
    
    // Print Gantt Chart
    print_gantt_chart();
    
    // Calculate averages
    float avg_waiting_time = total_waiting_time / (float)NUM_THREADS;
    float avg_turnaround_time = total_turnaround_time / (float)NUM_THREADS;
    float avg_response_time = total_response_time / (float)NUM_THREADS;
    
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
    printf("Throughput: %.2f processes/second\n", NUM_THREADS / (total_execution_time / 1000.0));
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
    
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("| %-3s | %-8d | %-8d | %-8d | %-8d | %-8d | %-10d |\n",
               threads[i].pid,
               threads[i].arrival_time,
               threads[i].burst_time,
               threads[i].start_time,
               threads[i].exit_time,
               threads[i].waiting_time,
               threads[i].turnaround_time);
    }
    printf("+-----+----------+----------+----------+----------+----------+------------+\n");
    
    // PrimeCart Threshold Analysis (Non-table format)
    printf("\n================================================================================\n");
    printf("PRIMECART THRESHOLD ANALYSIS (Windows 10 IoT Enterprise)\n");
    printf("================================================================================\n");
    printf("Metric                 Target Threshold    Windows Value   Status   Why It Matters for PrimeCart\n");
    printf("--------------------------------------------------------------------------------\n");
    
    // Interrupt Latency
    const char* int_status = INTERRUPT_LATENCY_WINDOWS < 0.100 ? "PASS" : "FAIL";
    printf("Interrupt Latency      < 0.100 ms          %-9s      %-6s   Ensures barcode scanner is detected immediately\n", 
           "0.095 ms", int_status);
    
    // Scheduling Jitter  
    const char* jitter_status = SCHEDULING_JITTER_WINDOWS < 2.000 ? "PASS" : "FAIL";
    printf("Scheduling Jitter      < 2.000 ms          %-9s      %-6s   Prevents cashier screen stuttering\n", 
           "0.001 ms", jitter_status);
    
    // Context Switch Time
    const char* cs_status = CONTEXT_SWITCH_WINDOWS <= 0.010 ? "PASS" : "FAIL";
    printf("Context Switch Time    < 0.010 ms          %-9s      %-6s   Windows hybrid kernel adds mode transition overhead\n", 
           "0.010 ms", cs_status);
    
    // CPU Utilization - FIXED: Added proper width for percentage
    const char* cpu_status = cpu_utilization < 75.0 ? "PASS" : "FAIL";
    char cpu_value[10];
    sprintf(cpu_value, "%.1f%%", cpu_utilization);
    printf("CPU Utilization        < 75.0%%             %-9s      %-6s   Leaves insufficient headroom for peak traffic\n", 
           cpu_value, cpu_status);
    
    // IPC Throughput - FIXED: Added proper width for MB/s value
    const char* ipc_status = IPC_THROUGHPUT_WINDOWS > 500.0 ? "PASS" : "FAIL";
    char ipc_value[10];
    sprintf(ipc_value, "%.0f MB/s", IPC_THROUGHPUT_WINDOWS);
    printf("IPC Throughput         > 500 MB/s          %-9s      %-6s   Enables fast POS-backend communication\n", 
           ipc_value, ipc_status);
    
    printf("--------------------------------------------------------------------------------\n");   
    // Convoy Effect Analysis
    printf("\n================================================================================\n");
    printf("CONVOY EFFECT ANALYSIS\n");
    printf("================================================================================\n");
    
    printf("\nCritical Issue Detected: P1 (LPUS Batch Update) blocks all POS transactions:\n\n");
    printf("• P2 (POS Scan) arrived at 1ms but waited %dms for P1 to complete\n", threads[1].waiting_time);
    printf("• P3 (POS Lookup) arrived at 2ms but waited %dms for P1 to complete\n", threads[2].waiting_time);
    printf("• P5 (POS Payment) arrived at 5ms but waited %dms\n", threads[4].waiting_time);
    printf("• P7 (POS Receipt) arrived at 9ms but waited %dms\n\n", threads[6].waiting_time);
    
    printf("This convoy effect violates POS response time requirements (<5ms).\n");
    
    printf("\n================================================================================\n");
    printf("ANALYSIS COMPLETE\n");
    printf("================================================================================\n");
}

int main() {
    run_windows_fcfs_analysis();
    return 0;
}
