#include "common.hpp"

void main_thread()
{
    // Разные частоты обновления для разных функций
    constexpr int WINDOW_CHECK_MS = 1000;     // Проверка окна раз в секунду
    constexpr int WEAPON_UPDATE_MS = 100;     // Оружие 10 раз в секунду
    constexpr int WANTED_UPDATE_MS = 500;     // Wanted level 2 раза в секунду

    auto last_window_check = std::chrono::steady_clock::now();
    auto last_weapon_update = std::chrono::steady_clock::now();
    auto last_wanted_update = std::chrono::steady_clock::now();

    // ИСПРАВЛЕНИЕ 4: Кешируем указатели вместо чтения каждый раз
    uintptr_t cached_ped = 0;
    auto last_cache_update = std::chrono::steady_clock::now();
    constexpr int CACHE_UPDATE_MS = 1000; // Обновляем кеш раз в секунду

    while (g_running)
    {
        auto now = std::chrono::steady_clock::now();

        // 1. Проверка окна игры (раз в секунду)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_window_check).count() >= WINDOW_CHECK_MS)
        {
            if (!FindWindowA("grcWindow", nullptr))
            {
                g_running = false;
                break;
            }
            last_window_check = now;
        }

        // Обновляем кешированный указатель на педа
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_cache_update).count() >= CACHE_UPDATE_MS)
        {
            cached_ped = RPM<uintptr_t>(CPedFactoryPointer, { 0x8 });
            last_cache_update = now;
        }

        // 2. Обновление оружия (10 раз в секунду)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_weapon_update).count() >= WEAPON_UPDATE_MS)
        {
            // ИСПРАВЛЕНИЕ 5: Объединяем чтение данных оружия
            bool need_weapon_update = g_settings->weapon.no_recoil ||
                g_settings->weapon.no_spread ||
                g_settings->weapon.one_shoot_kill;

            if (need_weapon_update && cached_ped)
            {
                // Читаем иерархию один раз
                uintptr_t CPedWeaponManager = RPM<uintptr_t>(cached_ped + 0x10B8);

                if (CPedWeaponManager)
                {
                    uintptr_t CWeaponInfo = RPM<uintptr_t>(CPedWeaponManager + 0x20);

                    if (CWeaponInfo)
                    {
                        // Применяем все модификации одновременно
                        if (g_settings->weapon.no_recoil)
                        {
                            WPM<int>(CWeaponInfo + 0x2E4, 0);
                            WPM<int>(CWeaponInfo + 0x2E8, 0);
                        }

                        if (g_settings->weapon.no_spread)
                        {
                            WPM<int>(CWeaponInfo + 0x74, 0);
                        }
                    }
                }

                // One shoot kill обрабатываем отдельно
                if (g_settings->weapon.one_shoot_kill)
                {
                    uintptr_t CPlayerInfo = RPM<uintptr_t>(cached_ped + 0x10A8);
                    if (CPlayerInfo)
                    {
                        WPM<float>(CPlayerInfo + 0xD0C, 999999);
                        WPM<float>(CPlayerInfo + 0xD18, 999999);
                    }
                }
            }
            else if (!g_settings->weapon.one_shoot_kill && cached_ped)
            {
                // Восстанавливаем нормальные значения
                static bool was_enabled = false;
                if (was_enabled)
                {
                    uintptr_t CPlayerInfo = RPM<uintptr_t>(cached_ped + 0x10A8);
                    if (CPlayerInfo)
                    {
                        WPM<float>(CPlayerInfo + 0xD0C, 1);
                        WPM<float>(CPlayerInfo + 0xD18, 1);
                        was_enabled = false;
                    }
                }
                else if (g_settings->weapon.one_shoot_kill)
                {
                    was_enabled = true;
                }
            }

            last_weapon_update = now;
        }

        // 3. Обновление Wanted level (2 раза в секунду)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_wanted_update).count() >= WANTED_UPDATE_MS)
        {
            if (g_settings->self.never_wanted && cached_ped)
            {
                uintptr_t CPlayerInfo = RPM<uintptr_t>(cached_ped + 0x10A8);
                if (CPlayerInfo)
                {
                    WPM<int>(CPlayerInfo + 0x888, 0);
                }
            }
            last_wanted_update = now;
        }

        // ИСПРАВЛЕНИЕ 6: Умный sleep
        int time1 = WINDOW_CHECK_MS - (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_window_check).count();
        int time2 = WEAPON_UPDATE_MS - (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_weapon_update).count();
        int time3 = WANTED_UPDATE_MS - (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_wanted_update).count();

        int next_update = time1 < time2 ? time1 : time2;
        next_update = next_update < time3 ? next_update : time3;

        if (next_update > 0)
        {
            int sleep_time = next_update < 50 ? next_update : 50;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Удаляем отдельные функции, так как они теперь интегрированы
// weapon_no_recoil(), weapon_no_spread(), weapon_one_shoot_kill(), self_never_wanted()