#include "common.hpp"

static inline auto now() {
    return std::chrono::steady_clock::now();
}

static inline int elapsed_ms(auto t) {
    return (int)std::chrono::duration_cast<std::chrono::milliseconds>(
        now() - t).count();
}

void game_state_thread() {
    constexpr int SCREEN_MS = 16;
    constexpr int SELF_MS = 16;
    constexpr int PED_MS = 50;

    auto last_screen = now();
    auto last_self = now();
    auto last_peds = now();

    while (g_running) {
        auto& state = g_game_state.get_write_buffer();

        // ===== SCREEN =====
        if (elapsed_ms(last_screen) >= SCREEN_MS) {
            ReadProcessMemory(
                g_handle,
                LPCVOID(RPM<uintptr_t>(CViewportGamePointer) + 0x250),
                state.view_matrix,
                sizeof(state.view_matrix),
                nullptr
            );

            state.screen_width = RPM<int>(WindowWidth);
            state.screen_height = RPM<int>(WindowWidth + 4);

            last_screen = now();
        }

        // ===== SELF =====
        if (elapsed_ms(last_self) >= SELF_MS) {
            state.self_ped = get_self_ped();
            if (state.self_ped) {
                state.self_pos = get_instance_world_coords(state.self_ped);
            }
            last_self = now();
        }

        // ===== PEDS =====
        if (elapsed_ms(last_peds) >= PED_MS) {
            state.peds.clear();

            uintptr_t ped_iface = RPM<uintptr_t>(ReplayInterfacePointer, { 0x18 });
            uintptr_t list = RPM<uintptr_t>(ped_iface + 0x100);
            int count = RPM<int>(ped_iface + 0x110);

            for (int i = 0; i < count && i < 256; i++) {
                uintptr_t ped = RPM<uintptr_t>(list + i * 0x10);
                if (!ped || ped == state.self_ped) continue;

                PedSnapshot p{};
                p.instance = ped;
                p.ped_data.health = get_ped_health(ped);
                if (p.ped_data.health < 100.f) continue;

                p.world_coords = get_instance_world_coords(ped);
                p.last_update = now();

                state.peds.push_back(p);
            }

            last_peds = now();
        }

        g_game_state.swap_buffers();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}