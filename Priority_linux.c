// primecart_linux_priority_preemptive.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int remaining_time;  // ms current
    int priority;        // 1=highest, 5=lowest (lower number = higher priority)
    int start_time;      // ms
    int exit_time;       // ms
    int waiting_time;    // ms
    int turnaround_time; // ms
    int response_time;   // ms
    int completed;       // 0 = not completed, 1 = completed
    int first_response;  // Flag for first response
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

// Structure to store Gantt chart events
typedef struct {
    int process_index;
    int start_time;
    int duration;
} GanttEvent;

GanttEvent gantt_events[100];
int gantt_event_count = 0;

// Function to find the highest priority process among arrived processes
int find_highest_priority_process(int current_time, int current_process) {
    int highest_priority_index = -1;
    int highest_priority = 999; // Higher number = lower priority
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (!processes[i].completed && 
            processes[i].arrival_time <= current_time && 
            processes[i].remaining_time > 0) {
            
            // Check if this process has higher priority (lower number)
            if (processes[i].priority < highest_priority) {
                highest_priority = processes[i].priority;
                highest_priority_index = i;
            }
            // If priority is equal, check arrival time (FCFS)
            else if (processes[i].priority == highest_priority && highest_priority_index != -1) {
                if (processes[i].arrival_time < processes[highest_priority_index].arrival_time) {
                    highest_priority_index = i;
                }
            }
        }
    }
    
    return highest_priority_index;
}

void print_gantt_chart() {
    printf("\n================================================================================\n");
    printf("GANTT CHART - PREEMPTIVE PRIORITY SCHEDULING SEQUENCE\n");
    printf("================================================================================\n\n");
    
    if (gantt_event_count == 0) {
        printf("No execution events recorded.\n");
        return;
    }
    
    // Print timeline header
    printf("Time(ms) ");
    for (int i = 0; i < gantt_event_count; i++) {
        printf("|");
        int scaled_length = gantt_events[i].duration;
        // Ensure minimum length for visibility
        if (scaled_length < 2) scaled_length = 2;
        for (int j = 0; j < scaled_length; j++) {
            printf("-");
        }
    }
    printf("|\n");
    
    // Print process labels
    printf("         ");
    for (int i = 0; i < gantt_event_count; i++) {
        printf("|");
        int process_index = gantt_events[i].process_index;
        int scaled_length = gantt_events[i].duration;
        if (scaled_length < 2) scaled_length = 2;
        
        // Center the PID in the block
        if (scaled_length >= 3) {
            int padding = (scaled_length - 2) / 2;
            for (int j = 0; j < padding; j++) printf(" ");
            printf("%s", processes[process_index].pid);
            for (int j = 0; j < scaled_length - 2 - padding; j++) printf(" ");
        } else {
            // For very short blocks, just show first character
            printf("%-*s", scaled_length, processes[process_index].pid);
        }
    }
    printf("|\n");
    
    // Print time markers
    printf("        0");
    int cumulative = 0;
    int last_printed_time = 0;
    
    for (int i = 0; i < gantt_event_count; i++) {
        cumulative += gantt_events[i].duration;
        int scaled_length = gantt_events[i].duration;
        if (scaled_length < 2) scaled_length = 2;
        
        // Only print time if we have enough space
        if (cumulative - last_printed_time >= 3) {
            int spacing = scaled_length + 1;
            // Adjust spacing based on number of digits
            if (cumulative < 10) {
                printf("%*d", spacing, cumulative);
            } else if (cumulative < 100) {
                printf("%*d", spacing, cumulative);
            } else {
                printf("%*d", spacing, cumulative);
            }
            last_printed_time = cumulative;
        } else {
            // Not enough space, skip this time marker
            printf("%*s", scaled_length + 1, "");
        }
    }
    
    // Always print final time if not already printed
    if (cumulative != last_printed_time) {
        printf("%d", cumulative);
    }
    printf("\n");
    
    // Print execution sequence with better formatting
    printf("\nExecution Sequence: ");
    int seq_per_line = 0;
    for (int i = 0; i < gantt_event_count; i++) {
        int process_index = gantt_events[i].process_index;
        printf("%s", processes[process_index].pid);
        
        if (i < gantt_event_count - 1) {
            printf(" -> ");
            seq_per_line++;
            
            // Break line after 5 processes for readability
            if (seq_per_line >= 5) {
                printf("\n                   ");
                seq_per_line = 0;
            }
        }
    }
    printf("\n");
    
    // Print timing information
    printf("\nDetailed Timing:\n");
    for (int i = 0; i < gantt_event_count; i++) {
        int process_index = gantt_events[i].process_index;
        printf("  %s: [%d-%d] ms (Duration: %d ms)", 
               processes[process_index].pid,
               gantt_events[i].start_time,
               gantt_events[i].start_time + gantt_events[i].duration,
               gantt_events[i].duration);
        
        if (i < gantt_event_count - 1) {
            if (gantt_events[i+1].start_time > gantt_events[i].start_time + gantt_events[i].duration) {
                printf("  [Context Switch: 0.004 ms]\n");
            } else {
                printf("\n");
            }
        } else {
            printf("\n");
        }
    }
    
    printf("\nNote: Each '-' character represents 1ms of execution time\n");
    printf("      Context switches (0.004ms) shown as gaps between processes\n");
    printf("      Processes may be preempted multiple times (shown as separate blocks)\n");
}

