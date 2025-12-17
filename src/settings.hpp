#pragma once

class settings
{
public:
	bool menu_opened = true;

	// HOTKEYS
	struct {
		int menu_toggle = VK_INSERT;
		int exit_program = VK_END;
	}hotkeys;

	struct {
		bool no_recoil = false;
		bool no_spread = false;
		bool one_shoot_kill = false;
		bool trigger_bot = false;
		bool aimbot = false;
		bool aimbot_player_only = false;
		// НОВОЕ: Smooth Aimbot
		bool aimbot_smooth = false;
		float aimbot_smooth_factor = 0.5f; // 0.0 = мгновенно, 1.0 = очень медленно
	}weapon;
	struct {
		bool never_wanted = false;
	}self;
	struct {
		bool enable = false;
		ImColor color = ImColor(255, 255, 255);
		struct {
			bool enable = false;
			bool line = true;
			bool text = true;
			ImColor color = ImColor(255, 255, 255);
		}vehicle;
		struct {
			bool enable = false;
			bool line = true;
			bool text = true;
			bool bone = true;
			bool box = true;
			bool health_bar = true; // НОВОЕ: Health bars
			ImColor color = ImColor(255, 255, 255);
			bool exclude_self = true;
		}ped;
		struct {
			bool enable = false;
			bool line = true;
			bool text = true;
			ImColor color = ImColor(255, 255, 255);
		}pickup;
		struct {
			bool enable = false;
			bool line = true;
			bool text = true;
			ImColor color = ImColor(255, 255, 255);
		}object;
		// НОВОЕ: Настройки отображения
		struct {
			int snapline_position = 0; // 0=top, 1=center, 2=bottom
			int box_style = 0; // 0=full, 1=corners, 2=3d
			bool distance_fade = true;
			float max_distance = 500.0f;
		}display;
	}esp;

	// НОВОЕ: Aimbot расширенные настройки
	struct {
		bool show_fov = true;
		float fov_size = 300.0f;
		ImColor fov_color = ImColor(255, 255, 255, 100);
		int target_priority = 0; // 0=crosshair, 1=distance, 2=low_hp
		int target_bone = 0; // 0=head, 1=neck, 2=chest
		bool prediction = false;
		float prediction_factor = 1.0f;
	}aimbot_advanced;

