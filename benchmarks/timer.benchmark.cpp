#include "src/timer/ScopedTimer.h"

#include <benchmark/benchmark.h>
#include <iostream>

// Замер производительности сброса таймера
static void BMRestart(benchmark::State& state)
{
	ScopedTimer timer("Benchmark", std::cout);
	for (auto _ : state)
	{
		timer.Restart();
		benchmark::DoNotOptimize(timer);
	}
}
BENCHMARK(BMRestart);