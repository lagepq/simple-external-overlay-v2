#include "common.hpp"

void aimbot_thread()
{
    constexpr float MIN_HEALTH = 100.0f;
    constexpr float MAX_AIMBOT_DISTANCE = 500.0f;

    while (g_running)
    {
        if (!g_settings->weapon.aimbot)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (GetKeyState(VK_RBUTTON) >= 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // ¡‡Ú˜ËÏ ˜ÚÂÌËÂ ‚ÒÂı ÌÂÓ·ıÓ‰ËÏ˚ı ‰‡ÌÌ˚ı
        struct AimbotData {
            uintptr_t self_ped;
            Vector3 self_pos;
            float matrix[16];
            int width;
            int height;
            uintptr_t ped_list_base;
            uintptr_t ped_list;
            int max_count;
            int current_count;
        } data;

        data.self_ped = RPM<uintptr_t>(CPedFactoryPointer, { 0x8 });
        if (!data.self_ped)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        data.self_pos = RPM<Vector3>(data.self_ped + 0x90);

        uintptr_t viewport = RPM<uintptr_t>(CViewportGamePointer);
        ReadProcessMemory(g_handle, LPCVOID(viewport + 0x250), data.matrix, 16 * 4, 0);

        data.width = RPM<int>(WindowWidth);
        data.height = RPM<int>(WindowWidth + 0x4);

        float center_x = data.width / 2.0f;
        float center_y = data.height / 2.0f;
        float fov_limit = g_settings->aimbot_advanced.fov_size;

        data.ped_list_base = RPM<uintptr_t>(ReplayInterfacePointer, { 0x18 });
        data.ped_list = RPM<uintptr_t>(data.ped_list_base + 0x100);
        data.max_count = RPM<int>(data.ped_list_base + 0x108);
        data.current_count = RPM<int>(data.ped_list_base + 0x110);

        float best_score = FLT_MAX;
        uintptr_t target_ped = 0;

        for (size_t i = 0, j = 0; i < data.max_count && j < data.current_count; i++)
        {
            uintptr_t ped = RPM<uintptr_t>(data.ped_list + i * 0x10);

            if (!ped || ped == data.self_ped)
                continue;

            j++;

            float health = RPM<float>(ped + 0x280);
            if (health < MIN_HEALTH)
                continue;

            if (g_settings->weapon.aimbot_player_only)
            {
                uintptr_t player_info = RPM<uintptr_t>(ped + 0x10A8);
                if (!player_info)
                    continue;
            }

            Vector3 pos = RPM<Vector3>(ped + 0x90);

            float dist_3d = data.self_pos.Distance(pos);
            if (dist_3d > MAX_AIMBOT_DISTANCE)
                continue;

            Vector3 screen;
            if (!WorldToScreen(pos, screen, data.matrix, data.width, data.height))
                continue;

            float dx = screen.x - center_x;
            float dy = screen.y - center_y;
            float screen_dist = sqrt(dx * dx + dy * dy);

            if (screen_dist > fov_limit)
                continue;

            // ÕŒ¬Œ≈: Target Priority System
            float score = 0.0f;

            if (g_settings->aimbot_advanced.target_priority == 0) // Closest to crosshair
            {
                score = screen_dist;
            }
            else if (g_settings->aimbot_advanced.target_priority == 1) // Closest distance
            {
                score = dist_3d;
            }
            else if (g_settings->aimbot_advanced.target_priority == 2) // Lowest HP
            {
                score = health; // Lower HP = lower score = priority
            }

            if (score < best_score)
            {
                best_score = score;
                target_ped = ped;
            }
        }

        if (target_ped != 0)
        {
            float model_matrix[16];
            get_instace_model_matrix(target_ped, model_matrix);

            // ÕŒ¬Œ≈: Bone Selection
            int bone_offset;
            switch (g_settings->aimbot_advanced.target_bone)
            {
            case 0: bone_offset = 0x410; break; // Head
            case 1: bone_offset = 0x410 + 0x10 * 7; break; // Neck
            case 2: bone_offset = 0x410 + 0x10 * 8; break; // Chest (Abdomen)
            default: bone_offset = 0x410; break;
            }

            Vector3 bone_model = RPM<Vector3>(target_ped + bone_offset);
            Vector3 bone_world;
            model_to_world(bone_model, bone_world, model_matrix);

            // ÕŒ¬Œ≈: Prediction for moving targets
            if (g_settings->aimbot_advanced.prediction)
            {
                static std::map<uintptr_t, Vector3> last_positions;
                static std::map<uintptr_t, std::chrono::steady_clock::time_point> last_times;

                auto now = std::chrono::steady_clock::now();

                if (last_positions.find(target_ped) != last_positions.end())
                {
                    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_times[target_ped]).count();
                    if (time_diff > 0 && time_diff < 500) // Valid time window
                    {
                        Vector3 velocity;
                        velocity.x = (bone_world.x - last_positions[target_ped].x) / (time_diff / 1000.0f);
                        velocity.y = (bone_world.y - last_positions[target_ped].y) / (time_diff / 1000.0f);
                        velocity.z = (bone_world.z - last_positions[target_ped].z) / (time_diff / 1000.0f);

                        float prediction_time = 0.1f * g_settings->aimbot_advanced.prediction_factor; // Predict 0.1s ahead
                        bone_world.x += velocity.x * prediction_time;
                        bone_world.y += velocity.y * prediction_time;
                        bone_world.z += velocity.z * prediction_time;
                    }
                }

                last_positions[target_ped] = bone_world;
                last_times[target_ped] = now;
            }

            uintptr_t camGameplayDirector = RPM<uintptr_t>(camGameplayDirectorPointer);
            uintptr_t camFollowPedCamera = RPM<uintptr_t>(camGameplayDirector + 0x2C8);
            Vector3 camera_pos = RPM<Vector3>(camFollowPedCamera + 0x60);

            uintptr_t camFollowPedCameraMetadata = RPM<uintptr_t>(camFollowPedCamera + 0x10);
            float camera_distance = RPM<float>(camFollowPedCameraMetadata + 0x30);
            bool is_first_person = (camera_distance == 0.0f);

            float dist = camera_pos.Distance(bone_world);
            Vector3 target_angle;
            target_angle.x = (bone_world.x - camera_pos.x) / dist;
            target_angle.y = (bone_world.y - camera_pos.y) / dist;
            target_angle.z = (bone_world.z - camera_pos.z) / dist;

            uintptr_t angle_offset = is_first_person ? 0x40 : 0x3D0;
            Vector3 current_angle = RPM<Vector3>(camFollowPedCamera + angle_offset);

            Vector3 final_angle;
            if (g_settings->weapon.aimbot_smooth)
            {
                float smooth = g_settings->weapon.aimbot_smooth_factor;
                final_angle.x = current_angle.x + (target_angle.x - current_angle.x) * smooth;
                final_angle.y = current_angle.y + (target_angle.y - current_angle.y) * smooth;
                final_angle.z = current_angle.z + (target_angle.z - current_angle.z) * smooth;
            }
            else
            {
                final_angle = target_angle;
            }

            WPM<Vector3>(camFollowPedCamera + angle_offset, final_angle);
        }

        if (target_ped != 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}