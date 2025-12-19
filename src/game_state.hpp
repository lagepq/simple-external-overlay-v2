#pragma once

#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>
#include "types.hpp"

// Базовая структура для всех сущностей
struct EntitySnapshot {
    uintptr_t instance = 0;
    Vector3 world_coords{};
    std::chrono::steady_clock::time_point last_update;
    bool is_valid = false;  // Флаг валидности сущности

    struct {
        uint32_t hash = 0;
        float dist = 0.0f;
        bool has_text_data = false;
    } text;
};

// Информация о педе
struct PedSnapshot : EntitySnapshot {
    struct {
        float health = 0.0f;
        uint32_t ped_type = 0;
    } ped_data;

    struct BoneData {
        Vector3 model_coords{};
        Vector3 world_coords{};
    } bones[9];

    bool has_bones = false;
};

// Информация о транспорте
struct VehicleSnapshot : EntitySnapshot {
    char name_1[20] = {};
    char name_2[20] = {};
};

// Основное состояние игры
struct GameState {
    // Критичные данные
    float view_matrix[16] = {};
    int screen_width = 0;
    int screen_height = 0;
    uintptr_t self_ped = 0;
    Vector3 self_pos{};

    // Счетчики
    int num_vehicles = 0;
    int num_peds = 0;
    int num_pickups = 0;
    int num_objects = 0;

    // Сущности
    std::vector<VehicleSnapshot> vehicles;
    std::vector<PedSnapshot> peds;
    std::vector<EntitySnapshot> pickups;
    std::vector<EntitySnapshot> objects;

    // Временные метки
    std::chrono::steady_clock::time_point last_critical_update;
    std::chrono::steady_clock::time_point last_entity_update;
    std::chrono::steady_clock::time_point last_text_update;
};

// Менеджер двойной буферизации
class GameStateManager {
private:
    GameState buffers[2];
    std::atomic<int> read_index{ 0 };
    std::atomic<int> write_index{ 1 };
    std::mutex swap_mutex;

public:
    GameStateManager() = default;

    // Получить буфер для чтения (thread-safe)
    const GameState& get_read_buffer() const {
        return buffers[read_index.load(std::memory_order_acquire)];
    }

    // Получить буфер для записи
    GameState& get_write_buffer() {
        return buffers[write_index.load(std::memory_order_relaxed)];
    }

    // Переключить буферы
    void swap_buffers() {
        std::lock_guard<std::mutex> lock(swap_mutex);
        int current_read = read_index.load(std::memory_order_relaxed);
        int current_write = write_index.load(std::memory_order_relaxed);

        read_index.store(current_write, std::memory_order_release);
        write_index.store(current_read, std::memory_order_relaxed);
    }
};

// Глобальный экземпляр (объявление)
extern GameStateManager g_game_state;