void run_linux_preemptive_priority_analysis() {
    printf("================================================================================\n");
    printf("PRIMECART RETAIL - LINUX LPUS BACKEND PREEMPTIVE PRIORITY SCHEDULING ANALYSIS\n");
    printf("Ubuntu 22.04 LTS Server | Preemptive Priority Scheduling\n");
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
    
    printf("\nPriority Legend: 1=Highest (POS Tasks), 5=Lowest (Background)\n");
    printf("PREEMPTIVE: Higher priority processes can interrupt lower priority ones\n");
    
    // Reset process states
    for (int i = 0; i < NUM_PROCESSES; i++) {
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].completed = 0;
        processes[i].start_time = -1;
        processes[i].response_time = -1;
        processes[i].first_response = 0;
    }
    
    // Execute Preemptive Priority Scheduling Simulation
    int current_time = 0;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;
    int total_response_time = 0;
    int total_burst_time = 0;
    int total_idle_time = 0;
    int completed_count = 0;
    int current_process = -1;
    
    printf("\nEXECUTION TIMELINE (All times in milliseconds):\n");
    printf("================================================================================\n\n");
    
    // Initialize Gantt chart
    gantt_event_count = 0;
    
    while (completed_count < NUM_PROCESSES) {
        // Find highest priority process that has arrived
        int next_process = find_highest_priority_process(current_time, current_process);
        
        // Check if we need to switch processes (preemption or new process)
        if (next_process != -1) {
            if (current_process == -1) {
                // No process currently running, start new one
                current_process = next_process;
            }
            else if (processes[next_process].priority < processes[current_process].priority) {
                // PREEMPTION: Higher priority process arrived
                printf("[Time %dms] PREEMPTION: %s (Priority %d) preempts %s (Priority %d)\n",
                       current_time,
                       processes[next_process].pid, processes[next_process].priority,
                       processes[current_process].pid, processes[current_process].priority);
                
                // End current Gantt event
                if (gantt_event_count > 0) {
                    gantt_events[gantt_event_count-1].duration = current_time - gantt_events[gantt_event_count-1].start_time;
                }
                
                // Add context switch overhead
                current_time += CONTEXT_SWITCH_LINUX;
                current_process = next_process;
            }
            else if (next_process != current_process && processes[next_process].priority == processes[current_process].priority) {
                // Same priority, check if new process arrived earlier
                if (processes[next_process].arrival_time < processes[current_process].arrival_time) {
                    current_process = next_process;
                }
            }
        }
        
        if (current_process == -1) {
            // CPU idle
            current_time++;
            total_idle_time++;
            continue;
        }
        
        // Record start time if first execution
        if (processes[current_process].start_time == -1) {
            processes[current_process].start_time = current_time;
        }
        
        // Record response time on first execution
        if (!processes[current_process].first_response) {
            processes[current_process].response_time = current_time - processes[current_process].arrival_time;
            processes[current_process].first_response = 1;
            printf("[Time %dms] First response for %s (Priority %d, Arrival: %dms, Response Time: %dms)\n",
                   current_time, processes[current_process].pid, 
                   processes[current_process].priority,
                   processes[current_process].arrival_time,
                   processes[current_process].response_time);
        }
        
        // Record Gantt chart event if new event
        if (gantt_event_count == 0 || 
            gantt_events[gantt_event_count-1].process_index != current_process ||
            current_time != gantt_events[gantt_event_count-1].start_time + gantt_events[gantt_event_count-1].duration) {
            gantt_events[gantt_event_count].process_index = current_process;
            gantt_events[gantt_event_count].start_time = current_time;
            gantt_events[gantt_event_count].duration = 0;
            gantt_event_count++;
        }
        
        // Execute for 1 time unit
        processes[current_process].remaining_time--;
        gantt_events[gantt_event_count-1].duration++;
        current_time++;
        
        // Check if current process completed
        if (processes[current_process].remaining_time == 0) {
            processes[current_process].completed = 1;
            processes[current_process].exit_time = current_time;
            processes[current_process].turnaround_time = processes[current_process].exit_time - processes[current_process].arrival_time;
            processes[current_process].waiting_time = processes[current_process].turnaround_time - processes[current_process].burst_time;
            
            printf("[Time %dms] Completed %s (Priority %d)\n", 
                   current_time, processes[current_process].pid, processes[current_process].priority);
            printf("         Waiting Time: %dms | Response Time: %dms | Turnaround Time: %dms\n\n",
                   processes[current_process].waiting_time,
                   processes[current_process].response_time,
                   processes[current_process].turnaround_time);
            
            // Accumulate totals
            total_waiting_time += processes[current_process].waiting_time;
            total_turnaround_time += processes[current_process].turnaround_time;
            total_response_time += processes[current_process].response_time;
            total_burst_time += processes[current_process].burst_time;
            
            completed_count++;
            current_process = -1;
            
            // Add context switch overhead if more processes to run
            if (completed_count < NUM_PROCESSES) {
                current_time += CONTEXT_SWITCH_LINUX;
            }
        }
    }
    
    // Print Gantt Chart
    print_gantt_chart();
    
    // Calculate averages
    float avg_waiting_time = total_waiting_time / (float)NUM_PROCESSES;
    float avg_turnaround_time = total_turnaround_time / (float)NUM_PROCESSES;
    float avg_response_time = total_response_time / (float)NUM_PROCESSES;
    
    // Calculate CPU utilization
    int total_execution_time = current_time;
    float cpu_utilization = (total_burst_time * 100.0) / total_execution_time;
    
    // Print System-Wide Performance Metrics
    printf("\n================================================================================\n");
    printf("SYSTEM-WIDE PERFORMANCE METRICS (Preemptive Priority Scheduling)\n");
    printf("================================================================================\n");
    
    printf("\nAverage Waiting Time:    %.2f ms\n", avg_waiting_time);
    printf("Average Response Time:   %.2f ms\n", avg_response_time);
    printf("Average Turnaround Time: %.2f ms\n", avg_turnaround_time);
    printf("CPU Utilization:         %.1f%%\n", cpu_utilization);
    printf("Total Execution Time:    %d ms\n", total_execution_time);
    printf("Total CPU Busy Time:     %d ms\n", total_burst_time);
    printf("Total Idle Time:         %d ms\n", total_idle_time);
    
    // Process Performance Metrics Table
    printf("\n================================================================================\n");
    printf("PROCESS PERFORMANCE METRICS (Preemptive Priority Scheduling)\n");
    printf("================================================================================\n");
    
    printf("\n+-----+----------+----------+----------+----------+----------+------------+------------+------------+\n");
    printf("| PID | Priority | Arrival  | Burst    | Start    | Exit     | Response   | Wait       | Turnaround |\n");
    printf("+-----+----------+----------+----------+----------+----------+------------+------------+------------+\n");
    
    // Sort by priority for display
    for (int prio = 1; prio <= 5; prio++) {
        for (int i = 0; i < NUM_PROCESSES; i++) {
            if (processes[i].priority == prio) {
                printf("| %-3s | %-8d | %-8d | %-8d | %-8d | %-8d | %-10d | %-10d | %-10d |\n",
                       processes[i].pid,
                       processes[i].priority,
                       processes[i].arrival_time,
                       processes[i].burst_time,
                       processes[i].start_time,
                       processes[i].exit_time,
                       processes[i].response_time,
                       processes[i].waiting_time,
                       processes[i].turnaround_time);
            }
        }
    }
    printf("+-----+----------+----------+----------+----------+----------+------------+------------+------------+\n");
    
    // PrimeCart Threshold Analysis
    printf("\n================================================================================\n");
    printf("PRIMECART THRESHOLD ANALYSIS (Ubuntu 22.04 LTS Server)\n");
    printf("================================================================================\n");
    printf("Metric                 Target Threshold    Linux Value     Status   Why It Matters for PrimeCart\n");
    printf("--------------------------------------------------------------------------------\n");
    
    // Interrupt Latency
    const char* int_status = INTERRUPT_LATENCY_LINUX < 0.100 ? "PASS" : "FAIL";
    printf("Interrupt Latency      < 0.100 ms          %-12s %-6s   Ensures barcode scanner is detected immediately\n", 
           "0.080 ms", int_status);
    
    // Scheduling Jitter  
    const char* jitter_status = SCHEDULING_JITTER_LINUX < 2.000 ? "PASS" : "FAIL";
    printf("Scheduling Jitter      < 2.000 ms          %-12s %-6s   Prevents cashier screen stuttering\n", 
           "0.0015 ms", jitter_status);
    
    // Context Switch Time - FIXED: Business impact explanation
    const char* cs_status = CONTEXT_SWITCH_LINUX <= 0.010 ? "PASS" : "FAIL";
    printf("Context Switch Time    < 0.010 ms          %-12s %-6s   Maintains smooth task switching during checkout\n", 
           "0.004 ms", cs_status);  // CHANGED EXPLANATION
    
    // CPU Utilization
    const char* cpu_status = cpu_utilization < 75.0 ? "PASS" : "FAIL";
    char cpu_value[20];
    sprintf(cpu_value, "%.1f%%", cpu_utilization);
    printf("CPU Utilization        < 75.0%%             %-12s %-6s   Leaves no headroom for background tasks during peak traffic\n", 
           cpu_value, cpu_status);
    
    // IPC Throughput
    const char* ipc_status = IPC_THROUGHPUT_LINUX > 500.0 ? "PASS" : "FAIL";
    char ipc_value[20];
    sprintf(ipc_value, "%.0f MB/s", IPC_THROUGHPUT_LINUX);
    printf("IPC Throughput         > 500 MB/s          %-12s %-6s   Enables fast communication between POS and backend systems\n", 
           ipc_value, ipc_status);
    
    printf("\n================================================================================\n");
    printf("ANALYSIS COMPLETE - PREEMPTIVE PRIORITY SCHEDULING\n");
    printf("================================================================================\n");
}

int main() {
    run_linux_preemptive_priority_analysis();
    return 0;
}
