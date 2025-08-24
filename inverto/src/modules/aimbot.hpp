#include "module_includes.h"

void move_mouse(float x, float y, float speed)
{
	if (G::localPlayer.health <= 0)
		return;
	if (G::render_ui)
		return;

	float vel = 0;
	if (G::entities.size() > 0)
	{
		vel = G::nearest_player.absVelocity.length();
	}
	float ownVel = G::localPlayer.absVelocity.length();

	x = x / 360;
	y = y / 360;
	int repeats = (int)(speed + round(vel * 12) + round(ownVel * 12));
	if (repeats > speed * 1.5) repeats = (int)round(speed * 1.5);

	mouse_event(0x0001, (DWORD)(-round(x * repeats)), (DWORD)round(y * repeats), 0, 0);
}


void aim_at(Vector angles, float speed)
{
	float moveY = angles.y - G::memory.Read<float>(G::client + G::offsets.viewangles);
	float moveX = angles.x - G::memory.Read<float>(G::client + G::offsets.viewangles + 0x4);

	if (moveX > 70)
	{
		moveX = moveX - 360;
	}
	if (moveX < -70)
	{
		moveX = moveX + 360;
	}

	move_mouse(moveX, moveY, speed);
}

Vector newAngles = Vector(0, 0, 0);
Vector oldAngles = Vector(0, 0, 0);

float m_pitch = 0.022f;
float m_yaw = 0.022f;
float sens = 2.0f;

std::list<Vector> recoilpoints;
Vector recoilreference;

void calculateRecoilOffset()
{
	int ShotsFired = G::memory.Read<int>(G::localPlayer.address + G::offsets.iShotsFired);

	if (ShotsFired > 1)
	{
		Vector aimPunch = G::memory.Read<Vector>(G::localPlayer.address + G::offsets.aimPunchAngle);
		newAngles.x = (aimPunch.y - oldAngles.y) * 2.f / (m_pitch * sens) / 1;
		newAngles.y = -(aimPunch.x - oldAngles.x) * 2.f / (m_yaw * sens) / 1;

		if (newAngles != Vector(0, 0, 0))
		{
			Vector nw = recoilreference + newAngles;
			recoilpoints.push_back(nw);
			recoilreference = nw;
		}

		oldAngles = aimPunch;
	}
	else
	{
		recoilpoints.clear();
		recoilreference = Vector(0, 0, 0);
		oldAngles = Vector(0, 0, 0);
	}
}

float easingFactor = 23.f;

void aim_and_shoot(Entity e, float speed) {
	calculateRecoilOffset();

	int entityindex = G::memory.Read<int>(G::localPlayer.address + G::offsets.IDEntIndex);
	Vector angles = CalcAngles(G::localPlayer.head, e.head);
	if (recoilpoints.size() > 0 && G::S.rcs)
		aim_at(Vector(angles.x - recoilpoints.back().x / easingFactor, angles.y + recoilpoints.back().y / easingFactor, angles.z), speed);
	else
		aim_at(angles, speed);

	if (G::S.triggerbot && entityindex != -1) {
		if ((G::localPlayer.absVelocity.z > 1 || G::localPlayer.absVelocity.z < -1) && G::S.jumpShotHack && G::weaponName == "weapon_ssg08")
			return;

		if (G::S.onlyShootWhenStill && G::localPlayer.absVelocity.length() > 50.f)
			return;

		G::shoot = true;
	}
}

class Aimbot {
public:
	static void OnTick(TickEvent event) {
		float speed = G::S.aimbotspeed * event.delta_time * 100 * (90.f / G::fov);
		if (G::S.aimbot && G::entities.size() > 0) {
			if (GetAsyncKeyState(G::S.AIMBOT_KEY)) {
				if ((G::nearest_player.angleDiff < G::S.maxAngleDiffAimbot * (90.f / G::fov)) || G::S.disableAngleDiff) {
					if ((G::nearest_player.visible || G::S.ignoreVisible) && G::time_alive[G::nearest_player.id] > 0.5f) {
						aim_and_shoot(G::nearest_player, speed);
					}
				}
			}
		}
		if (G::S.autoAimWhenVisible && G::entities.size() > 0) {
			Entity to_aim_at;
			bool v = false;
			for (Entity& e : G::entities) {
				if (e.visible) {
					to_aim_at = e;
					v = true;
					break;
				}
			}
			if (v && G::time_alive[to_aim_at.id] > 0.5f) {
				aim_and_shoot(to_aim_at, speed);
			}
		}
	}
};