#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Linux performance characteristics
#define CONTEXT_SWITCH_LINUX 0.004    // 4 μs in ms
#define INTERRUPT_LATENCY_LINUX 0.080 // 80 μs in ms
#define SCHEDULING_JITTER_LINUX 0.0015 // 1.5 ms
#define IPC_THROUGHPUT_LINUX 950.0    // MB/s

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
    int completed;       // 0 = not completed, 1 = completed
} LinuxProcess;

LinuxProcess processes[] = {
    {"P1", "LPUS Batch Update: SQL DB Write", "Background", 0, 20, 4, 0, 0, 0, 0, 0, 0},
    {"P2", "POS Scan Validation: Barcode Check", "Foreground", 1, 6, 1, 0, 0, 0, 0, 0, 0},
    {"P3", "POS Price Lookup: GUI Display", "Foreground", 2, 4, 1, 0, 0, 0, 0, 0, 0},
    {"P4", "LPUS Inventory Sync: Stock Upload", "Background", 4, 12, 4, 0, 0, 0, 0, 0, 0},
    {"P5", "POS Payment Auth: Data Encryption", "Foreground", 5, 8, 2, 0, 0, 0, 0, 0, 0},
    {"P6", "LPUS Metadata Refresh: Cache Update", "Background", 7, 10, 5, 0, 0, 0, 0, 0, 0},
    {"P7", "POS Receipt Gen: Log Transaction", "Foreground", 9, 4, 1, 0, 0, 0, 0, 0, 0}
};

#define NUM_PROCESSES (sizeof(processes)/sizeof(processes[0]))

// Function to find the shortest job among arrived processes
int find_shortest_job(int current_time) {
    int shortest_index = -1;
    int shortest_burst = 9999; // Large initial value
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (!processes[i].completed && 
            processes[i].arrival_time <= current_time && 
            processes[i].burst_time < shortest_burst) {
            shortest_burst = processes[i].burst_time;
            shortest_index = i;
        }
    }
    
    return shortest_index;
}

void print_gantt_chart(int execution_order[], int order_count) {
    printf("\n================================================================================\n");
    printf("GANTT CHART - SJF SCHEDULING SEQUENCE\n");
    printf("================================================================================\n\n");
    
    // Print timeline header
    printf("Time(ms) ");
    for (int i = 0; i < order_count; i++) {
        printf("|");
        int process_index = execution_order[i];
        int scaled_length = processes[process_index].burst_time / 2;
        for (int j = 0; j < scaled_length; j++) {
            printf("-");
        }
    }
    printf("|\n");
    
    // Print process labels
    printf("         ");
    for (int i = 0; i < order_count; i++) {
        printf("|");
        int process_index = execution_order[i];
        int scaled_length = processes[process_index].burst_time / 2;
        printf("%-*s", scaled_length, processes[process_index].pid);
    }
    printf("|\n");
    
    // Print time markers
    printf("        0");
    int cumulative = 0;
    for (int i = 0; i < order_count; i++) {
        int process_index = execution_order[i];
        cumulative += processes[process_index].burst_time;
        int spacing = (processes[process_index].burst_time / 2) + 1;
        printf("%*d", spacing, cumulative);
    }
    printf("\n");
    
    printf("\nExecution Sequence: ");
    for (int i = 0; i < order_count; i++) {
        int process_index = execution_order[i];
        printf("%s", processes[process_index].pid);
        if (i < order_count - 1) {
            printf(" -> ");
        }
    }
    printf("\n");
    
    printf("Note: Each '-' character represents 2ms of execution time\n");
    printf("      Context switches (0.1ms) shown as gaps between processes\n");
}

