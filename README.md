# HW1: расширение Third Person Template

Unreal Engine **5.8.3**, C++, Windows. Проект расширяет Third Person Template спринтом, взаимодействием через делегат, отдельным состоянием анимации Sprint и выносливостью.

## Что сделано

- `AHomeworkCharacter` расширяет шаблонный `AHW1Character`, который наследуется от `ACharacter`. `BP_ThirdPersonCharacter` переподключён к новому C++-классу.
- Enhanced Input: `IA_Sprint` / Left Shift и `IA_Interact` / E добавлены в существующий `IMC_Default`; обработчики привязаны через `BindAction` в C++.
- Dynamic Multicast Delegate `OnInteraction`: C++ вызывает `Broadcast`, Blueprint подписывается в BeginPlay и выполняет `Print String` с текстом `Interaction triggered!`.
- Спринт меняет `MaxWalkSpeed` с 500 на 900 см/с; `bIsSprinting` доступен Blueprint только для чтения.
- В стандартном `ABP_Unarmed`, в машине `Locomotion`, добавлено состояние `Sprint` с обычной анимацией бега и переходами по `bIsSprinting`.
- Выносливость 0-100: расход 25/с, восстановление 20/с с задержкой 0,8 с; при истощении спринт прекращается даже при удержании Shift. Значение и шкала отображаются в HUD.
- Интерфейс выполнен в UMG: `Content/ThirdPerson/UI/WBP_StaminaHUD`. Разметка редактируется во вкладке Designer, число, полоса и состояние обновляются Blueprint-привязками. Виджет добавляет локальный `BP_UMGPlayerController`; на стартовом уровне выбран `BP_UMGGameMode`.

## Управление

| Клавиша | Действие |
|---|---|
| W / A / S / D | Перемещение |
| Мышь | Поворот камеры |
| Space | Прыжок |
| Left Shift, удерживать | Спринт |
| E | Взаимодействие: `Interaction triggered!` |
| Esc | Завершить Play In Editor |
