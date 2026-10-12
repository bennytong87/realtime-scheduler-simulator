# Real-Time Scheduling Theory

## 1. Real-Time Systems

Correctness depends on both logical result and time.

| Type | Miss consequence | Example |
|------|-----------------|---------|
| Hard | Catastrophic | Interlock, ABS |
| Firm | Result useless, no catastrophe | Live streaming |
| Soft | Quality degradation | Video playback |

## 2. Task vs Job

| | Task | Job |
|---|---|---|
| Nature | Static | Dynamic |
| Fields | Period, WCET, RDeadline, Phase | release_time, abs_deadline, remain |

## 3. Utilization

U = sum(WCET_i / Period_i)

- U > 1 -> unschedulable
- U <= 1 -> possibly schedulable

## 4. Algorithms

### RM (Rate Monotonic)
- Fixed priority: shorter period -> higher priority
- Liu & Layland bound: U <= n(2^(1/n) - 1)
- n -> infinity, bound -> ln2 ~ 0.693

### EDF (Earliest Deadline First)
- Dynamic priority: earlier absolute deadline -> higher priority
- Optimal on uniprocessor
- Schedulability: U <= 1

### LST (Least Slack Time)
- slack = abs_deadline - current_time - remaining_execution
- Smaller slack -> higher priority

## 5. EDF Optimality (Exchange Argument)

Assume a schedulable schedule S that does not follow EDF at time t.
Swap the job with later deadline with the one with earlier deadline.
The earlier-deadline job becomes safer; the later-deadline job still meets its deadline.
Repeat to obtain EDF schedule. Hence EDF is optimal.

## 6. Scheduling Servers

### CUS (Constant Utilization Server)
- R1: server has budget and deadline
- R2: when queue becomes non-empty, replenish: d = t + C/us
- R3: when deadline reached and work remains, replenish again

### TBS (Total Bandwidth Server)
- R2: when queue becomes non-empty, replenish: d = max(d_old, t) + C/us
- On completion: immediately compute deadline for next job
- No R3 needed

| | CUS | TBS |
|---|---|---|
| Replenish timing | Queue non-empty OR deadline | Queue non-empty AND completion |
| Deadline formula | t + C/us | max(d, t) + C/us |
| Latency | Higher | Lower |
