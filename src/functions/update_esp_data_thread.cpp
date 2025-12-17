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
        bool matrix_updated = false;
        bool entity_updated = false;
        bool text_updated = false;

        // 1. КРИТИЧНЫЕ ДАННЫЕ: Матрица, размеры окна, позиция игрока (60 FPS)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_matrix_update).count() >= MATRIX_UPDATE_MS)
        {
            ReadProcessMemory(g_handle, LPCVOID(RPM<uintptr_t>(CViewportGamePointer) + 0x250), esp_matirx, 16 * 4, 0);
            esp_game_width = RPM<int>(WindowWidth);
            esp_game_height = RPM<int>(WindowWidth + 4);
            esp_self.instance = get_self_ped();
            esp_self.world_coords = get_instance_world_coords(esp_self.instance);

            last_matrix_update = now;
            matrix_updated = true;
        }

        // 2. ДАННЫЕ ОБ ENTITIES: Позиции, инстансы (20 FPS)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_entity_update).count() >= ENTITY_UPDATE_MS)
        {
            // Читаем актуальное количество entities
            esp_num_vehicle = RPM<int>(ReplayInterfacePointer, { 0x10, 0x190 });
            esp_num_ped = RPM<int>(ReplayInterfacePointer, { 0x18, 0x110 });
            esp_num_pickup = RPM<int>(ReplayInterfacePointer, { 0x20, 0x110 });
            esp_num_object = RPM<int>(ReplayInterfacePointer, { 0x28, 0x168 });

            // Обновляем только включенные категории
            if (g_settings->esp.vehicle.enable)
            {
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x10 });
                uintptr_t list = RPM<uintptr_t>(face + 0x180);

                // ИСПРАВЛЕНИЕ 1: Читаем только АКТИВНЫЕ транспорты
                int vehicle_limit = esp_num_vehicle;
                if (vehicle_limit > (int)(sizeof(esp_vehicle.item) / sizeof(esp_vehicle.item[0])))
                    vehicle_limit = (int)(sizeof(esp_vehicle.item) / sizeof(esp_vehicle.item[0]));

                for (int i = 0; i < vehicle_limit; i++)
                {
                    esp_vehicle.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
                    if (!esp_vehicle.item[i].instance) continue;

                    esp_vehicle.item[i].world_coords = get_instance_world_coords(esp_vehicle.item[i].instance);
                }

                // Обнуляем неиспользуемые слоты
                for (int i = vehicle_limit; i < sizeof(esp_vehicle.item) / sizeof(esp_vehicle.item[0]); i++)
                {
                    esp_vehicle.item[i].instance = 0;
                }
            }

            if (g_settings->esp.ped.enable)
            {
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x18 });
                uintptr_t list = RPM<uintptr_t>(face + 0x100);

                // ИСПРАВЛЕНИЕ 1: Читаем только АКТИВНЫХ педов
                int ped_limit = esp_num_ped;
                if (ped_limit > (int)(sizeof(esp_ped.item) / sizeof(esp_ped.item[0])))
                    ped_limit = (int)(sizeof(esp_ped.item) / sizeof(esp_ped.item[0]));

                for (int i = 0; i < ped_limit; i++)
                {
                    esp_ped.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
                    if (!esp_ped.item[i].instance) continue;

                    esp_ped.item[i].world_coords = get_instance_world_coords(esp_ped.item[i].instance);

                    // Кости обновляем только если опция включена
                    if (g_settings->esp.ped.bone)
                    {
                        float model_matrix[16];
                        get_instace_model_matrix(esp_ped.item[i].instance, model_matrix);

                        for (int j = 0; j < 9; j++)
                        {
                            esp_ped.item[i].bone[j].model_coords = RPM<Vector3>(esp_ped.item[i].instance + 0x410 + 0x10 * j);
                            model_to_world(esp_ped.item[i].bone[j].model_coords, esp_ped.item[i].bone[j].world_coords, model_matrix);
                        }
                    }
                }

                // Обнуляем неиспользуемые слоты
                for (int i = ped_limit; i < sizeof(esp_ped.item) / sizeof(esp_ped.item[0]); i++)
                {
                    esp_ped.item[i].instance = 0;
                }
            }

            if (g_settings->esp.pickup.enable)
            {
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x20 });
                uintptr_t list = RPM<uintptr_t>(face + 0x100);

                // ИСПРАВЛЕНИЕ 1: Читаем только АКТИВНЫЕ пикапы
                int pickup_limit = esp_num_pickup;
                if (pickup_limit > (int)(sizeof(esp_pickup.item) / sizeof(esp_pickup.item[0])))
                    pickup_limit = (int)(sizeof(esp_pickup.item) / sizeof(esp_pickup.item[0]));

                for (int i = 0; i < pickup_limit; i++)
                {
                    esp_pickup.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
                    if (!esp_pickup.item[i].instance) continue;

                    esp_pickup.item[i].world_coords = get_instance_world_coords(esp_pickup.item[i].instance);
                }

                // Обнуляем неиспользуемые слоты
                for (int i = pickup_limit; i < sizeof(esp_pickup.item) / sizeof(esp_pickup.item[0]); i++)
                {
                    esp_pickup.item[i].instance = 0;
                }
            }

            if (g_settings->esp.object.enable)
            {
                uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x28 });
                uintptr_t list = RPM<uintptr_t>(face + 0x158);

                // ИСПРАВЛЕНИЕ 1: Читаем только АКТИВНЫЕ объекты
                int object_limit = esp_num_object;
                if (object_limit > (int)(sizeof(esp_object.item) / sizeof(esp_object.item[0])))
                    object_limit = (int)(sizeof(esp_object.item) / sizeof(esp_object.item[0]));

                for (int i = 0; i < object_limit; i++)
                {
                    esp_object.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
                    if (!esp_object.item[i].instance) continue;

                    esp_object.item[i].world_coords = get_instance_world_coords(esp_object.item[i].instance);
                }

                // Обнуляем неиспользуемые слоты
                for (int i = object_limit; i < sizeof(esp_object.item) / sizeof(esp_object.item[0]); i++)
                {
                    esp_object.item[i].instance = 0;
                }
            }

            last_entity_update = now;
            entity_updated = true;
        }

        // 3. ТЕКСТОВАЯ ИНФОРМАЦИЯ: Hash, имена, здоровье (10 FPS)
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_text_update).count() >= TEXT_UPDATE_MS)
        {
            // ИСПРАВЛЕНИЕ 2: Distance culling - не обновляем данные для далеких объектов
            constexpr float MAX_TEXT_DISTANCE = 500.0f;

            if (g_settings->esp.vehicle.enable && g_settings->esp.vehicle.text)
            {
                int vehicle_limit = esp_num_vehicle;
                if (vehicle_limit > (int)(sizeof(esp_vehicle.item) / sizeof(esp_vehicle.item[0])))
                    vehicle_limit = (int)(sizeof(esp_vehicle.item) / sizeof(esp_vehicle.item[0]));

                for (int i = 0; i < vehicle_limit; i++)
                {
                    if (!esp_vehicle.item[i].instance) continue;

                    esp_vehicle.item[i].text.dist = esp_vehicle.item[i].world_coords.Distance(esp_self.world_coords);

                    // ИСПРАВЛЕНИЕ 2: Читаем текст только для близких объектов
                    if (esp_vehicle.item[i].text.dist < MAX_TEXT_DISTANCE)
                    {
                        esp_vehicle.item[i].text.hash = get_instance_hash(esp_vehicle.item[i].instance);
                        get_vehicle_name_1(esp_vehicle.item[i].instance, esp_vehicle.item[i].text.name_1);
                        get_vehicle_name_2(esp_vehicle.item[i].instance, esp_vehicle.item[i].text.name_2);
                    }
                }
            }

            if (g_settings->esp.ped.enable && g_settings->esp.ped.text)
            {
                int ped_limit = esp_num_ped;
                if (ped_limit > (int)(sizeof(esp_ped.item) / sizeof(esp_ped.item[0])))
                    ped_limit = (int)(sizeof(esp_ped.item) / sizeof(esp_ped.item[0]));

                for (int i = 0; i < ped_limit; i++)
                {
                    if (!esp_ped.item[i].instance) continue;

                    esp_ped.item[i].text.dist = esp_ped.item[i].world_coords.Distance(esp_self.world_coords);

                    // ИСПРАВЛЕНИЕ 2: Читаем текст только для близких объектов
                    if (esp_ped.item[i].text.dist < MAX_TEXT_DISTANCE)
                    {
                        esp_ped.item[i].text.hash = get_instance_hash(esp_ped.item[i].instance);
                        esp_ped.item[i].text.health = get_ped_health(esp_ped.item[i].instance);
                        esp_ped.item[i].text.ped_type = get_ped_ped_type(esp_ped.item[i].instance);
                    }
                }
            }

            if (g_settings->esp.pickup.enable && g_settings->esp.pickup.text)
            {
                int pickup_limit = esp_num_pickup;
                if (pickup_limit > (int)(sizeof(esp_pickup.item) / sizeof(esp_pickup.item[0])))
                    pickup_limit = (int)(sizeof(esp_pickup.item) / sizeof(esp_pickup.item[0]));

                for (int i = 0; i < pickup_limit; i++)
                {
                    if (!esp_pickup.item[i].instance) continue;

                    esp_pickup.item[i].text.dist = esp_pickup.item[i].world_coords.Distance(esp_self.world_coords);

                    // ИСПРАВЛЕНИЕ 2: Читаем текст только для близких объектов
                    if (esp_pickup.item[i].text.dist < MAX_TEXT_DISTANCE)
                    {
                        esp_pickup.item[i].text.hash = get_instance_hash(esp_pickup.item[i].instance);
                    }
                }
            }

            if (g_settings->esp.object.enable && g_settings->esp.object.text)
            {
                int object_limit = esp_num_object;
                if (object_limit > (int)(sizeof(esp_object.item) / sizeof(esp_object.item[0])))
                    object_limit = (int)(sizeof(esp_object.item) / sizeof(esp_object.item[0]));

                for (int i = 0; i < object_limit; i++)
                {
                    if (!esp_object.item[i].instance) continue;

                    esp_object.item[i].text.dist = esp_object.item[i].world_coords.Distance(esp_self.world_coords);

                    // ИСПРАВЛЕНИЕ 2: Читаем текст только для близких объектов
                    if (esp_object.item[i].text.dist < MAX_TEXT_DISTANCE)
                    {
                        esp_object.item[i].text.hash = get_instance_hash(esp_object.item[i].instance);
                    }
                }
            }

            last_text_update = now;
            text_updated = true;
        }

        // ИСПРАВЛЕНИЕ 3: Адаптивный sleep - спим меньше если нужно обновление
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
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}