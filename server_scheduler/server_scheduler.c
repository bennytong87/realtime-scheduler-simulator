/*
 * server_scheduler.c - Scheduling server simulator for CUS and TBS
 *
 * Servers:
 *   - Constant Utilization Server (CUS)
 *   - Total Bandwidth Server (TBS)
 *
 * Input:
 *   - periodic file: Period , WCET
 *   - aperiodic file: ArrivalTime , ExecutionTime
 *
 * Usage: ./server_scheduler <CUS|TBS> <periodic_file> <aperiodic_file>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    int tid;
    int period;
    int wcet;
} Task;

typedef struct Job {
    int tid;
    int deadline;
    int remain;
    struct Job *next;
} Job;

typedef struct AJob {
    int arrival;
    int remain;
    int exe_time;
    struct AJob *next;
} AJob;

typedef struct {
    int arrival;
    int remain;
} AInputData;

Task P_Tasks[100];
int P_Count = 0;
AInputData *A_Input = NULL;
int A_Input_Count = 0;

/**
 * compare_ajobs - qsort comparator for aperiodic input by arrival time.
 */
int compare_ajobs(const void *a, const void *b) {
    AInputData *ia = (AInputData *)a;
    AInputData *ib = (AInputData *)b;
    return ia->arrival - ib->arrival;
}

/**
 * read_files - Read periodic and aperiodic task files.
 */
void read_files(const char* p_path, const char* a_path) {
    FILE *fp = fopen(p_path, "r");
    P_Count = 0;
    if (fp) {
        while (fscanf(fp, "%d,%d", &P_Tasks[P_Count].period, &P_Tasks[P_Count].wcet) != EOF) {
            P_Tasks[P_Count].tid = P_Count + 1;
            P_Count++;
        }
        fclose(fp);
    } else {
        printf("Error: Cannot open periodic file at %s\n", p_path);
        exit(1);
    }

    fp = fopen(a_path, "r");
    A_Input_Count = 0;
    if (fp) {
        int a, e;
        while (fscanf(fp, "%d,%d", &a, &e) != EOF) {
            A_Input_Count++;
            A_Input = realloc(A_Input, sizeof(AInputData) * A_Input_Count);
            A_Input[A_Input_Count-1].arrival = a;
            A_Input[A_Input_Count-1].remain = e;
        }
        fclose(fp);
        qsort(A_Input, A_Input_Count, sizeof(AInputData), compare_ajobs);
    } else {
        printf("Error: Cannot open aperiodic file at %s\n", a_path);
        exit(1);
    }
}

/**
 * simulate - Run CUS or TBS server simulation.
 */
void simulate(int mode) {
    int clock = 0;
    int total_p = 0, miss_p = 0, finished_a = 0;
    long long total_resp = 0;
    int a_idx = 0;

    Job *PQ = NULL;
    AJob *AQ = NULL;

    int s_deadline = 0;
    int s_budget = 0;
    const int max_sim_time = 1000;
    const double us = 0.2;

    while (clock <= max_sim_time) {
        Job *curr_p = PQ, *prev_p = NULL;
        while (curr_p) {
            if (curr_p->deadline - clock - curr_p->remain <= 0) {
                miss_p++;
                Job *tmp = curr_p;
                if (prev_p) prev_p->next = curr_p->next; else PQ = curr_p->next;
                curr_p = curr_p->next;
                free(tmp);
            } else {
                prev_p = curr_p; curr_p = curr_p->next;
            }
        }

        for (int i = 0; i < P_Count; i++) {
            if (clock % P_Tasks[i].period == 0 && clock < max_sim_time) {
                Job *nj = malloc(sizeof(Job));
                nj->tid = P_Tasks[i].tid;
                nj->remain = P_Tasks[i].wcet;
                nj->deadline = clock + P_Tasks[i].period;
                nj->next = PQ; PQ = nj;
                total_p++;
            }
        }

        while (a_idx < A_Input_Count && A_Input[a_idx].arrival == clock) {
            AJob *nj = malloc(sizeof(AJob));
            nj->arrival = A_Input[a_idx].arrival;
            nj->remain = A_Input[a_idx].remain;
            nj->exe_time = A_Input[a_idx].remain;
            nj->next = NULL;

            int was_empty = (AQ == NULL);
            if (!AQ) AQ = nj;
            else { AJob *t = AQ; while(t->next) t=t->next; t->next = nj; }

            if (was_empty) {
                if (mode == 1) {
                    if (clock >= s_deadline) {
                        s_budget = AQ->remain;
                        s_deadline = clock + (int)(s_budget / us);
                    }
                } else {
                    s_budget = AQ->remain;
                    int base = (s_deadline > clock) ? s_deadline : clock;
                    s_deadline = base + (int)(s_budget / us);
                }
            }
            a_idx++;
        }

        if (mode == 1 && AQ && clock == s_deadline) {
            s_budget = AQ->remain;
            s_deadline = clock + (int)(s_budget / us);
        }

        if (!AQ) s_budget = 0;

        if (clock == max_sim_time) break;

        Job *best_p = NULL;
        Job *it = PQ;
        while (it) {
            if (!best_p || it->deadline < best_p->deadline ||
               (it->deadline == best_p->deadline && it->tid < best_p->tid)) {
                best_p = it;
            }
            it = it->next;
        }

        int run_server = (AQ && s_budget > 0 && (best_p == NULL || s_deadline <= best_p->deadline));

        if (run_server) {
            AQ->remain--;
            s_budget--;
            if (AQ->remain == 0) {
                finished_a++;
                total_resp += (clock + 1 - AQ->arrival);
                AJob *tmp = AQ;
                AQ = AQ->next;
                free(tmp);

                if (mode == 2 && AQ) {
                    s_budget = AQ->remain;
                    s_deadline += (int)(s_budget / us);
                } else {
                    s_budget = 0;
                }
            }
        } else if (best_p) {
            best_p->remain--;
            if (best_p->remain == 0) {
                Job *c = PQ, *p = NULL;
                while (c != best_p) { p = c; c = c->next; }
                if (p) p->next = c->next; else PQ = c->next;
                free(c);
            }
        }
        clock++;
    }

    printf("RESULT_MODE=%s\n", mode == 1 ? "CUS" : "TBS");
    printf("RESULT_TOTAL_P=%d\n", total_p);
    printf("RESULT_MISS_P=%d\n", miss_p);
    printf("RESULT_FINISHED_A=%d\n", finished_a);
    printf("RESULT_AVG_RESP=%.4f\n", finished_a > 0 ? (double)total_resp / finished_a : 0);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <CUS|TBS> <periodic_file> <aperiodic_file>\n", argv[0]);
        return 0;
    }

    read_files(argv[2], argv[3]);

    if (strcmp(argv[1], "CUS") == 0) {
        simulate(1);
    } else if (strcmp(argv[1], "TBS") == 0) {
        simulate(2);
    } else {
        printf("Invalid Mode! Please choose CUS or TBS.\n");
    }

    if (A_Input) free(A_Input);

    return 0;
}
