# Test Results

## Periodic Scheduler

| Case | U | RM Miss | EDF Miss | LST Miss |
|------|---|---------|----------|----------|
| case1 | 0.50 | 0 | 0 | 0 |
| case2 | 1.30 | 1 | 1 | 2 |
| case3 | 0.65 | 0 | 0 | 0 |

### Case Details

**case1** (U = 0.50, schedulable):
- T1: Phase=0, Period=10, RDeadline=10, WCET=2
- T2: Phase=0, Period=15, RDeadline=15, WCET=3
- T3: Phase=0, Period=20, RDeadline=20, WCET=2

**case2** (U = 1.30, overloaded):
- T1: Phase=0, Period=5, RDeadline=5, WCET=3
- T2: Phase=0, Period=10, RDeadline=10, WCET=4
- T3: Phase=0, Period=20, RDeadline=20, WCET=6

**case3** (U = 0.65, schedulable, with phase offset):
- T1: Phase=2, Period=10, RDeadline=10, WCET=3
- T2: Phase=0, Period=15, RDeadline=15, WCET=4
- T3: Phase=5, Period=20, RDeadline=20, WCET=2

## Server Scheduler

| Case | Server | Periodic Miss | Aperiodic Finished | Avg Response |
|------|--------|---------------|-------------------|--------------|
| case1 | CUS | 0 | 3 | 14.6667 |
| case1 | TBS | 0 | 3 | 12.0000 |
| case2 | CUS | 0 | 3 | 10.3333 |
| case2 | TBS | 0 | 3 | 6.0000 |

### Case Details

**case1**:
- Periodic: P1(10,3), P2(15,4)
- Aperiodic: A1(arrival=0, exec=5), A2(arrival=20, exec=3), A3(arrival=35, exec=6)

**case2**:
- Periodic: P1(8,2), P2(12,3)
- Aperiodic: A1(arrival=5, exec=4), A2(arrival=18, exec=2), A3(arrival=30, exec=5)

### Analysis

1. **TBS outperforms CUS in average response time**:
   - case1: TBS 12.0 vs CUS 14.67 (18% faster)
   - case2: TBS 6.0 vs CUS 10.33 (42% faster)
   - Reason: TBS replenishes budget immediately when a job completes, reducing waiting time for the next aperiodic job.

2. **No periodic deadline misses**:
   - The server correctly reserves bandwidth (us = 0.2) and does not steal CPU from periodic tasks.
   - This confirms the server's isolation property.

3. **LST vs EDF on overloaded case2**:
   - LST misses 2 jobs, EDF misses 1.
   - LST's tie-breaking and context switch behavior cause extra overhead.
   - EDF remains the better choice for overloaded systems.
