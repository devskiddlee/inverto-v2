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
		vel = G::entities[0].absVelocity.length();
	}
	float ownVel = G::localPlayer.absVelocity.length();

	float repeats = speed + vel + ownVel;
	x = x / 360 * repeats;
	y = y / 360 * repeats;
	
	if (G::S.strictMouseAim) {
		if (x > 0.f && x > G::S.strictMouseAimThreshold && x < 0.5f) x = 1.f;
		if (x < 0.f && x < -G::S.strictMouseAimThreshold && x > -0.5f) x = -1.f;

		if (y > 0.f && y > G::S.strictMouseAimThreshold && y < 0.5f) y = 1.f;
		if (y < 0.f && y < -G::S.strictMouseAimThreshold && y > -0.5f) y = -1.f;
	}

	mouse_event(0x0001, (DWORD)(-round(x)), (DWORD)round(y), 0, 0);
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

void GetLastRecoilPunch(Vector* punch) {
	C_UTL_VECTOR<QAngle> aimPunchCache = G::memory.Read<C_UTL_VECTOR<QAngle>>(G::localPlayer.address + G::offsets.aimPunchAngle + 36);
	QAngle a = G::memory.Read<QAngle>((uintptr_t)aimPunchCache.Data + (aimPunchCache.Count - 1) * sizeof(QAngle));
	*punch = Vector(a.yaw, a.pitch, 0);
}

void aim_and_shoot(const Entity& e, float speed) {
	if (!G::lowerVisibleMap[e.id] && !e.visible && !G::S.ignoreVisible) return;

	int entityindex = G::memory.Read<int>(G::localPlayer.address + G::offsets.IDEntIndex);
	Vector angles = CalcAngles(G::localPlayer.head, e.head);

	if (G::S.aimbotSmart) {
		int dmg = GetDamageOfCurrentWeapon(HitGroup_Chest_Arm_Neck, &G::render_entities[0]);
		if ((!e.visible && !G::S.ignoreVisible) || (dmg > G::entities[0].health && G::lowerVisibleMap[e.id]))
			angles = CalcAngles(G::localPlayer.head, GetBone(&e, LowerChest));
	}

	Vector aimPunch;
	GetLastRecoilPunch(&aimPunch);
	int shotsFired = G::memory.Read<int>(G::localPlayer.address + G::offsets.iShotsFired);

	// Check if RCS is on, if player is spraying and avoid garbage values
	// (20.f limit is arbitrary, most weapons don't even go over 5.f)
	if (G::S.rcs && shotsFired > 1 && aimPunch.length() < 20.f) {
		angles.y -= aimPunch.y * 2.f;
		angles.x -= aimPunch.x * 2.f;
	}

	aim_at(angles, speed);

	if (G::S.triggerbot && entityindex != -1) {
		if ((G::localPlayer.absVelocity.z > 1 || G::localPlayer.absVelocity.z < -1) && G::S.jumpShotHack && G::weaponName == "weapon_ssg08")
			return;

		G::shoot = true;
	}
}

class Aimbot {
public:
	static void OnTick(const TickEvent& event) {
		float speed = G::S.aimbotspeed * event.delta_time * 100 * GetFovScaleFactor(90.f);

		bool aim = true;
		if (G::S.aimAtDelay > 0.f && !G::S.ignoreVisible && G::entities.size() > 0) {
			aim = (G::S.aimAtDelay / 1000.f) < G::timeVisibleMap[G::entities[0].id];
		}

		if (G::S.aimbot && G::entities.size() > 0) {
			if (GetAsyncKeyState(G::S.AIMBOT_KEY)) {
				if ((G::entities[0].angleDiff < G::S.maxAngleDiffAimbot * GetFovScaleFactor(90.f)) || G::S.disableAngleDiff) {
					if (G::time_alive[G::entities[0].id] > 0.5f && aim) {
						aim_and_shoot(G::entities[0], speed);
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
			if (v && G::time_alive[to_aim_at.id] > 0.5f && aim) {
				aim_and_shoot(to_aim_at, speed);
			}
		}
	}
};