	settings()
	{
		std::ifstream f("settings.json");
		try
		{
			nlohmann::json j = nlohmann::json::parse(f);
			menu_opened = j["menu_opened"];

			// Hotkeys
			if (j.contains("hotkeys"))
			{
				if (j["hotkeys"].contains("menu_toggle"))
					hotkeys.menu_toggle = j["hotkeys"]["menu_toggle"];
				if (j["hotkeys"].contains("exit_program"))
					hotkeys.exit_program = j["hotkeys"]["exit_program"];
			}

			weapon.no_recoil = j["weapon"]["no_recoil"];
			weapon.no_spread = j["weapon"]["no_spread"];
			weapon.one_shoot_kill = j["weapon"]["one_shoot_kill"];
			weapon.trigger_bot = j["weapon"]["trigger_bot"];
			weapon.aimbot = j["weapon"]["aimbot"];
			weapon.aimbot_player_only = j["weapon"]["aimbot_player_only"];
			// НОВОЕ: Загрузка smooth aimbot настроек
			if (j["weapon"].contains("aimbot_smooth"))
				weapon.aimbot_smooth = j["weapon"]["aimbot_smooth"];
			if (j["weapon"].contains("aimbot_smooth_factor"))
				weapon.aimbot_smooth_factor = j["weapon"]["aimbot_smooth_factor"];

			self.never_wanted = j["self"]["never_wanted"];
			esp.enable = j["esp"]["enable"];
			esp.color = (ImColor)j["esp"]["color"];
			esp.vehicle.enable = j["esp"]["vehicle"]["enable"];
			esp.vehicle.line = j["esp"]["vehicle"]["line"];
			esp.vehicle.text = j["esp"]["vehicle"]["text"];
			esp.vehicle.color = (ImColor)j["esp"]["vehicle"]["color"];
			esp.ped.enable = j["esp"]["ped"]["enable"];
			esp.ped.line = j["esp"]["ped"]["line"];
			esp.ped.text = j["esp"]["ped"]["text"];
			esp.ped.bone = j["esp"]["ped"]["bone"];
			esp.ped.box = j["esp"]["ped"]["box"];
			esp.ped.color = (ImColor)j["esp"]["ped"]["color"];
			esp.ped.exclude_self = j["esp"]["ped"]["exclude_self"];
			if (j["esp"]["ped"].contains("health_bar"))
				esp.ped.health_bar = j["esp"]["ped"]["health_bar"];

			esp.pickup.enable = j["esp"]["pickup"]["enable"];
			esp.pickup.line = j["esp"]["pickup"]["line"];
			esp.pickup.text = j["esp"]["pickup"]["text"];
			esp.pickup.color = (ImColor)j["esp"]["pickup"]["color"];
			esp.object.enable = j["esp"]["object"]["enable"];
			esp.object.line = j["esp"]["object"]["line"];
			esp.object.text = j["esp"]["object"]["text"];
			esp.object.color = (ImColor)j["esp"]["object"]["color"];

			// ESP Display settings
			if (j["esp"].contains("display"))
			{
				if (j["esp"]["display"].contains("snapline_position"))
					esp.display.snapline_position = j["esp"]["display"]["snapline_position"];
				if (j["esp"]["display"].contains("box_style"))
					esp.display.box_style = j["esp"]["display"]["box_style"];
				if (j["esp"]["display"].contains("distance_fade"))
					esp.display.distance_fade = j["esp"]["display"]["distance_fade"];
				if (j["esp"]["display"].contains("max_distance"))
					esp.display.max_distance = j["esp"]["display"]["max_distance"];
			}

			// Aimbot Advanced
			if (j.contains("aimbot_advanced"))
			{
				if (j["aimbot_advanced"].contains("show_fov"))
					aimbot_advanced.show_fov = j["aimbot_advanced"]["show_fov"];
				if (j["aimbot_advanced"].contains("fov_size"))
					aimbot_advanced.fov_size = j["aimbot_advanced"]["fov_size"];
				if (j["aimbot_advanced"].contains("fov_color"))
					aimbot_advanced.fov_color = (ImColor)j["aimbot_advanced"]["fov_color"];
				if (j["aimbot_advanced"].contains("target_priority"))
					aimbot_advanced.target_priority = j["aimbot_advanced"]["target_priority"];
				if (j["aimbot_advanced"].contains("target_bone"))
					aimbot_advanced.target_bone = j["aimbot_advanced"]["target_bone"];
				if (j["aimbot_advanced"].contains("prediction"))
					aimbot_advanced.prediction = j["aimbot_advanced"]["prediction"];
				if (j["aimbot_advanced"].contains("prediction_factor"))
					aimbot_advanced.prediction_factor = j["aimbot_advanced"]["prediction_factor"];
			}
		}
		catch (...)
		{
			spdlog::warn("Failed to load settings.json, using defaults");
		}
	}
	~settings()
	{
		nlohmann::json j;
		j["menu_opened"] = menu_opened;

		// Hotkeys
		j["hotkeys"]["menu_toggle"] = hotkeys.menu_toggle;
		j["hotkeys"]["exit_program"] = hotkeys.exit_program;

		j["weapon"]["no_recoil"] = weapon.no_recoil;
		j["weapon"]["no_spread"] = weapon.no_spread;
		j["weapon"]["one_shoot_kill"] = weapon.one_shoot_kill;
		j["weapon"]["trigger_bot"] = weapon.trigger_bot;
		j["weapon"]["aimbot"] = weapon.aimbot;
		j["weapon"]["aimbot_player_only"] = weapon.aimbot_player_only;
		// НОВОЕ: Сохранение smooth aimbot настроек
		j["weapon"]["aimbot_smooth"] = weapon.aimbot_smooth;
		j["weapon"]["aimbot_smooth_factor"] = weapon.aimbot_smooth_factor;

		j["self"]["never_wanted"] = self.never_wanted;
		j["esp"]["enable"] = esp.enable;
		j["esp"]["color"] = (ImU32)esp.color;
		j["esp"]["vehicle"]["enable"] = esp.vehicle.enable;
		j["esp"]["vehicle"]["line"] = esp.vehicle.line;
		j["esp"]["vehicle"]["text"] = esp.vehicle.text;
		j["esp"]["vehicle"]["color"] = (ImU32)esp.vehicle.color;
		j["esp"]["ped"]["enable"] = esp.ped.enable;
		j["esp"]["ped"]["line"] = esp.ped.line;
		j["esp"]["ped"]["text"] = esp.ped.text;
		j["esp"]["ped"]["bone"] = esp.ped.bone;
		j["esp"]["ped"]["box"] = esp.ped.box;
		j["esp"]["ped"]["color"] = (ImU32)esp.ped.color;
		j["esp"]["ped"]["exclude_self"] = esp.ped.exclude_self;
		j["esp"]["ped"]["health_bar"] = esp.ped.health_bar;

		j["esp"]["pickup"]["enable"] = esp.pickup.enable;
		j["esp"]["pickup"]["line"] = esp.pickup.line;
		j["esp"]["pickup"]["text"] = esp.pickup.text;
		j["esp"]["pickup"]["color"] = (ImU32)esp.pickup.color;
		j["esp"]["object"]["enable"] = esp.object.enable;
		j["esp"]["object"]["line"] = esp.object.line;
		j["esp"]["object"]["text"] = esp.object.text;
		j["esp"]["object"]["color"] = (ImU32)esp.object.color;

		// ESP Display
		j["esp"]["display"]["snapline_position"] = esp.display.snapline_position;
		j["esp"]["display"]["box_style"] = esp.display.box_style;
		j["esp"]["display"]["distance_fade"] = esp.display.distance_fade;
		j["esp"]["display"]["max_distance"] = esp.display.max_distance;

		// Aimbot Advanced
		j["aimbot_advanced"]["show_fov"] = aimbot_advanced.show_fov;
		j["aimbot_advanced"]["fov_size"] = aimbot_advanced.fov_size;
		j["aimbot_advanced"]["fov_color"] = (ImU32)aimbot_advanced.fov_color;
		j["aimbot_advanced"]["target_priority"] = aimbot_advanced.target_priority;
		j["aimbot_advanced"]["target_bone"] = aimbot_advanced.target_bone;
		j["aimbot_advanced"]["prediction"] = aimbot_advanced.prediction;
		j["aimbot_advanced"]["prediction_factor"] = aimbot_advanced.prediction_factor;

		std::ofstream o("settings.json");
		o << std::setw(4) << j << std::endl;
	}
};

inline std::unique_ptr<settings> g_settings;