#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <gcclib/util.h>

#include <il2cpp-appdata.h>

#include <gcclib/Logger.h>

template<class type>
struct EntityFilterField
{
	bool enabled;
	type value;
};

struct SimpleEntityFilter 
{
	EntityFilterField<app::EntityType__Enum_1> typeFilter;
	EntityFilterField<std::vector<std::string>> nameFilter;
};

bool GetResourceMemory(HINSTANCE hInstance, int resId, LPBYTE& pDest, DWORD& size);
int64_t GetCurrentTimeMillisec();

// Position utils
app::Vector3 GetRelativePosition(app::BaseEntity* entity);
void SetRelativePosition(app::BaseEntity* entity, app::Vector3 position);
app::Vector3 GetAvatarRelativePosition();

app::BaseEntity* GetAvatarEntity();
app::CameraEntity* GetMainCameraEntity();
void SetAvatarRelativePosition(app::Vector3 position);
float GetDistToAvatar(app::BaseEntity* entity);

std::vector<app::BaseEntity*> GetEntities();
app::BaseEntity* GetEntityByRuntimeId(uint32_t runtimeId);
uint32_t GetAvatarRuntimeId();

// Entity filters
typedef bool (*FilterFunc)(app::BaseEntity* entity);
std::vector<app::BaseEntity*> FindEntities(FilterFunc func);
std::vector<app::BaseEntity*> FindEntities(const SimpleEntityFilter& filter);
app::BaseEntity* FindNearestEntity(const SimpleEntityFilter& filter);
bool IsEntityFilterValid(app::BaseEntity* entity, const SimpleEntityFilter& filter);

const SimpleEntityFilter& GetFilterCrystalShell();
const SimpleEntityFilter& GetFilterChest();
const SimpleEntityFilter& GetMonsterFilter();

bool IsEntityCrystalShell(app::BaseEntity* entity);
