/*
 * periodic_scheduler.c - Real-time periodic scheduling simulator
 *
 * Algorithms:
 *   - Rate Monotonic (RM)
 *   - Earliest Deadline First (EDF)
 *   - Least Slack Time (LST)
 *
 * Input format: Phase , Period , RelativeDeadline , WCET
 *
 * Build: gcc -Wall -Wextra -O2 -o periodic_scheduler periodic_scheduler.c -lm
 * Usage: ./periodic_scheduler <task_file> <algorithm>
 *        algorithm: 1=RM, 2=EDF, 3=LST
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ===== Structures ===== */

/*
 * Task - A periodic task specification.
 *
 * @TID:          task ID (1-based)
 * @Phase:        first release time
 * @Period:       interval between consecutive releases
 * @RDeadline:    relative deadline (from release)
 * @WCET:         worst-case execution time
 * @Utilization:  WCET / Period
 */
typedef struct {
    int TID;
    int Phase;
    int Period;
    int RDeadline;
    int WCET;
    float Utilization;
} Task;

/*
 * Job - A single released instance of a task.
 *
 * @TID:                    owning task ID
 * @release_time:           time this job became ready
 * @remain_execution_time:  remaining WCET
 * @absolute_deadline:      release_time + RDeadline
 * @next:                   linked list pointer
 */
typedef struct Job {
    int TID;
    int release_time;
    int remain_execution_time;
    int absolute_deadline;
    struct Job *next;
} Job;

/* ===== Global Variables ===== */

Task *Task_List = NULL;
int Task_Count = 0;
int LCM_value = 0;
int MaxPH = 0;
Job *Ready_Queue_Head = NULL;
int Clock = 0;
int Total_Job_Number = 0;
int Miss_Deadline_Job_Number = 0;

/* ===== Math Helpers ===== */

/*
 * gcd - Compute greatest common divisor (Euclid's algorithm).
 */
int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b); }

/*
 * lcm_int - Compute least common multiple of two integers.
 */
int lcm_int(int a, int b) { return (a * b) / gcd(a, b); }

/* ===== Task Loading ===== */

/*
 * read_tasks - Read task specifications from a file.
 *
 * File format: "Phase , Period , RelativeDeadline , WCET" per line.
 * Computes LCM of all periods (used as simulation length).
 */
void read_tasks(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open %s\n", filename);
        exit(1);
    }

    char line[128];
    Task_Count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strlen(line) > 1)
            Task_Count++;
    }
    rewind(fp);

    Task_List = (Task *)malloc(sizeof(Task) * Task_Count);

    for (int i = 0; i < Task_Count; i++) {
        fscanf(fp, "%d , %d , %d , %d",
               &Task_List[i].Phase,
               &Task_List[i].Period,
               &Task_List[i].RDeadline,
               &Task_List[i].WCET);
        Task_List[i].TID = i + 1;
        Task_List[i].Utilization = (float)Task_List[i].WCET / Task_List[i].Period;
        if (Task_List[i].Phase > MaxPH)
            MaxPH = Task_List[i].Phase;
    }

    LCM_value = Task_List[0].Period;
    for (int i = 1; i < Task_Count; i++)
        LCM_value = lcm_int(LCM_value, Task_List[i].Period);

    fclose(fp);
}

/* ===== Ready Queue ===== */

/*
 * create_job - Allocate a new job instance from a task.
 */
Job *create_job(Task *t, int now) {
    Job *j = (Job *)malloc(sizeof(Job));
    j->TID = t->TID;
    j->release_time = now;
    j->remain_execution_time = t->WCET;
    j->absolute_deadline = now + t->RDeadline;
    j->next = NULL;
    Total_Job_Number++;
    return j;
}

/*
 * insert_job - Insert job at the head of the ready queue (O(1)).
 */
void insert_job(Job *j) {
    j->next = Ready_Queue_Head;
    Ready_Queue_Head = j;
}

/*
 * delete_job - Remove a job from the ready queue.
 *
 * @target: job to remove
 * @prev:   node before target (NULL if target is head)
 */
void delete_job(Job *target, Job *prev) {
    if (!target) return;
    if (prev == NULL)
        Ready_Queue_Head = target->next;
    else
        prev->next = target->next;
    free(target);
}

/* ===== Priority Selection ===== */

/*
 * find_highest_priority_job - Select the highest-priority ready job.
 *
 * @algorithm: 1=RM, 2=EDF, 3=LST
 *
 * Ties are broken by TID (lower TID wins) for deterministic output.
 *
 * Returns the selected job, or NULL if no job has remaining work.
 */
