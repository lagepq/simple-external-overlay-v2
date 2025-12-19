#include "common.hpp"

void update_esp_data_thread()
{
    // Настройки частоты обновления
    constexpr int MATRIX_UPDATE_MS = 16;      // ~60 FPS - критичные данные (матрица, позиция)
    constexpr int ENTITY_UPDATE_MS = 50;      // ~20 FPS - данные об entities
    constexpr int TEXT_UPDATE_MS = 100;       // ~10 FPS - текстовая информация

    auto last_matrix_update = std::chrono::steady_clock::now();
    auto last_entity_update = std::chrono::steady_clock::now();
    auto last_text_update = std::chrono::steady_clock::now();

    while (g_running)
    {
        if (!g_settings->esp.enable) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        auto now = std::chrono::steady_clock::now();
        GameState& write_state = g_game_state.get_write_buffer();

        // 1. КРИТИЧНЫЕ ДАННЫЕ: Матрица, размеры окна, позиция игрока (60 FPS)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_matrix_update).count() >= MATRIX_UPDATE_MS)
        {
            ReadProcessMemory(g_handle, LPCVOID(RPM<uintptr_t>(CViewportGamePointer) + 0x250), write_state.view_matrix, 16 * 4, 0);
            write_state.screen_width = RPM<int>(WindowWidth);
            write_state.screen_height = RPM<int>(WindowHeight);
            write_state.self_ped = get_self_ped();
            get_instance_world_coords(write_state.self_ped, write_state.self_pos);

            last_matrix_update = now;
        }

        // 2. ДАННЫЕ ОБ ENTITIES: Позиции, инстансы (20 FPS)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_entity_update).count() >= ENTITY_UPDATE_MS)
        {
            // ИСПРАВЛЕНО: читаем количество один раз и сохраняем локально
            int num_vehicle = RPM<int>(ReplayInterfacePointer, { 0x10, 0x190 });
            int num_ped = RPM<int>(ReplayInterfacePointer, { 0x18, 0x110 });
            int num_pickup = RPM<int>(ReplayInterfacePointer, { 0x20, 0x110 });
            int num_object = RPM<int>(ReplayInterfacePointer, { 0x28, 0x168 });

            // Обновляем глобальные переменные
            write_state.num_vehicles = num_vehicle;
            write_state.num_peds = num_ped;
            write_state.num_pickups = num_pickup;
            write_state.num_objects = num_object;

            // Обновляем только включенные категории
            if (g_settings->esp.vehicle.enable)
            {
                write_state.vehicles.clear();
                
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x10 });
                uintptr_t list = RPM<uintptr_t>(face + 0x180);

                int vehicle_limit = num_vehicle;
                if (vehicle_limit > (int)write_state.vehicles.capacity())
                    vehicle_limit = (int)write_state.vehicles.capacity();

                for (int i = 0; i < vehicle_limit; i++)
                {
                    uintptr_t instance = RPM<uintptr_t>(list + 0x10 * i);
                    if (!instance) continue;

                    VehicleSnapshot vehicle;
                    vehicle.instance = instance;
                    get_instance_world_coords(instance, vehicle.world_coords);
                    vehicle.last_update = now;

                    write_state.vehicles.push_back(vehicle);
                }
            }

            if (g_settings->esp.ped.enable)
            {
                write_state.peds.clear();
                
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x18 });
                uintptr_t list = RPM<uintptr_t>(face + 0x100);

                int ped_limit = num_ped;
                if (ped_limit > (int)write_state.peds.capacity())
                    ped_limit = (int)write_state.peds.capacity();

                for (int i = 0; i < ped_limit; i++)
                {
                    uintptr_t instance = RPM<uintptr_t>(list + 0x10 * i);
                    if (!instance) continue;

                    PedSnapshot ped;
                    ped.instance = instance;
                    get_instance_world_coords(instance, ped.world_coords);
                    ped.last_update = now;

                    // Кости обновляем только если опция включена
                    if (g_settings->esp.ped.bone)
                    {
                        float model_matrix[16];
                        get_instace_model_matrix(instance, model_matrix);

                        for (int j = 0; j < 9; j++)
                        {
                            ped.bones[j].model_coords = RPM<Vector3>(instance + 0x410 + 0x10 * j);
                            model_to_world(ped.bones[j].model_coords, ped.bones[j].world_coords, model_matrix);
                        }
                        ped.has_bones = true;
                    }

                    write_state.peds.push_back(ped);
                }
            }

            if (g_settings->esp.pickup.enable)
            {
                write_state.pickups.clear();
                
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x20 });
                uintptr_t list = RPM<uintptr_t>(face + 0x100);

                int pickup_limit = num_pickup;
                if (pickup_limit > (int)write_state.pickups.capacity())
                    pickup_limit = (int)write_state.pickups.capacity();

                for (int i = 0; i < pickup_limit; i++)
                {
                    uintptr_t instance = RPM<uintptr_t>(list + 0x10 * i);
                    if (!instance) continue;

                    EntitySnapshot pickup;
                    pickup.instance = instance;
                    get_instance_world_coords(instance, pickup.world_coords);
                    pickup.last_update = now;

                    write_state.pickups.push_back(pickup);
                }
            }

            if (g_settings->esp.object.enable)
            {
                write_state.objects.clear();
                
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x28 });
                uintptr_t list = RPM<uintptr_t>(face + 0x158);

                int object_limit = num_object;
                if (object_limit > (int)write_state.objects.capacity())
                    object_limit = (int)write_state.objects.capacity();

                for (int i = 0; i < object_limit; i++)
                {
                    uintptr_t instance = RPM<uintptr_t>(list + 0x10 * i);
                    if (!instance) continue;

                    EntitySnapshot object;
                    object.instance = instance;
                    get_instance_world_coords(instance, object.world_coords);
                    object.last_update = now;

                    write_state.objects.push_back(object);
                }
            }

            last_entity_update = now;
        }

        // 3. ТЕКСТОВАЯ ИНФОРМАЦИЯ: Hash, имена, здоровье (10 FPS)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_text_update).count() >= TEXT_UPDATE_MS)
        {
            constexpr float MAX_TEXT_DISTANCE = 500.0f;

            if (g_settings->esp.vehicle.enable && g_settings->esp.vehicle.text)
            {
                for (auto& vehicle : write_state.vehicles)
                {
                    if (!vehicle.instance) continue;

                    vehicle.text.dist = vehicle.world_coords.Distance(write_state.self_pos);

                    if (vehicle.text.dist < MAX_TEXT_DISTANCE)
                    {
                        vehicle.text.hash = get_instance_hash(vehicle.instance);
                        get_vehicle_name_1(vehicle.instance, vehicle.name_1);
                        get_vehicle_name_2(vehicle.instance, vehicle.name_2);
                        vehicle.text.has_text_data = true;
                    }
                }
            }

            if (g_settings->esp.ped.enable && g_settings->esp.ped.text)
            {
                for (auto& ped : write_state.peds)
                {
                    if (!ped.instance) continue;

                    ped.text.dist = ped.world_coords.Distance(write_state.self_pos);

                    if (ped.text.dist < MAX_TEXT_DISTANCE)
                    {
                        ped.text.hash = get_instance_hash(ped.instance);
                        ped.ped_data.health = get_ped_health(ped.instance);
                        ped.ped_data.ped_type = get_ped_ped_type(ped.instance);
                        ped.text.has_text_data = true;
                    }
                }
            }

            if (g_settings->esp.pickup.enable && g_settings->esp.pickup.text)
            {
                for (auto& pickup : write_state.pickups)
                {
                    if (!pickup.instance) continue;

                    pickup.text.dist = pickup.world_coords.Distance(write_state.self_pos);

                    if (pickup.text.dist < MAX_TEXT_DISTANCE)
                    {
                        pickup.text.hash = get_instance_hash(pickup.instance);
                        pickup.text.has_text_data = true;
                    }
                }
            }

            if (g_settings->esp.object.enable && g_settings->esp.object.text)
            {
                for (auto& object : write_state.objects)
                {
                    if (!object.instance) continue;

                    object.text.dist = object.world_coords.Distance(write_state.self_pos);

                    if (object.text.dist < MAX_TEXT_DISTANCE)
                    {
                        object.text.hash = get_instance_hash(object.instance);
                        object.text.has_text_data = true;
                    }
                }
            }

            last_text_update = now;
        }

        // Переключаем буферы после обновления
        g_game_state.swap_buffers();

        // Адаптивный sleep
        int time1 = MATRIX_UPDATE_MS - (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_matrix_update).count();
        int time2 = ENTITY_UPDATE_MS - (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_entity_update).count();
        int time3 = TEXT_UPDATE_MS - (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last_text_update).count();

        int next_update_in = time1 < time2 ? time1 : time2;
        next_update_in = next_update_in < time3 ? next_update_in : time3;

        if (next_update_in > 0)
        {
            int sleep_time = next_update_in < 5 ? next_update_in : 5;
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}