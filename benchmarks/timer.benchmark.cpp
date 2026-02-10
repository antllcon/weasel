#include "src/timer/Timer.h"
#include <benchmark/benchmark.h>

// Замер производительности сброса таймера
static void BMRestart(benchmark::State& state)
{
	Timer timer;
	for (auto _ : state)
	{
		timer.Restart();
		benchmark::DoNotOptimize(timer);
	}
}
BENCHMARK(BMRestart);