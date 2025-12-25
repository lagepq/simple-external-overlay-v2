//#include "common.hpp"
//
//// common.cpp должен быть пустым или содержать только:
//// #include "common.hpp"
//
//void update_esp_data_thread()
//{
//    constexpr int CRITICAL_UPDATE_MS = 16;    // ~60 FPS
//    constexpr int ENTITY_UPDATE_MS = 50;      // ~20 FPS
//    constexpr int TEXT_UPDATE_MS = 100;       // ~10 FPS
//
//    auto last_critical = std::chrono::steady_clock::now();
//    auto last_entity = std::chrono::steady_clock::now();
//    auto last_text = std::chrono::steady_clock::now();
//
//    while (g_running)
//    {
//        if (!g_settings->esp.enable) {
//            std::this_thread::sleep_for(std::chrono::milliseconds(100));
//            continue;
//        }
//
//        auto now = std::chrono::steady_clock::now();
//        GameState& state = g_game_state.get_write_buffer();
//        bool state_changed = false;
//
//        // ========================================
//        // 1. КРИТИЧНЫЕ ДАННЫЕ (~60 FPS)
//        // ========================================
//        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_critical).count() >= CRITICAL_UPDATE_MS)
//        {
//            ReadProcessMemory(g_handle,
//                LPCVOID(RPM<uintptr_t>(CViewportGamePointer) + 0x250),
//                state.view_matrix, 16 * 4, 0);
//
//            state.screen_width = RPM<int>(WindowWidth);
//            state.screen_height = RPM<int>(WindowWidth + 4);
//
//            state.self_ped = get_self_ped();
//            if (state.self_ped) {
//                state.self_pos = get_instance_world_coords(state.self_ped);
//            }
//
//            // Синхронизация с глобальными переменными для ESP
//            memcpy(esp_matirx, state.view_matrix, sizeof(esp_matirx));
//            esp_game_width = state.screen_width;
//            esp_game_height = state.screen_height;
//            esp_self.instance = state.self_ped;
//            esp_self.world_coords = state.self_pos;
//
//            state.last_critical_update = now;
//            last_critical = now;
//            state_changed = true;
//        }
//
//        // ========================================
//        // 2. ПОЗИЦИИ ENTITIES (~20 FPS)
//        // ========================================
//        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_entity).count() >= ENTITY_UPDATE_MS)
//        {
//            state.num_vehicles = RPM<int>(ReplayInterfacePointer, { 0x10, 0x190 });
//            state.num_peds = RPM<int>(ReplayInterfacePointer, { 0x18, 0x110 });
//            state.num_pickups = RPM<int>(ReplayInterfacePointer, { 0x20, 0x110 });
//            state.num_objects = RPM<int>(ReplayInterfacePointer, { 0x28, 0x168 });
//
//            esp_num_vehicle = state.num_vehicles;
//            esp_num_ped = state.num_peds;
//            esp_num_pickup = state.num_pickups;
//            esp_num_object = state.num_objects;
//
//            // === VEHICLES ===
//            if (g_settings->esp.vehicle.enable)
//            {
//                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x10 });
//                uintptr_t list = RPM<uintptr_t>(face + 0x180);
//                int limit = std::min(state.num_vehicles, 300);
//
//                for (int i = 0; i < limit; i++)
//                {
//                    esp_vehicle.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
//                    if (esp_vehicle.item[i].instance) {
//                        esp_vehicle.item[i].world_coords = get_instance_world_coords(esp_vehicle.item[i].instance);
//                    }
//                }
//            }
//
//            // === PEDS ===
//            if (g_settings->esp.ped.enable)
//            {
//                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x18 });
//                uintptr_t list = RPM<uintptr_t>(face + 0x100);
//                int limit = std::min(state.num_peds, 256);
//
//                for (int i = 0; i < limit; i++)
//                {
//                    esp_ped.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
//                    if (esp_ped.item[i].instance) {
//                        esp_ped.item[i].world_coords = get_instance_world_coords(esp_ped.item[i].instance);
//
//                        if (g_settings->esp.ped.bone) {
//                            float model_matrix[16];
//                            get_instace_model_matrix(esp_ped.item[i].instance, model_matrix);
//
//                            for (int j = 0; j < 9; j++) {
//                                esp_ped.item[i].bone[j].model_coords = RPM<Vector3>(esp_ped.item[i].instance + 0x410 + 0x10 * j);
//                                model_to_world(esp_ped.item[i].bone[j].model_coords,
//                                    esp_ped.item[i].bone[j].world_coords,
//                                    model_matrix);
//                            }
//                        }
//                    }
//                }
//            }
//
//            // === PICKUPS ===
//            if (g_settings->esp.pickup.enable)
//            {
//                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x20 });
//                uintptr_t list = RPM<uintptr_t>(face + 0x100);
//                int limit = std::min(state.num_pickups, 73);
//
//                for (int i = 0; i < limit; i++)
//                {
//                    esp_pickup.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
//                    if (esp_pickup.item[i].instance) {
//                        esp_pickup.item[i].world_coords = get_instance_world_coords(esp_pickup.item[i].instance);
//                    }
//                }
//            }
//
//            // === OBJECTS ===
//            if (g_settings->esp.object.enable)
//            {
//                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x28 });
//                uintptr_t list = RPM<uintptr_t>(face + 0x158);
//                int limit = std::min(state.num_objects, 2300);
//
//                for (int i = 0; i < limit; i++)
//                {
//                    esp_object.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
//                    if (esp_object.item[i].instance) {
//                        esp_object.item[i].world_coords = get_instance_world_coords(esp_object.item[i].instance);
//                    }
//                }
//            }
//
//            state.last_entity_update = now;
//            last_entity = now;
//            state_changed = true;
//        }
//
//        // ========================================
//        // 3. ТЕКСТОВАЯ ИНФОРМАЦИЯ (~10 FPS)
//        // ========================================
//        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_text).count() >= TEXT_UPDATE_MS)
//        {
//            constexpr float MAX_TEXT_DISTANCE = 500.0f;
//
//            // === VEHICLES TEXT ===
//            if (g_settings->esp.vehicle.enable && g_settings->esp.vehicle.text)
//            {
//                int limit = std::min(state.num_vehicles, 300);
//                for (int i = 0; i < limit; i++)
//                {
//                    if (!esp_vehicle.item[i].instance) continue;
//
//                    esp_vehicle.item[i].text.dist = esp_vehicle.item[i].world_coords.Distance(esp_self.world_coords);
//
//                    if (esp_vehicle.item[i].text.dist < MAX_TEXT_DISTANCE) {
//                        esp_vehicle.item[i].text.hash = get_instance_hash(esp_vehicle.item[i].instance);
//                        get_vehicle_name_1(esp_vehicle.item[i].instance, esp_vehicle.item[i].text.name_1);
//                        get_vehicle_name_2(esp_vehicle.item[i].instance, esp_vehicle.item[i].text.name_2);
//                    }
//                }
//            }
//
//            // === PEDS TEXT ===
//            if (g_settings->esp.ped.enable && g_settings->esp.ped.text)
//            {
//                int limit = std::min(state.num_peds, 256);
//                for (int i = 0; i < limit; i++)
//                {
//                    if (!esp_ped.item[i].instance) continue;
//
//                    esp_ped.item[i].text.dist = esp_ped.item[i].world_coords.Distance(esp_self.world_coords);
//
//                    if (esp_ped.item[i].text.dist < MAX_TEXT_DISTANCE) {
//                        esp_ped.item[i].text.hash = get_instance_hash(esp_ped.item[i].instance);
//                        esp_ped.item[i].text.health = get_ped_health(esp_ped.item[i].instance);
//                        esp_ped.item[i].text.ped_type = get_ped_ped_type(esp_ped.item[i].instance);
//                    }
//                }
//            }
//
//            // === PICKUPS TEXT ===
//            if (g_settings->esp.pickup.enable && g_settings->esp.pickup.text)
//            {
//                int limit = std::min(state.num_pickups, 73);
//                for (int i = 0; i < limit; i++)
//                {
//                    if (!esp_pickup.item[i].instance) continue;
//
//                    esp_pickup.item[i].text.dist = esp_pickup.item[i].world_coords.Distance(esp_self.world_coords);
//
//                    if (esp_pickup.item[i].text.dist < MAX_TEXT_DISTANCE) {
//                        esp_pickup.item[i].text.hash = get_instance_hash(esp_pickup.item[i].instance);
//                    }
//                }
//            }
//
//            // === OBJECTS TEXT ===
//            if (g_settings->esp.object.enable && g_settings->esp.object.text)
//            {
//                int limit = std::min(state.num_objects, 2300);
//                for (int i = 0; i < limit; i++)
//                {
//                    if (!esp_object.item[i].instance) continue;
//
//                    esp_object.item[i].text.dist = esp_object.item[i].world_coords.Distance(esp_self.world_coords);
//
//                    if (esp_object.item[i].text.dist < MAX_TEXT_DISTANCE) {
//                        esp_object.item[i].text.hash = get_instance_hash(esp_object.item[i].instance);
//                    }
//                }
//            }
//
//            state.last_text_update = now;
//            last_text = now;
//            state_changed = true;
//        }
//
//        // Переключаем буферы только если были изменения
//        if (state_changed) {
//            g_game_state.swap_buffers();
//        }
//
//        // Адаптивный sleep
//        int time_to_critical = CRITICAL_UPDATE_MS -
//            (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_critical).count();
//        int time_to_entity = ENTITY_UPDATE_MS -
//            (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_entity).count();
//        int time_to_text = TEXT_UPDATE_MS -
//            (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_text).count();
//
//        int next_update = time_to_critical;
//        if (time_to_entity < next_update) next_update = time_to_entity;
//        if (time_to_text < next_update) next_update = time_to_text;
//
//        if (next_update > 0) {
//            std::this_thread::sleep_for(std::chrono::milliseconds(std::min(next_update, 5)));
//        }
//        else {
//            std::this_thread::sleep_for(std::chrono::milliseconds(1));
//        }
//    }
//}
#include "common.hpp"