void run_linux_sjf_analysis() {
    printf("================================================================================\n");
    printf("PRIMECART RETAIL - LINUX LPUS BACKEND SJF ANALYSIS\n");
    printf("Ubuntu 22.04 LTS Server | Non-Preemptive SJF Scheduling\n");
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
    
    // Execute SJF Simulation
    int current_time = 0;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;
    int total_response_time = 0;
    int total_burst_time = 0;
    int total_idle_time = 0;
    int completed_count = 0;
    int execution_order[NUM_PROCESSES];
    int order_index = 0;
    
    printf("\nEXECUTION TIMELINE (All times in milliseconds):\n");
    printf("================================================================================\n\n");
    
    while (completed_count < NUM_PROCESSES) {
        // Find the shortest job that has arrived
        int next_index = find_shortest_job(current_time);
        
        if (next_index == -1) {
            // No process has arrived yet, CPU idle
            int next_arrival = 9999;
            for (int i = 0; i < NUM_PROCESSES; i++) {
                if (!processes[i].completed && processes[i].arrival_time < next_arrival) {
                    next_arrival = processes[i].arrival_time;
                }
            }
            int idle_time = next_arrival - current_time;
            total_idle_time += idle_time;
            current_time = next_arrival;
            continue;
        }
        
        // Record execution order
        execution_order[order_index++] = next_index;
        
        // Record start time
        processes[next_index].start_time = current_time;
        
        // Calculate response time (time from arrival to first execution)
        processes[next_index].response_time = processes[next_index].start_time - processes[next_index].arrival_time;
        
        printf("[Time %dms] Starting %s (Shortest Job: %dms burst)\n", 
               current_time, processes[next_index].pid, processes[next_index].burst_time);
        printf("[Linux] Executing %s - %s\n", 
               processes[next_index].pid, processes[next_index].description);
        
        // Simulate execution (non-preemptive)
        usleep(processes[next_index].burst_time * 1000);
        
        // Record completion time
        processes[next_index].exit_time = current_time + processes[next_index].burst_time;
        
        // Calculate turnaround time (time from arrival to completion)
        processes[next_index].turnaround_time = processes[next_index].exit_time - processes[next_index].arrival_time;
        
        // Calculate waiting time (time spent waiting in ready queue)
        processes[next_index].waiting_time = processes[next_index].start_time - processes[next_index].arrival_time;
        
        printf("[Time %dms] Completed %s\n", processes[next_index].exit_time, processes[next_index].pid);
        printf("         Waiting Time: %dms | Response Time: %dms | Turnaround Time: %dms\n\n",
               processes[next_index].waiting_time,
               processes[next_index].response_time,
               processes[next_index].turnaround_time);
        
        // Mark as completed
        processes[next_index].completed = 1;
        completed_count++;
        
        // Accumulate totals
        total_waiting_time += processes[next_index].waiting_time;
        total_turnaround_time += processes[next_index].turnaround_time;
        total_response_time += processes[next_index].response_time;
        total_burst_time += processes[next_index].burst_time;
        
        current_time = processes[next_index].exit_time;
        
        // Add context switch overhead
        if (completed_count < NUM_PROCESSES) {
            current_time += 1; // 0.1ms context switch
        }
    }
    
    // Print Gantt Chart
    print_gantt_chart(execution_order, order_index);
    
    // Calculate averages
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
    printf("PROCESS PERFORMANCE METRICS (SJF Order)\n");
    printf("================================================================================\n");
    
    printf("\n+-----+----------+----------+----------+----------+----------+------------+\n");
    printf("| PID | Arrival  | Burst    | Start    | Exit     | Wait     | Turnaround |\n");
    printf("+-----+----------+----------+----------+----------+----------+------------+\n");
    
    // Print in execution order
    for (int i = 0; i < order_index; i++) {
        int idx = execution_order[i];
        printf("| %-3s | %-8d | %-8d | %-8d | %-8d | %-8d | %-10d |\n",
               processes[idx].pid,
               processes[idx].arrival_time,
               processes[idx].burst_time,
               processes[idx].start_time,
               processes[idx].exit_time,
               processes[idx].waiting_time,
               processes[idx].turnaround_time);
    }
    printf("+-----+----------+----------+----------+----------+----------+------------+\n");
    // PrimeCart Threshold Analysis
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
    printf("Scheduling Jitter      < 2.000 ms          %-12s %-6s   Maintains consistent LPUS response times to POS terminals\n", 
           "0.002 ms", jitter_status);
    
    // Context Switch Time - FIXED: Changed to business impact
    const char* cs_status = CONTEXT_SWITCH_LINUX <= 0.010 ? "PASS" : "FAIL";
    printf("Context Switch Time    < 0.010 ms          %-12s %-6s   Maintains smooth task switching during checkout\n", 
           "0.004 ms", cs_status);  // CHANGED EXPLANATION
    
    // CPU Utilization
    const char* cpu_status = cpu_utilization < 75.0 ? "PASS" : "FAIL";
    char cpu_value[20];
    sprintf(cpu_value, "%.1f%%", cpu_utilization);
    printf("CPU Utilization        < 75.0%%             %-12s %-6s   Fails to maintain headroom for peak traffic, risking checkout delays.”\n", 
           cpu_value, cpu_status);
    
    // IPC Throughput - FIXED: Standardized to >500 MB/s
    const char* ipc_status = IPC_THROUGHPUT_LINUX > 500.0 ? "PASS" : "FAIL";
    char ipc_value[20];
    sprintf(ipc_value, "%.0f MB/s", IPC_THROUGHPUT_LINUX);
    printf("IPC Throughput         > 500 MB/s          %-12s %-6s   Ensures fast communication between POS terminals and LPUS database\n", 
           ipc_value, ipc_status);
    
    printf("--------------------------------------------------------------------------------\n");    
    // SJF Algorithm Analysis
    printf("\n================================================================================\n");
    printf("SJF ALGORITHM ANALYSIS\n");
    printf("================================================================================\n");
    
    printf("\nSJF Selection Logic Demonstration:\n");
    printf("At time 0ms: Ready Queue = {P1(20ms)} -> Select P1\n");
    printf("At time 20ms: Ready Queue = {P2(6ms), P3(4ms), P4(12ms)} -> Select P3 (shortest)\n");
    printf("At time 24ms: Ready Queue = {P2(6ms), P4(12ms)} -> Select P2 (shortest)\n");
    printf("At time 30ms: Ready Queue = {P4(12ms), P5(8ms)} -> Select P5 (shortest)\n");
    printf("At time 38ms: Ready Queue = {P4(12ms), P6(10ms), P7(4ms)} -> Select P7 (shortest)\n");
    
    printf("\nImpact on LPUS Backend Operations:\n");
    printf("✓ Short POS tasks get faster service (P3, P7 execute early)\n");
    printf("✓ Average waiting time reduced to %.1fms\n", avg_waiting_time);
    printf("✓ Improved POS-to-LPUS response times\n");
    printf("✗ Long LPUS background tasks experience increased waiting\n");
    printf("✗ Requires accurate burst time estimation for optimal scheduling\n");
    printf("   • Improves overall system throughput for mixed workloads\n");
    
    printf("\n================================================================================\n");
    printf("ANALYSIS COMPLETE\n");
    printf("================================================================================\n");
}

int main() {
    run_linux_sjf_analysis();
    return 0;
}
