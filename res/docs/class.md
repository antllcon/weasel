# Классы

> **Статус:** синтаксис парсится, семантика и кодогенерация **не реализованы**.

Классы — основной механизм пользовательских типов данных в Weasel. Структур (`struct`) в языке нет: `class` покрывает все сценарии.

---

## Планируемый синтаксис

```wes
class Matrix
{
    var uint rows
    var uint cols
    var array[int] data

    Matrix(uint rows, uint cols) { ... }

    void Fill(int value) { ... }
    int GetRows() { ... }
    int GetCols() { ... }
}
```

---

## Что уже готово в рантайме

- **Грамматика**: `ClassDecl` и `ClassBody` парсятся LALR-парсером.
- **AST-узел**: `StructDeclStmt` (переиспользуется для классов).
- **Семантика**: `Visit(StructDeclStmt)` регистрирует тип через `StructTypeInfo` и `TypeResolver::RegisterStruct`.
- **Опкоды VM**: `AllocateStruct`, `GetField N`, `StoreField N`, `CopyObject` — уже реализованы в виртуальной машине.

## Что нужно реализовать

1. Литерал класса: `MyClass { field = value, ... }` — не реализован в `CstToAstConverter`
2. Присваивание поля: `obj.field = value` — `AssignStmt` не обрабатывает `MemberAccessExpr`
3. Кодоген: подключить `GetField`/`StoreField` через `StructTypeInfo` в `CodeGenerator`
4. Методы: объявление, вызов, связывание с экземпляром
5. Конструкторы и деструкторы