void update_esp_data_thread()
{
    while (g_running)
    {
        // Если ESP выключен - просто спим и не читаем память
        if (!g_settings->esp.enable) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Читаем матрицу и размеры экрана
        ReadProcessMemory(g_handle,
            LPCVOID(RPM<uintptr_t>(CViewportGamePointer) + 0x250),
            esp_matirx, 16 * 4, 0);

        esp_game_width = RPM<int>(WindowWidth);
        esp_game_height = RPM<int>(WindowWidth + 4);

        // Читаем количество объектов
        esp_num_vehicle = RPM<int>(ReplayInterfacePointer, { 0x10, 0x190 });
        esp_num_ped = RPM<int>(ReplayInterfacePointer, { 0x18, 0x110 });
        esp_num_pickup = RPM<int>(ReplayInterfacePointer, { 0x20, 0x110 });
        esp_num_object = RPM<int>(ReplayInterfacePointer, { 0x28, 0x168 });

        // Читаем позицию игрока
        esp_self.instance = get_self_ped();
        esp_self.world_coords = get_instance_world_coords(esp_self.instance);

        // === VEHICLES ===
        if (g_settings->esp.vehicle.enable)
        {
            uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x10 });
            uintptr_t list = RPM<uintptr_t>(face + 0x180);

            for (int i = 0; i < 300; i++)
            {
                esp_vehicle.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);

                if (esp_vehicle.item[i].instance) {
                    esp_vehicle.item[i].world_coords = get_instance_world_coords(esp_vehicle.item[i].instance);

                    if (g_settings->esp.vehicle.text)
                    {
                        esp_vehicle.item[i].text.dist = esp_vehicle.item[i].world_coords.Distance(esp_self.world_coords);
                        esp_vehicle.item[i].text.hash = get_instance_hash(esp_vehicle.item[i].instance);
                        get_vehicle_name_1(esp_vehicle.item[i].instance, esp_vehicle.item[i].text.name_1);
                        get_vehicle_name_2(esp_vehicle.item[i].instance, esp_vehicle.item[i].text.name_2);
                    }
                }
            }
        }

        // === PEDS ===
        if (g_settings->esp.ped.enable)
        {
            uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x18 });
            uintptr_t list = RPM<uintptr_t>(face + 0x100);

            for (int i = 0; i < 256; i++)
            {
                esp_ped.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);

                if (esp_ped.item[i].instance) {
                    esp_ped.item[i].world_coords = get_instance_world_coords(esp_ped.item[i].instance);

                    if (g_settings->esp.ped.text)
                    {
                        esp_ped.item[i].text.dist = esp_ped.item[i].world_coords.Distance(esp_self.world_coords);
                        esp_ped.item[i].text.hash = get_instance_hash(esp_ped.item[i].instance);
                        esp_ped.item[i].text.health = get_ped_health(esp_ped.item[i].instance);
                        esp_ped.item[i].text.ped_type = get_ped_ped_type(esp_ped.item[i].instance);
                    }

                    if (g_settings->esp.ped.bone)
                    {
                        float model_matrix[16];
                        get_instace_model_matrix(esp_ped.item[i].instance, model_matrix);

                        for (int j = 0; j < 9; j++) {
                            esp_ped.item[i].bone[j].model_coords = RPM<Vector3>(esp_ped.item[i].instance + 0x410 + 0x10 * j);
                            model_to_world(esp_ped.item[i].bone[j].model_coords,
                                esp_ped.item[i].bone[j].world_coords,
                                model_matrix);
                        }
                    }
                }
            }
        }

        // === PICKUPS ===
        if (g_settings->esp.pickup.enable)
        {
            uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x20 });
            uintptr_t list = RPM<uintptr_t>(face + 0x100);

            for (int i = 0; i < 73; i++)
            {
                esp_pickup.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);

                if (esp_pickup.item[i].instance) {
                    esp_pickup.item[i].world_coords = get_instance_world_coords(esp_pickup.item[i].instance);

                    if (g_settings->esp.pickup.text)
                    {
                        esp_pickup.item[i].text.dist = esp_pickup.item[i].world_coords.Distance(esp_self.world_coords);
                        esp_pickup.item[i].text.hash = get_instance_hash(esp_pickup.item[i].instance);
                    }
                }
            }
        }

        // === OBJECTS ===
        if (g_settings->esp.object.enable)
        {
            uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x28 });
            uintptr_t list = RPM<uintptr_t>(face + 0x158);

            for (int i = 0; i < 2300; i++)
            {
                esp_object.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);

                if (esp_object.item[i].instance) {
                    esp_object.item[i].world_coords = get_instance_world_coords(esp_object.item[i].instance);

                    if (g_settings->esp.object.text)
                    {
                        esp_object.item[i].text.dist = esp_object.item[i].world_coords.Distance(esp_self.world_coords);
                        esp_object.item[i].text.hash = get_instance_hash(esp_object.item[i].instance);
                    }
                }
            }
        }

        // Спим 1мс между обновлениями
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}