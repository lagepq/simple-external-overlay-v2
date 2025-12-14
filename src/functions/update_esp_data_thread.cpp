#include "common.hpp"

void update_esp_data_thread()
{
    while (g_running)
    {
        // ИСПРАВЛЕНО: добавлен sleep при отключенном ESP
        if (!g_settings->esp.enable) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        
        ReadProcessMemory(g_handle, LPCVOID(RPM<uintptr_t>(CViewportGamePointer) + 0x250), esp_matirx, 16 * 4, 0);
        esp_game_width = RPM<int>(WindowWidth);
        esp_game_height = RPM<int>(WindowWidth + 4);
        esp_num_vehicle = RPM<int>(ReplayInterfacePointer, { 0x10, 0x190 });
        esp_num_ped = RPM<int>(ReplayInterfacePointer, { 0x18, 0x110 });
        esp_num_pickup = RPM<int>(ReplayInterfacePointer, { 0x20, 0x110 });
        esp_num_object = RPM<int>(ReplayInterfacePointer, { 0x28, 0x168 });
        esp_self.instance = get_self_ped();
        esp_self.world_coords = get_instance_world_coords(esp_self.instance);
        
        if (g_settings->esp.vehicle.enable)
        {
            uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x10 });
            uintptr_t list = RPM<uintptr_t>(face + 0x180);
            for (int i = 0; i < sizeof(esp_vehicle.item) / sizeof(esp_vehicle.item[0]); i++)
            {
                esp_vehicle.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
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
        
        if (g_settings->esp.ped.enable)
        {
            uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x18 });
            uintptr_t list = RPM<uintptr_t>(face + 0x100);
            for (int i = 0; i < sizeof(esp_ped.item) / sizeof(esp_ped.item[0]); i++)
            {
                esp_ped.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
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
                    for (int j = 0; j < 9; j++)
                    {
                        float model_matrix[16];
                        get_instace_model_matrix(esp_ped.item[i].instance, model_matrix);
                        esp_ped.item[i].bone[j].model_coords = RPM<Vector3>(esp_ped.item[i].instance + 0x410 + 0x10 * j);
                        model_to_world(esp_ped.item[i].bone[j].model_coords, esp_ped.item[i].bone[j].world_coords, model_matrix);
                    }
                }
            }
        }
        
        if (g_settings->esp.pickup.enable)
        {
            uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x20 });
            uintptr_t list = RPM<uintptr_t>(face + 0x100);
            for (int i = 0; i < sizeof(esp_pickup.item) / sizeof(esp_pickup.item[0]); i++)
            {
                esp_pickup.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
                esp_pickup.item[i].world_coords = get_instance_world_coords(esp_pickup.item[i].instance);
                if (g_settings->esp.pickup.text)
                {
                    esp_pickup.item[i].text.dist = esp_pickup.item[i].world_coords.Distance(esp_self.world_coords);
                    esp_pickup.item[i].text.hash = get_instance_hash(esp_pickup.item[i].instance);
                }
            }
        }
        
        if (g_settings->esp.object.enable)
        {
            uintptr_t face = RPM<uintptr_t>(ReplayInterfacePointer, { 0x28 });
            uintptr_t list = RPM<uintptr_t>(face + 0x158);
            for (int i = 0; i < sizeof(esp_object.item) / sizeof(esp_object.item[0]); i++)
            {
                esp_object.item[i].instance = RPM<uintptr_t>(list + 0x10 * i);
                esp_object.item[i].world_coords = get_instance_world_coords(esp_object.item[i].instance);
                if (g_settings->esp.object.text)
                {
                    esp_object.item[i].text.dist = esp_object.item[i].world_coords.Distance(esp_self.world_coords);
                    esp_object.item[i].text.hash = get_instance_hash(esp_object.item[i].instance);
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
