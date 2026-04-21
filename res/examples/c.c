number <- CalculateFibonacci(number targetIndex)
{
	if (targetIndex <= 0)
	{
		return 0;
	}

	if (targetIndex == 1)
	{
		return 1;
	}

	number previousFirst = 0;
	number previousSecond = 1;
	number currentValue = 0;

	for (number i = 2; i <= targetIndex; i++)
	{
		currentValue = previousFirst + previousSecond;
		previousFirst = previousSecond;
		previousSecond = currentValue;
	}

	return currentValue;
}

??? ??? ??? ??? ??? ???
???
	??? number ??? = 7;
	??? number finalResult = CalculateFibonacci(??? ???
	return finalResult;
???