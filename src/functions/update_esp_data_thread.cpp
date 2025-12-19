#include "common.hpp"

void update_esp_data_thread()
{
    // Настройки частоты обновления
    constexpr int CRITICAL_UPDATE_MS = 16;    // ~60 FPS - матрица, позиция игрока
    constexpr int ENTITY_UPDATE_MS = 50;      // ~20 FPS - позиции entities
    constexpr int TEXT_UPDATE_MS = 100;       // ~10 FPS - текстовая информация

    auto last_critical = std::chrono::steady_clock::now();
    auto last_entity = std::chrono::steady_clock::now();
    auto last_text = std::chrono::steady_clock::now();

    while (g_running)
    {
        if (!g_settings->esp.enable) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        auto now = std::chrono::steady_clock::now();

        // Получаем буфер для записи
        GameState& state = g_game_state.get_write_buffer();
        bool state_changed = false;

        // ========================================
        // 1. КРИТИЧНЫЕ ДАННЫЕ (~60 FPS)
        // ========================================
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_critical).count() >= CRITICAL_UPDATE_MS)
        {
            // Матрица проекции
            ReadProcessMemory(g_handle,
                LPCVOID(RPM<uintptr_t>(CViewportGamePointer) + 0x250),
                state.view_matrix, 16 * 4, 0);

            // Размеры окна
            state.screen_width = RPM<int>(WindowWidth);
            state.screen_height = RPM<int>(WindowWidth + 4);

            // Позиция игрока
            state.self_ped = get_self_ped();
            state.self_pos = get_instance_world_coords(state.self_ped);

            state.last_critical_update = now;
            last_critical = now;
            state_changed = true;
        }

        // ========================================
        // 2. ПОЗИЦИИ ENTITIES (~20 FPS)
        // ========================================
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_entity).count() >= ENTITY_UPDATE_MS)
        {
            // Читаем актуальные счетчики
            state.num_vehicles = RPM<int>(ReplayInterfacePointer, { 0x10, 0x190 });
            state.num_peds = RPM<int>(ReplayInterfacePointer, { 0x18, 0x110 });
            state.num_pickups = RPM<int>(ReplayInterfacePointer, { 0x20, 0x110 });
            state.num_objects = RPM<int>(ReplayInterfacePointer, { 0x28, 0x168 });

            // === VEHICLES ===
            if (g_settings->esp.vehicle.enable)
            {
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x10 });
                uintptr_t list = RPM<uintptr_t>(face + 0x180);

                int limit = (state.num_vehicles < 300) ? state.num_vehicles : 300;
                state.vehicles.resize(limit);

                for (int i = 0; i < limit; i++)
                {
                    auto& veh = state.vehicles[i];
                    veh.instance = RPM<uintptr_t>(list + 0x10 * i);

                    if (veh.instance) {
                        veh.world_coords = get_instance_world_coords(veh.instance);
                        veh.last_update = now;
                    }
                }
            }
            else {
                state.vehicles.clear();
            }

            // === PEDS ===
            if (g_settings->esp.ped.enable)
            {
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x18 });
                uintptr_t list = RPM<uintptr_t>(face + 0x100);

                int limit = (state.num_peds < 256) ? state.num_peds : 256;
                state.peds.resize(limit);

                for (int i = 0; i < limit; i++)
                {
                    auto& ped = state.peds[i];
                    ped.instance = RPM<uintptr_t>(list + 0x10 * i);

                    if (ped.instance) {
                        ped.world_coords = get_instance_world_coords(ped.instance);
                        ped.last_update = now;

                        // Кости (если нужны)
                        if (g_settings->esp.ped.bone) {
                            float model_matrix[16];
                            get_instace_model_matrix(ped.instance, model_matrix);

                            for (int j = 0; j < 9; j++) {
                                ped.bones[j].model_coords = RPM<Vector3>(ped.instance + 0x410 + 0x10 * j);
                                model_to_world(ped.bones[j].model_coords,
                                    ped.bones[j].world_coords,
                                    model_matrix);
                            }
                            ped.has_bones = true;
                        }
                    }
                }
            }
            else {
                state.peds.clear();
            }

            // === PICKUPS ===
            if (g_settings->esp.pickup.enable)
            {
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x20 });
                uintptr_t list = RPM<uintptr_t>(face + 0x100);

                int limit = (state.num_pickups < 73) ? state.num_pickups : 73;
                state.pickups.resize(limit);

                for (int i = 0; i < limit; i++)
                {
                    auto& pickup = state.pickups[i];
                    pickup.instance = RPM<uintptr_t>(list + 0x10 * i);

                    if (pickup.instance) {
                        pickup.world_coords = get_instance_world_coords(pickup.instance);
                        pickup.last_update = now;
                    }
                }
            }
            else {
                state.pickups.clear();
            }

            // === OBJECTS ===
            if (g_settings->esp.object.enable)
            {
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x28 });
                uintptr_t list = RPM<uintptr_t>(face + 0x158);

                int limit = (state.num_objects < 2300) ? state.num_objects : 2300;
                state.objects.resize(limit);

                for (int i = 0; i < limit; i++)
                {
                    auto& obj = state.objects[i];
                    obj.instance = RPM<uintptr_t>(list + 0x10 * i);

                    if (obj.instance) {
                        obj.world_coords = get_instance_world_coords(obj.instance);
                        obj.last_update = now;
                    }
                }
            }
            else {
                state.objects.clear();
            }

            state.last_entity_update = now;
            last_entity = now;
            state_changed = true;
        }

        // ========================================
        // 3. ТЕКСТОВАЯ ИНФОРМАЦИЯ (~10 FPS)
        // ========================================
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_text).count() >= TEXT_UPDATE_MS)
        {
            constexpr float MAX_TEXT_DISTANCE = 500.0f;

            // === VEHICLES TEXT ===
            if (g_settings->esp.vehicle.enable && g_settings->esp.vehicle.text)
            {
                for (auto& veh : state.vehicles)
                {
                    if (!veh.instance) continue;

                    veh.text.dist = veh.world_coords.Distance(state.self_pos);

                    if (veh.text.dist < MAX_TEXT_DISTANCE) {
                        veh.text.hash = get_instance_hash(veh.instance);
                        get_vehicle_name_1(veh.instance, veh.name_1);
                        get_vehicle_name_2(veh.instance, veh.name_2);
                        veh.text.has_text_data = true;
                    }
                }
            }

            // === PEDS TEXT ===
            if (g_settings->esp.ped.enable && g_settings->esp.ped.text)
            {
                for (auto& ped : state.peds)
                {
                    if (!ped.instance) continue;

                    ped.text.dist = ped.world_coords.Distance(state.self_pos);

                    if (ped.text.dist < MAX_TEXT_DISTANCE) {
                        ped.text.hash = get_instance_hash(ped.instance);
                        ped.ped_data.health = get_ped_health(ped.instance);
                        ped.ped_data.ped_type = get_ped_ped_type(ped.instance);
                        ped.text.has_text_data = true;
                    }
                }
            }

            // === PICKUPS TEXT ===
            if (g_settings->esp.pickup.enable && g_settings->esp.pickup.text)
            {
                for (auto& pickup : state.pickups)
                {
                    if (!pickup.instance) continue;

                    pickup.text.dist = pickup.world_coords.Distance(state.self_pos);

                    if (pickup.text.dist < MAX_TEXT_DISTANCE) {
                        pickup.text.hash = get_instance_hash(pickup.instance);
                        pickup.text.has_text_data = true;
                    }
                }
            }

            // === OBJECTS TEXT ===
            if (g_settings->esp.object.enable && g_settings->esp.object.text)
            {
                for (auto& obj : state.objects)
                {
                    if (!obj.instance) continue;

                    obj.text.dist = obj.world_coords.Distance(state.self_pos);

                    if (obj.text.dist < MAX_TEXT_DISTANCE) {
                        obj.text.hash = get_instance_hash(obj.instance);
                        obj.text.has_text_data = true;
                    }
                }
            }

            state.last_text_update = now;
            last_text = now;
            state_changed = true;
        }

        // ========================================
        // ПЕРЕКЛЮЧАЕМ БУФЕРЫ (если были изменения)
        // ========================================
        if (state_changed) {
            g_game_state.swap_buffers();
        }

        // ========================================
        // АДАПТИВНЫЙ SLEEP
        // ========================================
        int time_to_critical = CRITICAL_UPDATE_MS -
            (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_critical).count();
        int time_to_entity = ENTITY_UPDATE_MS -
            (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_entity).count();
        int time_to_text = TEXT_UPDATE_MS -
            (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_text).count();

        int next_update = time_to_critical;
        if (time_to_entity < next_update) next_update = time_to_entity;
        if (time_to_text < next_update) next_update = time_to_text;

        if (next_update > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(std::min(next_update, 5)));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}