Job *find_highest_priority_job(int algorithm) {
    if (!Ready_Queue_Head) return NULL;

    Job *cur = Ready_Queue_Head;
    Job *best = NULL;

    while (cur) {
        if (cur->remain_execution_time > 0) {
            best = cur;
            break;
        }
        cur = cur->next;
    }
    if (!best) return NULL;

    cur = best->next;
    while (cur) {
        if (cur->remain_execution_time <= 0) {
            cur = cur->next;
            continue;
        }

        int better = 0;
        Task *tc = &Task_List[cur->TID - 1];
        Task *tb = &Task_List[best->TID - 1];

        if (algorithm == 1) { /* RM: shorter period wins */
            if (tc->Period < tb->Period)
                better = 1;
            else if (tc->Period == tb->Period && cur->TID < best->TID)
                better = 1;
        } else if (algorithm == 2) { /* EDF: earlier deadline wins */
            if (cur->absolute_deadline < best->absolute_deadline)
                better = 1;
            else if (cur->absolute_deadline == best->absolute_deadline && cur->TID < best->TID)
                better = 1;
        } else if (algorithm == 3) { /* LST: smaller slack wins */
            int slack_c = cur->absolute_deadline - Clock - cur->remain_execution_time;
            int slack_b = best->absolute_deadline - Clock - best->remain_execution_time;
            if (slack_c < slack_b)
                better = 1;
            else if (slack_c == slack_b && cur->TID < best->TID)
                better = 1;
        }

        if (better)
            best = cur;
        cur = cur->next;
    }
    return best;
}

/* ===== Schedulability Tests ===== */

/*
 * rm_test - Liu & Layland bound test for Rate Monotonic.
 *
 * Schedulable if U <= n * (2^(1/n) - 1).
 */
void rm_test() {
    float U = 0;
    for (int i = 0; i < Task_Count; i++) U += Task_List[i].Utilization;
    float Bound = Task_Count * (powf(2.0, 1.0 / Task_Count) - 1);
    printf("\n--- RM Schedulability Test ---\n");
    printf("U = %.3f, Bound = %.3f\n", U, Bound);
    printf((U <= Bound) ? "Schedulable under RM.\n"
                        : "May not be schedulable under RM.\n");
}

/*
 * edf_test - Utilization test for Earliest Deadline First.
 *
 * Schedulable if U <= 1.0.
 */
void edf_test() {
    float U = 0;
    for (int i = 0; i < Task_Count; i++) U += Task_List[i].Utilization;
    printf("\n--- EDF Schedulability Test ---\n");
    printf("U = %.3f\n", U);
    printf((U <= 1.0) ? "Schedulable under EDF.\n"
                      : "May not be schedulable under EDF.\n");
}

/* ===== Simulation ===== */

/*
 * simulate - Run the scheduling simulation from t=0 to LCM + MaxPH.
 *
 * Each tick:
 *   1. Check for deadline misses and remove those jobs.
 *   2. Release new jobs whose period boundary matches the clock.
 *   3. Select the highest-priority job.
 *   4. Execute it for one tick.
 */
void simulate(int algorithm) {
    if (algorithm == 1)
        rm_test();
    else if (algorithm == 2)
        edf_test();
    else
        printf("\n--- LST Simulation ---\n");

    int end_time = LCM_value + MaxPH;
    Job *Current = NULL;
    printf("\nSimulation Start: 0 to %d\n", end_time - 1);

    while (Clock < end_time) {

        /* ===== Deadline miss check ===== */
        Job *cur = Ready_Queue_Head;
        Job *prev = NULL;
        while (cur) {
            if ((cur->absolute_deadline - Clock - cur->remain_execution_time) < 0) {
                printf("Clock %d: !!! T%d Missed Deadline (D_abs=%d) !!!\n",
                       Clock, cur->TID, cur->absolute_deadline);
                Miss_Deadline_Job_Number++;
                if (cur == Current) Current = NULL;
                Job *tmp = cur;
                cur = cur->next;
                delete_job(tmp, prev);
                continue;
            }
            prev = cur;
            cur = cur->next;
        }

        /* ===== New job releases ===== */
        int new_arrival = 0;
        for (int i = 0; i < Task_Count; i++) {
            Task *t = &Task_List[i];
            if (Clock >= t->Phase && (Clock - t->Phase) % t->Period == 0) {
                Job *j = create_job(t, Clock);
                insert_job(j);
                new_arrival = 1;
            }
        }

        if (new_arrival || Current == NULL)
            Current = find_highest_priority_job(algorithm);

        /* ===== Execute one tick ===== */
        printf("%d ", Clock);
        if (Current) {
            printf("T%d\n", Current->TID);
            Current->remain_execution_time--;

            if (Current->remain_execution_time == 0) {
                printf("Clock %d: T%d Job Finished.\n", Clock, Current->TID);
                Job *ptr = Ready_Queue_Head, *pre = NULL;
                while (ptr) {
                    if (ptr == Current) {
                        delete_job(ptr, pre);
                        break;
                    }
                    pre = ptr;
                    ptr = ptr->next;
                }
                Current = NULL;
            }
        } else {
            printf("Idle\n");
        }

        Clock++;
    }

    printf("\nSimulation End. Total Jobs: %d, Missed Deadlines: %d\n",
           Total_Job_Number, Miss_Deadline_Job_Number);
}

/* ===== Main ===== */

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./periodic_scheduler <task_file> <algorithm_id>\n");
        printf("Algorithm: 1=RM, 2=EDF, 3=LST\n");
        return 1;
    }
    read_tasks(argv[1]);
    printf("\n--- Simulation for %s ---\n", argv[1]);
    printf("Tasks Loaded: %d, LCM=%d, MaxPH=%d\n",
           Task_Count, LCM_value, MaxPH);
    simulate(atoi(argv[2]));
    free(Task_List);
    return 0;
}
