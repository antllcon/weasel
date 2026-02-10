#include "src/timer/Timer.h"
#include <gtest/gtest.h>

using namespace testing;

// Проверка корректности подсчета прошедшего времени
TEST(TimerTest, ElapsedTimeIsPositive)
{
	const Timer timer;
	EXPECT_GE(timer.Elapsed(), 0.0);
}