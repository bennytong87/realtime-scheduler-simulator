# Real-Time Scheduling Simulator

A collection of real-time scheduling algorithms and scheduling servers implemented in C.

## Overview

This project implements and simulates:

- **Periodic scheduling algorithms**: RM, EDF, LST
- **Scheduling servers for aperiodic tasks**: CUS, TBS

The simulator verifies schedulability, measures deadline misses, and evaluates average response time for mixed task sets.

## Features

- Rate Monotonic (RM) with Liu & Layland bound test
- Earliest Deadline First (EDF) with utilization <= 1 test
- Least Slack Time (LST)
- Constant Utilization Server (CUS)
- Total Bandwidth Server (TBS)
- Mixed periodic + aperiodic task simulation
- Deadline miss statistics and average response time

## Architecture

### Periodic Scheduler

    main
      -> read_tasks (parse input file)
      -> compute LCM and MaxPH
      -> simulate loop:
           check deadline misses
           release new jobs
           select highest priority
           execute one time unit
           Clock++
      -> print statistics

### Server Scheduler

    main
      -> read_files (periodic + aperiodic)
      -> simulate loop:
           check periodic misses
           release periodic jobs
           release aperiodic jobs + R2 replenish
           CUS: check R3 replenish
           EDF selection (server vs periodic)
           execute
           clock++
      -> print statistics

## Project Structure

    realtime-scheduler-simulator/
    +-- periodic_scheduler/     # RM, EDF, LST
    +-- server_scheduler/       # CUS, TBS
    +-- tests/                  # Test cases
    +-- docs/                   # Theory notes and results

## Build

    make

Or build individually:

    cd periodic_scheduler && make
    cd server_scheduler && make

## Run

### Periodic Scheduler

    ./periodic_scheduler/periodic_scheduler <task_file> <algorithm>
    # algorithm: 1=RM, 2=EDF, 3=LST

### Server Scheduler

    ./server_scheduler/server_scheduler <CUS|TBS> <periodic_file> <aperiodic_file>

## Input Formats

### Periodic task file (periodic_scheduler)

    Phase , Period , RelativeDeadline , WCET

Example:

    0 , 10 , 10 , 2
    0 , 15 , 15 , 3
    0 , 20 , 20 , 2

### Server periodic file (server_scheduler)

    Period , WCET

### Server aperiodic file (server_scheduler)

    ArrivalTime , ExecutionTime

## Test Results

### Periodic Scheduler

| Case | U | RM Miss | EDF Miss | LST Miss |
|------|---|---------|----------|----------|
| case1 | 0.50 | 0 | 0 | 0 |
| case2 | 1.30 | 1 | 1 | 2 |
| case3 | 0.65 | 0 | 0 | 0 |

### Server Scheduler

| Case | Server | Periodic Miss | Aperiodic Finished | Avg Response |
|------|--------|---------------|-------------------|--------------|
| case1 | CUS | 0 | 3 | 14.6667 |
| case1 | TBS | 0 | 3 | 12.0000 |
| case2 | CUS | 0 | 3 | 10.3333 |
| case2 | TBS | 0 | 3 | 6.0000 |

### Observations

- case2 (U=1.3) is overloaded; all algorithms miss deadlines as expected.
- LST misses one more job than EDF due to tie-breaking and extra context switches.
- TBS achieves lower average response time than CUS (12.0 vs 14.67, 6.0 vs 10.33) because it replenishes budget immediately after job completion.
- No periodic deadline misses in server scheduler, confirming the server does not steal bandwidth from periodic tasks.

## Theory

See docs/theory.md for:

- Real-time system basics
- EDF optimality proof (exchange argument)
- RM bound derivation
- CUS vs TBS

## What I Learned

- Implementing a scheduler makes EDF optimality concrete, not just a slogan
- Server budget replenishment timing is the core design decision
- Simulation validates theoretical bounds against actual behavior

## Relation to Industry

CIM system handles dispatching, scheduling, and automation in fabs.

| This project | CIM counterpart |
|---|---|
| EDF | Dispatch priority by due date |
| RM | Control tasks (interlock) prioritized over reports |
| CUS / TBS | Guarantee control task bandwidth while handling sensor data |
| Schedulability test | Pre-deployment worst-case verification |
| Simulator | Digital twin offline validation |

