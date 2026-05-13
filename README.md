# Airport Performance Metrics — Amdahl & Gustafson Analysis

Performance benchmarking and scalability analysis of the airport 
concurrency simulation, measuring speedup and efficiency using 
Amdahl's Law and Gustafson-Barsis Law.

## Built with
- C
- Pthreads
- POSIX Semaphores
- CLOCK_MONOTONIC (wall clock time measurement)

## About
Extends the airport concurrency simulation with instrumented 
performance metrics. The program accepts runtime arguments to 
test scalability across different configurations of runways, 
baggage belt size, and ground crew workers.

Supports both serial and parallel execution modes for accurate 
Tserial reference measurements.

## Compilation
```bash
make
```

## Clean
```bash
make clean
```

## Usage
```bash
./metricas_rendimiento <TOTAL_VUELOS> <PISTAS_DISPONIBLES> <TAMANO_BANDA> <PERSONAL_TIERRA> <MODO_SERIAL>
```

Example — parallel mode with 1000 flights and 4 runways:
```bash
./metricas_rendimiento 1000 4 50 4 0
```

Example — serial reference measurement:
```bash
./metricas_rendimiento 1000 1 50 1 1
```

## Metrics measured
- Initialization time (memory allocation, semaphore setup)
- Parallel execution time (thread create to thread join)
- Destruction time (semaphore and mutex cleanup)
- Pure serial fraction (init + destroy)

## Scalability tests
- Fixed 1000 flights, varying p = 1, 2, 4, 8, 16 runways
- Speedup S = Tserial / Tparalelo
- Efficiency E = S / p
- Amdahl's Law — theoretical maximum speedup with infinite runways
- Gustafson-Barsis — scaled workload efficiency analysis

## Debug mode
Enable verbose logging by setting `#define DEBUG 1` at the top 
of the file.

## Author
- Wilson Chavarría Miranda
- Fabricio Padilla Madrigal