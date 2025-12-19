#pragma once
#include <vector>
#include <atomic>
#include <chrono>
#include "types.hpp"

// Snapshot одной сущности
struct EntitySnapshot {
    uintptr_t instance = 0;
    Vector3 world_coords;

    // Текстовые данные (обновляются реже)
    struct {
        float dist = 0.0f;
        uint32_t hash = 0;
        bool has_text_data = false; // Флаг валидности текстовых данных
    } text;

    // Для кэша - когда последний раз обновляли
    std::chrono::steady_clock::time_point last_update;

    bool is_valid() const { return instance != 0; }
};

// Snapshot педа (с дополнительными данными)
struct PedSnapshot : EntitySnapshot {
    struct {
        float health = 0.0f;
        uint32_t ped_type = 0;
    } ped_data;

    struct BoneData {
        Vector3 model_coords;
        Vector3 world_coords;
    } bones[9];

    bool has_bones = false;
};

// Snapshot транспорта (с именами)
struct VehicleSnapshot : EntitySnapshot {
    char name_1[20] = { 0 };
    char name_2[20] = { 0 };
};

// Главное состояние игры
struct GameState {
    // === КРИТИЧНЫЕ ДАННЫЕ (обновляются ~60 FPS) ===
    uintptr_t self_ped = 0;
    Vector3 self_pos;
    float view_matrix[16] = { 0 };
    int screen_width = 0;
    int screen_height = 0;

    // Счетчики активных сущностей
    int num_vehicles = 0;
    int num_peds = 0;
    int num_pickups = 0;
    int num_objects = 0;

    // === ENTITY SNAPSHOTS (обновляются ~20 FPS) ===
    std::vector<VehicleSnapshot> vehicles;
    std::vector<PedSnapshot> peds;
    std::vector<EntitySnapshot> pickups;
    std::vector<EntitySnapshot> objects;

    // Метки времени последнего обновления
    std::chrono::steady_clock::time_point last_critical_update;
    std::chrono::steady_clock::time_point last_entity_update;
    std::chrono::steady_clock::time_point last_text_update;

    GameState() {
        // Резервируем память заранее
        vehicles.reserve(300);
        peds.reserve(256);
        pickups.reserve(73);
        objects.reserve(2300);
    }

    void clear() {
        vehicles.clear();
        peds.clear();
        pickups.clear();
        objects.clear();
    }
};

// === DOUBLE BUFFERING для lock-free чтения ===
class GameStateManager {
private:
    GameState buffers[2];
    std::atomic<int> active_buffer{ 0 };

public:
    // Получить буфер для ЧТЕНИЯ (из любого потока)
    const GameState& get_read_buffer() const {
        return buffers[active_buffer.load(std::memory_order_acquire)];
    }

    // Получить буфер для ЗАПИСИ (только из update потока!)
    GameState& get_write_buffer() {
        return buffers[1 - active_buffer.load(std::memory_order_relaxed)];
    }

    // Переключить буферы (после завершения записи)
    void swap_buffers() {
        active_buffer.store(1 - active_buffer.load(std::memory_order_relaxed),
            std::memory_order_release);
    }

    // Быстрый доступ к критичным данным (читаются чаще всего)
    uintptr_t get_self_ped() const {
        return get_read_buffer().self_ped;
    }

    Vector3 get_self_pos() const {
        return get_read_buffer().self_pos;
    }

    const float* get_view_matrix() const {
        return get_read_buffer().view_matrix;
    }

    void get_screen_size(int& w, int& h) const {
        const auto& state = get_read_buffer();
        w = state.screen_width;
        h = state.screen_height;
    }
};

// Глобальный экземпляр
inline GameStateManager g_game_state;