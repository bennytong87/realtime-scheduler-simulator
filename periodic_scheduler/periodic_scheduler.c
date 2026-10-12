/*
 * periodic_scheduler.c - Real-time periodic scheduling simulator
 *
 * Algorithms:
 *   - Rate Monotonic (RM)
 *   - Earliest Deadline First (EDF)
 *   - Least Slack Time (LST)
 *
 * Input format: Phase , Period , RelativeDeadline , WCET
 * Usage: ./periodic_scheduler <task_file> <algorithm>
 *        algorithm: 1=RM, 2=EDF, 3=LST
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ===== Structures ===== */

typedef struct {
    int TID;
    int Phase;
    int Period;
    int RDeadline;
    int WCET;
    float Utilization;
} Task;

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

/**
 * gcd - Compute greatest common divisor.
 */
int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b); }

/**
 * lcm_int - Compute least common multiple of two integers.
 */
int lcm_int(int a, int b) { return (a * b) / gcd(a, b); }

/* ===== Task Loading ===== */

/**
 * read_tasks - Read task specifications from file.
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

/**
 * create_job - Create a new job instance from a task.
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

/**
 * insert_job - Insert job into ready queue (at head).
 */
void insert_job(Job *j) {
    j->next = Ready_Queue_Head;
    Ready_Queue_Head = j;
}

/**
 * delete_job - Remove a job from ready queue.
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

/**
 * find_highest_priority_job - Select highest priority job.
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

        if (algorithm == 1) { /* RM */
            if (tc->Period < tb->Period)
                better = 1;
            else if (tc->Period == tb->Period && cur->TID < best->TID)
                better = 1;
        } else if (algorithm == 2) { /* EDF */
            if (cur->absolute_deadline < best->absolute_deadline)
                better = 1;
            else if (cur->absolute_deadline == best->absolute_deadline && cur->TID < best->TID)
                better = 1;
        } else if (algorithm == 3) { /* LST */
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

/**
 * rm_test - Liu & Layland bound test for RM.
 */
void rm_test() {
    float U = 0;
    for (int i = 0; i < Task_Count; i++) U += Task_List[i].Utilization;
    float Bound = Task_Count * (powf(2.0, 1.0 / Task_Count) - 1);
    printf("\n--- RM Schedulability Test ---\n");
    printf("U = %.3f, Bound = %.3f\n", U, Bound);
    printf((U <= Bound) ? "Schedulable under RM.\n" : "May not be schedulable under RM.\n");
}

/**
 * edf_test - Utilization test for EDF.
 */
void edf_test() {
    float U = 0;
    for (int i = 0; i < Task_Count; i++) U += Task_List[i].Utilization;
    printf("\n--- EDF Schedulability Test ---\n");
    printf("U = %.3f\n", U);
    printf((U <= 1.0) ? "Schedulable under EDF.\n" : "May not be schedulable under EDF.\n");
}

/* ===== Simulation ===== */

/**
 * simulate - Run the scheduling simulation.
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
    printf("Tasks Loaded: %d, LCM=%d, MaxPH=%d\n", Task_Count, LCM_value, MaxPH);
    simulate(atoi(argv[2]));
    free(Task_List);
    return 0;
}
