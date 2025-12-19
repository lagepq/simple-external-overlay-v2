#pragma once

#include <iostream>
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <fstream>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "types.hpp"

inline std::atomic_bool g_running = true;

inline DWORD g_process_id;
inline HANDLE g_handle;

inline uintptr_t ReplayInterfacePointer;
inline uintptr_t CPedFactoryPointer;
inline uintptr_t CViewportGamePointer;
inline uintptr_t WindowWidth;
inline uintptr_t WindowHeight;
inline uintptr_t camGameplayDirectorPointer;
inline uintptr_t CNetworkPlayerMgrPointer;
inline uintptr_t AimCPedPointer;

#include "util/misc.hpp"
#include "game_state.hpp"
#include "settings.hpp"
#include "signature.hpp"
#include "functions/functions.hpp"

// CAutomobile : CVehicle : CPhysical : CDynamicEntity : CEntity : rage::fwEntity : rage::fwExtensibleBase : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// fragInstGta : rage::phfwFragInst : rage::fragInst : rage::phInstBreakable : rage::phInst : rage::pgBase
// CPed : CPhysical : CDynamicEntity : CEntity : rage::fwEntity : rage::fwExtensibleBase : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// phInstGta : rage::phfwInst : rage::phInst : rage::pgBase
// CPickup : CObject : CPhysical : CDynamicEntity : CEntity : rage::fwEntity : rage::fwExtensibleBase : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// phInstGta : rage::phfwInst : rage::phInst : rage::pgBase
// CObject : CPhysical : CDynamicEntity : CEntity : rage::fwEntity : rage::fwExtensibleBase : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// phInstGta : rage::phfwInst : rage::phInst : rage::pgBase
// CPedFactory
// CPed : CPhysical : CDynamicEntity : CEntity : rage::fwEntity : rage::fwExtensibleBase : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// CViewportGame : CViewport
// CViewportPrimaryOrtho : CViewport
// CViewportFrontend3DScene : CViewport
// camGameplayDirector : camBaseDirector : camBaseObject : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// camFollowPedCamera : camFollowCamera : camThirdPersonCamera : camBaseCamera : camBaseObject : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// CNetworkPlayerMgr : rage::netPlayerMgrBase
// CNetGamePlayer : rage::netPlayer
// CPlayerInfo : rage::fwExtensibleBase : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// CPed : CPhysical : CDynamicEntity : CEntity : rage::fwEntity : rage::fwExtensibleBase : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// CPedWeaponManager : CInventoryListener : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// CWeaponInfo : CItemInfo : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// CPedInventory : CWeaponObserver : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// camFollowPedCamera : camFollowCamera : camThirdPersonCamera : camBaseCamera : camBaseObject : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// camFollowPedCameraMetadata : camFollowCameraMetadata : camThirdPersonCameraMetadata : camBaseCameraMetadata : camBaseObjectMetadata : rage::datBase
// camFirstPersonShooterCamera : camFirstPersonAimCamera : camAimCamera : camBaseCamera : camBaseObject : rage::fwRefAwareBase : rage::fwRefAwareBaseImpl<rage::datBase> : rage::datBase
// camFirstPersonShooterCameraMetadata : camFirstPersonAimCameraMetadata : camAimCameraMetadata : camBaseCameraMetadata : camBaseObjectMetadata : rage::datBase

// Model Coordinates
// World Coordinates
// Camera (View) Coordinates
// Clip Coordinates
// Image Coordinates

// Model Matrix (Model Coordinates -> World Coordinates)
// View Matrix (World Coordinates -> Camera (View) Coordinates)
// Projection Matrix (Camera (View) Coordinates -> Clip Coordinates)

// Local Space (Object Space)
// World Space
// View Space (Eye Space)
// Clip Space
// Screen Space