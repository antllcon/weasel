#include <gtest/gtest.h>
#include "src/vm/machine/VirtualMachine.h"

// Проверка базового вычитания чисел с плавающей точкой двойной точности
TEST(VirtualMachineTest, SubtractDoubleCorrectly)
{
    Chunk chunk;
    chunk.AddConstant(Value(10.5));
    chunk.AddConstant(Value(3.0));
    chunk.WriteOpCode(OpCode::Constant, 1);
    chunk.WriteByte(0, 1);
    chunk.WriteOpCode(OpCode::Constant, 2);
    chunk.WriteByte(1, 2);
    chunk.WriteOpCode(OpCode::SubtractDouble, 3);
    chunk.WriteOpCode(OpCode::Return, 4);

    VirtualMachine vm;
    vm.Interpret(chunk);

    EXPECT_DOUBLE_EQ(vm.Peek(0).AsDouble(), 7.5);
}

// Проверка генерации Infinity при делении на ноль (IEEE 754)
TEST(VirtualMachineTest, DivideByZeroYieldsInfinity)
{
    Chunk chunk;
    chunk.AddConstant(Value(42.0));
    chunk.AddConstant(Value(0.0));
    chunk.WriteOpCode(OpCode::Constant, 1);
    chunk.WriteByte(0, 1);
    chunk.WriteOpCode(OpCode::Constant, 2);
    chunk.WriteByte(1, 2);
    chunk.WriteOpCode(OpCode::DivideDouble, 3);
    chunk.WriteOpCode(OpCode::Return, 4);

    VirtualMachine vm;
    vm.Interpret(chunk);

    EXPECT_TRUE(std::isinf(vm.Peek(0).AsDouble()));
}

// Проверка переполнения типа float (Single) с уходом в бесконечность
TEST(VirtualMachineTest, SingleOverflowYieldsInfinity)
{
    Chunk chunk;
    const float maxSingle = std::numeric_limits<float>::max();
    chunk.AddConstant(Value(maxSingle));
    chunk.AddConstant(Value(maxSingle));
    chunk.WriteOpCode(OpCode::Constant, 1);
    chunk.WriteByte(0, 1);
    chunk.WriteOpCode(OpCode::Constant, 2);
    chunk.WriteByte(1, 2);
    chunk.WriteOpCode(OpCode::AddSingle, 3);
    chunk.WriteOpCode(OpCode::Return, 4);

    VirtualMachine vm;
    vm.Interpret(chunk);

    EXPECT_TRUE(std::isinf(vm.Peek(0).AsSingle()));
}

// Проверка wrap-around эффекта при переполнении знакового 32-битного числа
TEST(VirtualMachineTest, SNumberOverflowWrapsAround)
{
    Chunk chunk;
    const int32_t maxInt = std::numeric_limits<int32_t>::max();
    chunk.AddConstant(Value(maxInt));
    chunk.AddConstant(Value(1));
    chunk.WriteOpCode(OpCode::Constant, 1);
    chunk.WriteByte(0, 1);
    chunk.WriteOpCode(OpCode::Constant, 2);
    chunk.WriteByte(1, 2);
    chunk.WriteOpCode(OpCode::AddSNumber, 3);
    chunk.WriteOpCode(OpCode::Return, 4);

    VirtualMachine vm;
    vm.Interpret(chunk);

    EXPECT_EQ(vm.Peek(0).AsSNumber(), std::numeric_limits<int32_t>::min());
}

// Проверка выброса исключения при попытке извлечь значение из пустого стека
TEST(VirtualMachineTest, PopEmptyStackThrows)
{
    Chunk chunk;
    chunk.WriteOpCode(OpCode::AddDouble, 1);

    VirtualMachine vm;

    EXPECT_THROW(vm.Interpret(chunk), std::runtime_error);
}

// Проверка выброса исключения при обнаружении нереализованной инструкции
TEST(VirtualMachineTest, NotImplementedInstructionThrows)
{
    Chunk chunk;
    chunk.WriteOpCode(OpCode::Jump, 1);

    VirtualMachine vm;

    EXPECT_THROW(vm.Interpret(chunk), std::runtime_error);
}