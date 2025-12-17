#include "common.hpp"

void trigger_bot_thread()
{
    constexpr float MIN_HEALTH = 100.0f;
    bool was_shooting = false;

    // ИСПРАВЛЕНИЕ 17: Кешируем handle окна
    HWND game_window = FindWindowA("grcWindow", 0);
    auto last_window_check = std::chrono::steady_clock::now();

    while (g_running)
    {
        // Обновляем handle окна раз в секунду
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_window_check).count() >= 1000)
        {
            game_window = FindWindowA("grcWindow", 0);
            last_window_check = now;
        }

        // Быстрая проверка условий
        if (!g_settings->weapon.trigger_bot)
        {
            // Если был выстрел - отпускаем кнопку
            if (was_shooting)
            {
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                was_shooting = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // ИСПРАВЛЕНИЕ 18: Проверяем фокус окна БЕЗ вызова FindWindow каждый раз
        if (game_window != GetForegroundWindow())
        {
            if (was_shooting)
            {
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                was_shooting = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // ИСПРАВЛЕНИЕ 19: Читаем указатель на цель
        uintptr_t aimed_ped = RPM<uintptr_t>(AimCPedPointer);

        // Проверяем валидность цели
        if (aimed_ped != 0)
        {
            // Читаем здоровье
            float health = RPM<float>(aimed_ped + 0x280);

            // Стреляем только если здоровье >= 100
            if (health >= MIN_HEALTH)
            {
                if (!was_shooting)
                {
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    was_shooting = true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
        }

        // Отпускаем кнопку если нет валидной цели
        if (was_shooting)
        {
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            was_shooting = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Гарантируем отпускание кнопки при выходе
    if (was_shooting)
    {
        mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    }
}