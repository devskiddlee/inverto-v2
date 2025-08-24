#include "module_includes.h"

void move_mouse(float x, float y, float speed)
{
	float vel = 0;
	if (G::entities.size() > 0)
	{
		vel = abs(G::nearest_player.absVeloctiy.x) + abs(G::nearest_player.absVeloctiy.y) + abs(G::nearest_player.absVeloctiy.z);
	}
	float ownVel = abs(G::localPlayer.absVeloctiy.x) + abs(G::localPlayer.absVeloctiy.y) + abs(G::localPlayer.absVeloctiy.z);

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

class Aimbot {
public:
	static void OnTick(TickEvent event) {
		if (G::S.aimbot && G::entities.size() > 0) {
			if (GetAsyncKeyState(G::S.AIMBOT_KEY)) {
				calculateRecoilOffset();
				float speed = G::S.aimbotspeed * event.delta_time * 100 * (90.f / G::fov);
				int entityindex = G::memory.Read<int>(G::localPlayer.address + G::offsets.IDEntIndex);
				if ((G::nearest_player.angleDiff < G::S.maxAngleDiffAimbot * (90.f / G::fov)) || G::S.disableAngleDiff) {
					if ((G::nearest_player.visible || G::S.ignoreVisible) && G::time_alive[G::nearest_player.id] > 0.5f) {
						Vector angles = CalcAngles(G::localPlayer.head, G::nearest_player.head);
						if (recoilpoints.size() > 0 && G::S.rcs)
							aim_at(Vector(angles.x - recoilpoints.back().x / easingFactor, angles.y + recoilpoints.back().y / easingFactor, angles.z), speed);
						else
							aim_at(angles, speed);

						if (G::S.triggerbot && entityindex != -1) {
							if ((G::localPlayer.absVeloctiy.z > 1 || G::localPlayer.absVeloctiy.z < -1) && G::weaponName == "weapon_ssg08")
								return;

							G::shoot = true;
						}
					}
				}
			}
		}
	}
};