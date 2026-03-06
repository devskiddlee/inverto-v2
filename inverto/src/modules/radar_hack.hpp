#include "module_includes.h"

#define DEG2RAD(deg) ((deg)*(M_PI/180.0f))

static constexpr float RadarConstantFromMap(const char* map) {
    switch (Hash(map)) {
    case Hash("de_nuke"):
        return 12.f;
    case Hash("de_stronghold"):
        return 10.f;
    case Hash("de_warden"):
        return 9.6f;
    case Hash("de_overpass"):
    case Hash("de_anubis"):
        return 9.f;
    default:
    case Hash("de_ancient"):
    case Hash("de_ancient_night"):
    case Hash("de_inferno"):
    case Hash("de_mirage"):
        return 8.5f;
    case Hash("de_dust2"):
        return 7.5f;
    case Hash("cs_office"):
        return 7.f;
    case Hash("de_train"):
        return 7.f;
    case Hash("de_vertigo"):
        return 6.8f;
    case Hash("de_sanctum"):
        return 5.9f;
    case Hash("de_poseidon"):
        return 5.1f;
    }
}

class RadarHack {
private:
    static ImVec2 WorldToRadar(Vector target)
    {
        ImVec2 center = {
            G::S.radarOffset + G::S.radarSize / 2.f,
            G::S.radarOffset + G::S.radarSize / 2.f
        };

        Vector origin = G::localPlayer.origin;

        float forward = target.x - origin.x;
        float right = origin.y - target.y;

        float scale = G::S.radarZoom / RadarConstantFromMap(G::mapName.c_str());

        forward *= scale;
        right *= scale;

        float yaw = -DEG2RAD(G::memory.Read<float>(G::client + G::offsets.viewangles + 0x4));

        float cosYaw = std::cos(yaw);
        float sinYaw = std::sin(yaw);

        float localX = right * cosYaw - forward * sinYaw;
        float localY = right * sinYaw + forward * cosYaw;

        float relative = G::S.radarSize / 2.f;

        localX /= relative;
        localY /= relative;

        if (G::S.radarBorder) {
            float length = std::sqrt(localX * localX + localY * localY);
            if (length > 1.f) {
                localX /= length;
                localY /= length;
            }
        }

        return {
            center.x + localX * relative,
            center.y - localY * relative
        };
    }
public:
	static void OnRender(const RenderEvent& event) {
		if (!G::S.radarHack) return;

        if (G::renderRadarBox)
		    event.drawList->AddRect(
			    { G::S.radarOffset, G::S.radarOffset },
			    {
                    G::S.radarOffset + G::S.radarSize,
                    G::S.radarOffset + G::S.radarSize
                },
			    0xff0000ff
		    );

        for (auto& e : G::render_entities) {
            if (G::S.radarHackPointFilled)
                event.drawList->AddCircleFilled(
                    WorldToRadar(e.origin),
                    G::S.radarHackPointSize,
                    G::S.radarHackColor
                );
            else
                event.drawList->AddCircle(
                    WorldToRadar(e.origin),
                    G::S.radarHackPointSize,
                    G::S.radarHackColor
                );
        }
	}
};