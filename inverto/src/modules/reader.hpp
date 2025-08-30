#include "module_includes.h"

void trim(std::string& input) {
	std::list<int> remove;
	for (int i = 0; i < input.size(); i++) {
		char c = input[i];
		if ((int)c == 0) {
			remove.push_front(i);
		}
	}
	for (int index : remove) {
		input.erase(index);
	}
}

void UpdateEntity(Entity& entity) {
	entity.origin = G::memory.Read<Vector>(entity.address + G::offsets.origin);
	entity.viewOffset = Vector(0.f, 0.f, 65.f);
	entity.abs = entity.origin.copy() + entity.viewOffset;
	uintptr_t sceneNode = G::memory.Read<uintptr_t>(entity.address + G::offsets.gameScene);
	uintptr_t boneMatrix = G::memory.Read<uintptr_t>(sceneNode + G::offsets.modelState + G::offsets.boneArray);
	entity.head = G::memory.Read<Vector>(boneMatrix + bone_id_map[BONE_HEAD] * 32);

	entity.angleEye = G::memory.Read<Vector>(entity.address + G::offsets.eyeAngles);
	entity.health = G::memory.Read<int>(entity.address + G::offsets.health);
	entity.teamNum = G::memory.Read<int>(entity.address + G::offsets.teamNum);
	entity.absVelocity = G::memory.Read<Vector>(entity.address + G::offsets.absVelocity);
	entity.magnitude = CalcMagnitude(G::localPlayer.origin, entity.origin);

	ViewMatrix currentVM = G::memory.Read<ViewMatrix>(G::client + G::offsets.viewmatrix);
	world_to_screen(entity.head, entity.headScreenPos, currentVM);
	world_to_screen(entity.origin, entity.originScreenPos, currentVM);

	for (auto bone_id : bone_id_map) {
		Vector wp = G::memory.Read<Vector>(boneMatrix + bone_id.second * 32);
		entity.bone_pos.push_back(wp);

		Vector sp = Vector(0, 0, 0);
		world_to_screen(wp, sp, currentVM);
		if (sp != Vector(0, 0, 0))
			entity.bone_screen_pos.push_back(sp);
	}

	entity.angleDiff = CalcPixelDist(G::windowCenter, entity.headScreenPos);
}

bool compareAngleDiff(const Entity& e1, const Entity& e2) {
	return e1.angleDiff < e2.angleDiff;
}

void UpdateEntities(std::list<Entity>& entities)
{
	uintptr_t entityList = G::memory.Read<uintptr_t>(G::client + G::offsets.entityList);

	uintptr_t listEntry = G::memory.Read<uintptr_t>(entityList + 0x10);

	for (int i = 0; i < 64; i++)
	{
		uintptr_t currController = G::memory.Read<uintptr_t>(listEntry + i * 0x78);
		if (!currController) continue;

		int pawnHandle = G::memory.Read<int>(currController + G::offsets.playerpawn);
		if (pawnHandle == 0) continue;

		uintptr_t listEntry2 = G::memory.Read<uintptr_t>(entityList + 0x8 * ((pawnHandle & 0x7FFF) >> 9) + 0x10);
		if (!listEntry2) continue;

		uintptr_t currPawn = G::memory.Read<uintptr_t>(listEntry2 + 0x78 * (pawnHandle & 0x1FF));
		if (!currPawn) continue;

		Entity entity;
		entity.address = currPawn;

		UpdateEntity(entity);

		entity.dist = CalcMagnitude(Vector(G::localPlayer.origin.x, G::localPlayer.origin.y, 0), Vector(entity.origin.x, entity.origin.y, 0));
		entity.pos_offset = Vector(entity.origin.x - G::localPlayer.origin.x, entity.origin.y - G::localPlayer.origin.y, 0);

		std::ostringstream ss;
		for (int i = 0; i < 16; i++) {
			char buffer = { 0 };

			G::memory.ReadRaw(currController + G::offsets.playerName + i, &buffer, sizeof(buffer));

			ss << buffer;
		}

		std::string name = ss.str();
		trim(name);
		entity.name = name;
		entity.steam_id = G::memory.Read<uint64_t>(currController + G::offsets.steamid);

		entity.id = str(entity.address) + entity.name;
		entity.visible = G::visibleMap[entity.id];

		if (entity.health < 1 || entity.health > 100) {
			G::time_alive[entity.id] = 0.f;
			continue;
		}

		if (entity.origin.operator==(Vector(0.f, 0.f, 0.f)))
			continue;

		if (entity.teamNum == G::localPlayer.teamNum && G::S.teamCheck)
			continue;

		if (entity.address == G::localPlayer.address)
			continue;

		bool same_origin = false;
		for (Entity& e : entities) {
			if (e.origin.operator==(entity.origin)) {
				same_origin = true;
				break;
			}
		}

		if (!same_origin)
		{
			entities.push_back(entity);
		}
	}
}

void ReloadEntities(std::list<Entity>& entities)
{
	G::localPlayerController = G::memory.Read<uintptr_t>(G::client + G::offsets.localController);
	G::localPlayer.address = G::memory.Read<uintptr_t>(G::client + G::offsets.localPlayer);

	UpdateEntity(G::localPlayer);
	G::localPlayer.steam_id = G::memory.Read<uint64_t>(G::localPlayerController + G::offsets.steamid);

	UpdateEntities(entities);
}

struct EntityVisible {
	Entity& e;
	bool visible;
};

class Reader {
public:
	static void OnTick(TickEvent event) {

		std::list<Entity> entities;
		ReloadEntities(entities);

		float closest_dist = 0.f;
		Entity temp_nearest_player;
		for (Entity& player : entities) {
			G::time_alive[player.id] += event.delta_time;

			if (closest_dist > player.angleDiff || closest_dist == 0.f) {
				closest_dist = player.angleDiff;
				temp_nearest_player = player;
			}
		}

		G::nearest_player = Entity(temp_nearest_player);
		G::entities = std::list<Entity>(entities);
	}

	static void OnRender(RenderEvent event) {
		std::list<Entity> entities;
		ReloadEntities(entities);

		float closest_dist = 0.f;
		Entity temp_nearest_player;
		for (Entity& player : entities) {
			if (closest_dist > player.angleDiff || closest_dist == 0.f) {
				closest_dist = player.angleDiff;
				temp_nearest_player = player;
			}
		}

		G::render_nearest_player = Entity(temp_nearest_player);
		G::render_entities = std::list<Entity>(entities);
	}

	static std::list<Entity> GetEntities() {
		std::list<Entity> entities;
		ReloadEntities(entities);
		return entities;
	}
};