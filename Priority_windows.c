// primecart_windows_priority_preemptive.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

// Windows performance characteristics
#define CONTEXT_SWITCH_WINDOWS 0.010  // 10 μs in ms
#define INTERRUPT_LATENCY_WINDOWS 0.095  // 95 μs in ms
#define SCHEDULING_JITTER_WINDOWS 0.0008 // 0.8 ms
#define IPC_THROUGHPUT_WINDOWS 850.0    // MB/s

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

// Structure to store Gantt chart events
typedef struct {
    int thread_index;
    int start_time;
    int duration;
} GanttEvent;

GanttEvent gantt_events[100];
int gantt_event_count = 0;

// Function to find the highest priority thread among arrived threads
int find_highest_priority_thread(int current_time, int current_thread) {
    int highest_priority_index = -1;
    int highest_priority = 999; // Higher number = lower priority
    
    for (int i = 0; i < NUM_THREADS; i++) {
        if (!threads[i].completed && 
            threads[i].arrival_time <= current_time && 
            threads[i].remaining_time > 0) {
            
            // Check if this thread has higher priority (lower number)
            if (threads[i].priority < highest_priority) {
                highest_priority = threads[i].priority;
                highest_priority_index = i;
            }
            // If priority is equal, check arrival time (FCFS)
            else if (threads[i].priority == highest_priority && highest_priority_index != -1) {
                if (threads[i].arrival_time < threads[highest_priority_index].arrival_time) {
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
    
    // Calculate total execution time for scaling
    int total_time = 0;
    for (int i = 0; i < gantt_event_count; i++) {
        total_time += gantt_events[i].duration;
    }
    
    // Print timeline header - FIXED for better spacing
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
    
    // Print process labels - FIXED spacing
    printf("         ");
    for (int i = 0; i < gantt_event_count; i++) {
        printf("|");
        int thread_index = gantt_events[i].thread_index;
        int scaled_length = gantt_events[i].duration;
        if (scaled_length < 2) scaled_length = 2;
        
        // Center the PID in the block
        if (scaled_length >= 3) {
            int padding = (scaled_length - 2) / 2;
            for (int j = 0; j < padding; j++) printf(" ");
            printf("%s", threads[thread_index].pid);
            for (int j = 0; j < scaled_length - 2 - padding; j++) printf(" ");
        } else {
            // For very short blocks, just show first character
            printf("%-*s", scaled_length, threads[thread_index].pid);
        }
    }
    printf("|\n");
    
    // Print time markers - FIXED to show at process boundaries
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
        int thread_index = gantt_events[i].thread_index;
        printf("%s", threads[thread_index].pid);
        
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
        int thread_index = gantt_events[i].thread_index;
        printf("  %s: [%d-%d] ms (Duration: %d ms)", 
               threads[thread_index].pid,
               gantt_events[i].start_time,
               gantt_events[i].start_time + gantt_events[i].duration,
               gantt_events[i].duration);
        
        if (i < gantt_event_count - 1) {
            if (gantt_events[i+1].start_time > gantt_events[i].start_time + gantt_events[i].duration) {
                printf("  [Context Switch: 0.010 ms]\n");
            } else {
                printf("\n");
            }
        } else {
            printf("\n");
        }
    }
    
    printf("\nNote: Each '-' character represents 1ms of execution time\n");
    printf("      Context switches (0.010ms) shown as gaps between processes\n");
    printf("      Processes may be preempted multiple times (shown as separate blocks)\n");
}
void run_windows_preemptive_priority_analysis() {
    printf("================================================================================\n");
    printf("PRIMECART RETAIL - WINDOWS POS FRONTEND PREEMPTIVE PRIORITY SCHEDULING ANALYSIS\n");
    printf("Windows 10 IoT Enterprise | Preemptive Priority Scheduling\n");
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
    
    printf("\nPriority Legend: 1=Highest (POS Tasks), 5=Lowest (Background)\n");
    printf("PREEMPTIVE: Higher priority tasks can interrupt lower priority tasks\n");
    
    // Reset thread states
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].remaining_time = threads[i].burst_time;
        threads[i].completed = 0;
        threads[i].start_time = -1;
        threads[i].response_time = -1;
        threads[i].first_response = 0;
    }
    
    // Execute Preemptive Priority Scheduling Simulation
    int current_time = 0;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;
    int total_response_time = 0;
    int total_burst_time = 0;
    int total_idle_time = 0;
    int completed_count = 0;
    int current_thread = -1;
    
    printf("\nEXECUTION TIMELINE (All times in milliseconds):\n");
    printf("================================================================================\n\n");
    
    // Initialize Gantt chart
    gantt_event_count = 0;
    
    while (completed_count < NUM_THREADS) {
        // Find highest priority thread that has arrived
        int next_thread = find_highest_priority_thread(current_time, current_thread);
        
        // Check if we need to switch threads (preemption or new thread)
        if (next_thread != -1) {
            if (current_thread == -1) {
                // No thread currently running, start new one
                current_thread = next_thread;
            }
            else if (threads[next_thread].priority < threads[current_thread].priority) {
                // PREEMPTION: Higher priority thread arrived
                printf("[Time %dms] PREEMPTION: %s (Priority %d) preempts %s (Priority %d)\n",
                       current_time,
                       threads[next_thread].pid, threads[next_thread].priority,
                       threads[current_thread].pid, threads[current_thread].priority);
                
                // End current Gantt event
                if (gantt_event_count > 0) {
                    gantt_events[gantt_event_count-1].duration = current_time - gantt_events[gantt_event_count-1].start_time;
                }
                
                // Add context switch overhead
                current_time += CONTEXT_SWITCH_WINDOWS;
                current_thread = next_thread;
            }
            else if (next_thread != current_thread && threads[next_thread].priority == threads[current_thread].priority) {
                // Same priority, check if new thread arrived earlier (shouldn't normally happen with FCFS)
                if (threads[next_thread].arrival_time < threads[current_thread].arrival_time) {
                    current_thread = next_thread;
                }
            }
        }
        
        if (current_thread == -1) {
            // CPU idle
            current_time++;
            total_idle_time++;
            continue;
        }
        
        // Record start time if first execution
        if (threads[current_thread].start_time == -1) {
            threads[current_thread].start_time = current_time;
        }
        
        // Record response time on first execution
        if (!threads[current_thread].first_response) {
            threads[current_thread].response_time = current_time - threads[current_thread].arrival_time;
            threads[current_thread].first_response = 1;
            printf("[Time %dms] First response for %s (Priority %d, Arrival: %dms, Response Time: %dms)\n",
                   current_time, threads[current_thread].pid, 
                   threads[current_thread].priority,
                   threads[current_thread].arrival_time,
                   threads[current_thread].response_time);
        }
        
        // Record Gantt chart event if new event
        if (gantt_event_count == 0 || 
            gantt_events[gantt_event_count-1].thread_index != current_thread ||
            current_time != gantt_events[gantt_event_count-1].start_time + gantt_events[gantt_event_count-1].duration) {
            gantt_events[gantt_event_count].thread_index = current_thread;
            gantt_events[gantt_event_count].start_time = current_time;
            gantt_events[gantt_event_count].duration = 0;
            gantt_event_count++;
        }
        
        // Execute for 1 time unit
        threads[current_thread].remaining_time--;
        gantt_events[gantt_event_count-1].duration++;
        current_time++;
        
        // Check if current thread completed
        if (threads[current_thread].remaining_time == 0) {
            threads[current_thread].completed = 1;
            threads[current_thread].exit_time = current_time;
            threads[current_thread].turnaround_time = threads[current_thread].exit_time - threads[current_thread].arrival_time;
            threads[current_thread].waiting_time = threads[current_thread].turnaround_time - threads[current_thread].burst_time;
            
            printf("[Time %dms] Completed %s (Priority %d)\n", 
                   current_time, threads[current_thread].pid, threads[current_thread].priority);
            printf("         Waiting Time: %dms | Response Time: %dms | Turnaround Time: %dms\n\n",
                   threads[current_thread].waiting_time,
                   threads[current_thread].response_time,
                   threads[current_thread].turnaround_time);
            
            // Accumulate totals
            total_waiting_time += threads[current_thread].waiting_time;
            total_turnaround_time += threads[current_thread].turnaround_time;
            total_response_time += threads[current_thread].response_time;
            total_burst_time += threads[current_thread].burst_time;
            
            completed_count++;
            current_thread = -1;
            
            // Add context switch overhead if more threads to run
            if (completed_count < NUM_THREADS) {
                current_time += CONTEXT_SWITCH_WINDOWS;
            }
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
        for (int i = 0; i < NUM_THREADS; i++) {
            if (threads[i].priority == prio) {
                printf("| %-3s | %-8d | %-8d | %-8d | %-8d | %-8d | %-10d | %-10d | %-10d |\n",
                       threads[i].pid,
                       threads[i].priority,
                       threads[i].arrival_time,
                       threads[i].burst_time,
                       threads[i].start_time,
                       threads[i].exit_time,
                       threads[i].response_time,
                       threads[i].waiting_time,
                       threads[i].turnaround_time);
            }
        }
    }
    printf("+-----+----------+----------+----------+----------+----------+------------+------------+------------+\n");
    
    // PrimeCart Threshold Analysis
    printf("\n================================================================================\n");
    printf("PRIMECART THRESHOLD ANALYSIS (Windows 10 IoT Enterprise)\n");
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
    
    // Context Switch Time - FIXED: Business impact explanation
    const char* cs_status = CONTEXT_SWITCH_WINDOWS <= 0.010 ? "PASS" : "FAIL";
    printf("Context Switch Time    < 0.010 ms          %-12s %-6s   Maintains smooth task switching during checkout\n", 
           "0.010 ms", cs_status);  // CHANGED EXPLANATION
    
    // CPU Utilization
    const char* cpu_status = cpu_utilization < 75.0 ? "PASS" : "FAIL";
    char cpu_value[20];
    sprintf(cpu_value, "%.1f%%", cpu_utilization);
    printf("CPU Utilization        < 75.0%%             %-12s %-6s   Leaves no headroom for background tasks during peak traffic\n", 
           cpu_value, cpu_status);
    
    // IPC Throughput
    const char* ipc_status = IPC_THROUGHPUT_WINDOWS > 500.0 ? "PASS" : "FAIL";
    char ipc_value[20];
    sprintf(ipc_value, "%.0f MB/s", IPC_THROUGHPUT_WINDOWS);
    printf("IPC Throughput         > 500 MB/s          %-12s %-6s   Enables fast communication between POS and backend systems\n", 
           ipc_value, ipc_status);
    
    printf("--------------------------------------------------------------------------------\n");    
    // Preemptive Priority Algorithm Analysis
    printf("\n================================================================================\n");
    printf("PREEMPTIVE PRIORITY SCHEDULING ALGORITHM ANALYSIS\n");
    printf("================================================================================\n");
    
    printf("\nKey Preemption Events Observed:\n");
    printf("1. P1 starts at 0ms (only thread available)\n");
    printf("2. P2 arrives at 1ms (Priority 1) → PREEMPTS P1 immediately at 1ms\n");
    printf("3. P3 arrives at 2ms (Priority 1) → Waits as P2 has same priority and arrived earlier\n");
    printf("4. P5 arrives at 5ms (Priority 2) → Waits for Priority 1 tasks to complete\n");
    printf("5. P7 arrives at 9ms (Priority 1) → Runs immediately after P3 completes\n");
    printf("6. Background tasks only run when no POS tasks are in queue\n");
    
    printf("\nImpact on PrimeCart Operations:\n");
    printf("✓ POS tasks get immediate CPU attention upon arrival\n");
    printf("✓ Critical POS functions (barcode scan, price lookup) respond instantly\n");
    printf("✓ Checkout operations never wait for background tasks\n");
    printf("✓ Better response times compared to non-preemptive priority\n");
    printf("✗ Background tasks may experience starvation during busy periods\n");
    printf("✗ More context switches due to frequent preemptions\n");
    
    printf("\n================================================================================\n");
    printf("ANALYSIS COMPLETE - PREEMPTIVE PRIORITY SCHEDULING\n");
    printf("================================================================================\n");
}

int main() {
    run_windows_preemptive_priority_analysis();
    return 0;
}
