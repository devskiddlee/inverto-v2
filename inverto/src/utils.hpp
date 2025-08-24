#include <list>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstddef>
#include <memory>
#include <iostream>
#include <set>
#include <imgui/imgui.h>
#include <filesystem>

template <class T>
static T get_object_at_index(std::list<T> list, int index) {
	int ix = 0;
	for (auto obj : list) {
		if (ix == index)
			return obj;
		ix++;
	}
	return list.back();
}

struct Vector {
	Vector() noexcept
		: x(), y(), z() {
	}

	Vector(float x, float y, float z) noexcept
		: x(x), y(y), z(z) {
	}

	Vector& operator+(const Vector& v) noexcept {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Vector& operator-(const Vector& v) noexcept {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	Vector& operator*(const Vector& v) noexcept {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	Vector& operator*(const float& v) noexcept {
		x *= v;
		y *= v;
		z *= v;
		return *this;
	}

	Vector& operator/(const Vector& v) noexcept {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	bool operator==(const Vector& v) const {
		bool b = true;
		if (x != v.x)
			b = false;
		if (y != v.y)
			b = false;
		if (z != v.z)
			b = false;
		return b;
	}

	std::string to_str() {
		std::ostringstream ss;
		ss << "Vector(";
		ss << x;
		ss << ", ";
		ss << y;
		ss << ", ";
		ss << z;
		ss << ")";
		return ss.str();
	}

	Vector copy() noexcept {
		return Vector(x, y, z);
	}

	ImVec2 toVec2() noexcept {
		return ImVec2({ x, y });
	}

	float dot(const Vector& other) const {
		return x * other.x + y * other.y + z * other.z;
	}

	Vector cross(const Vector& other) const {
		return Vector(
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		);
	}

	float cross2D(const Vector& other) const {
		return x * other.y - y * other.x;
	}

	float length() const {
		return sqrt(x*x + y*y + z*z);
	}

	Vector normalized() const {
		float len = length();
		if (len == 0)
			return Vector(0, 0, 0);

		return Vector(x / len, y / len, z / len);
	}

	float x, y, z;
};

float orientation(Vector& a, Vector& b, Vector& c) {
	return (b.copy() - a).cross2D(c.copy() - a);
}

bool IsPixelInsideRegion(Vector pixel, Vector min, Vector max)
{
	return pixel.x > min.x && pixel.x < max.x && pixel.y > min.y && pixel.y < max.y;
}

struct ViewMatrix {
	ViewMatrix() noexcept
		: data() {
	}

	float* operator[](int index) noexcept {
		return data[index];
	}

	const float* operator[](int index) const noexcept {
		return data[index];
	}

	float data[4][4];
};

static bool world_to_screen(const Vector& world, Vector& screen, const ViewMatrix& vm) noexcept {
	float w = vm[3][0] * world.x + vm[3][1] * world.y + vm[3][2] * world.z + vm[3][3];

	if (w < 0.001f) {
		return false;
	}

	const float x = world.x * vm[0][0] + world.y * vm[0][1] + world.z * vm[0][2] + vm[0][3];
	const float y = world.x * vm[1][0] + world.y * vm[1][1] + world.z * vm[1][2] + vm[1][3];

	w = 1.f / w;
	float nx = x * w;
	float ny = y * w;

	const ImVec2 size = ImGui::GetIO().DisplaySize;

	screen.x = (size.x * .5f * nx) + (nx + size.x * .5f);
	screen.y = -(size.y * .5f * ny) + (ny + size.y * .5f);

	return true;
}


enum bone_ids
{
	BONE_HEAD,
	BONE_NECK,
	BONE_SPINE,
	BONE_SPINE_1,
	BONE_HIP,
	BONE_LEFT_SHOULDER,
	BONE_LEFT_ARM,
	BONE_LEFT_HAND,
	BONE_RIGHT_SHOULDER,
	BONE_RIGHT_ARM,
	BONE_RIGHT_HAND,

	BONE_LEFT_HIP,
	BONE_LEFT_KNEE,
	BONE_LEFT_FEET,

	BONE_RIGHT_HIP,
	BONE_RIGHT_KNEE,
	BONE_RIGHT_FEET
};

std::map<bone_ids, int> bone_id_map{
	{BONE_HEAD, 6},
	{BONE_NECK, 0},
	{BONE_SPINE, 4},
	{BONE_SPINE_1, 2},
	{BONE_HIP, 5},
	{BONE_LEFT_SHOULDER, 8},
	{BONE_LEFT_ARM, 9},
	{BONE_LEFT_HAND, 10},
	{BONE_RIGHT_SHOULDER, 13},
	{BONE_RIGHT_ARM, 14},
	{BONE_RIGHT_HAND, 15},

	{BONE_LEFT_HIP, 22},
	{BONE_LEFT_KNEE, 23},
	{BONE_LEFT_FEET, 24},

	{BONE_RIGHT_HIP, 25},
	{BONE_RIGHT_KNEE, 26},
	{BONE_RIGHT_FEET, 27}
};

class Entity
{
public:
	uintptr_t address = 0;
	int health = 0;
	Vector origin;
	Vector head;
	Vector absVelocity;
	int teamNum = 0;
	int jumpFlag = 0;
	Vector abs;
	Vector originScreenPos;
	Vector absScreenPos;
	Vector viewOffset;
	Vector headScreenPos;
	Vector angleEye;
	bool visible = false;
	float magnitude = 0.f;
	float angleDiff = 0.f;
	int jumpButton = 0;
	std::string name;
	std::list<Vector> bone_pos;
	std::list<Vector> bone_screen_pos;
	float dist = 0.f;
	Vector pos_offset;
	std::string id = "";

	bool compare(Entity& entity) {
		if (origin.operator==(entity.origin) && health == entity.health) {
			return true;
		}
		return false;
	}

	bool operator==(const Entity& v) const {
		return origin == v.origin && health == v.health;
	}
};

float CalcMagnitude(Vector v1, Vector v2)
{
	return (float)sqrt(pow(v2.x - v1.x, 2) + pow(v2.y - v1.y, 2) + pow(v2.z - v1.z, 2));
}

float CalcPixelDist(Vector v1, Vector v2)
{
	return (float)sqrt(pow(v2.x - v1.x, 2) + pow(v2.y - v1.y, 2));
}

static bool contains(std::string first, std::string second) {
	if (first.find(second) != std::string::npos) {
		return true;
	}
	return false;
}

static std::string getBetween(std::string strSource, std::string strStart, std::string strEnd)
{
	if (contains(strSource, strStart) && contains(strSource, strEnd))
	{
		int Start, End;
		Start = (int)strSource.find(strStart, 0) + (int)strStart.length();
		End = (int)strSource.find(strEnd, Start);
		return strSource.substr(Start, End - Start);
	}

	return "";
}

int getOffset(std::string offset, std::string offset_str)
{
	try
	{
		std::string o = getBetween(offset_str, offset, ";");
		o = o.substr(4, o.length() - 4);
		int value;
		std::from_chars(o.data(), o.data() + o.size(), value, 16);
		return value;
	}
	catch (std::exception e)
	{
		return 0;
	}
}

std::vector<ImVec2> sort_points_ccw(
	const std::vector<ImVec2>& points)
{
	if (points.empty()) return {};

	// Compute centroid
	double cx = 0.0, cy = 0.0;
	for (auto& p : points) {
		cx += p.x;
		cy += p.y;
	}
	cx /= points.size();
	cy /= points.size();

	// Copy points for sorting
	std::vector<ImVec2> sorted_points = points;

	// Sort by angle relative to centroid
	std::sort(sorted_points.begin(), sorted_points.end(),
		[cx, cy](const ImVec2& a, const ImVec2& b) {
			double angleA = std::atan2(a.y - cy, a.x - cx);
			double angleB = std::atan2(b.y - cy, b.x - cx);
			return angleA < angleB;
		});

	return sorted_points;
}

class PolygonFiller
{
public:
	// Draw filled polygon (convex or concave) using ImDrawList*
	static void DrawFilledPolygon(ImDrawList* draw_list,
		const std::vector<ImVec2>& points,
		ImU32 color)
	{
		if (points.size() < 3) return;

		// If convex, ImGui has a built-in fast path
		if (IsConvex(points))
		{
			draw_list->AddConvexPolyFilled(points.data(),
				static_cast<int>(points.size()),
				color);
		}
		else
		{
			// Otherwise triangulate
			std::vector<ImVec2> pts = points;
			std::vector<int> indices(pts.size());
			for (int i = 0; i < (int)pts.size(); i++) indices[i] = i;

			while (indices.size() > 2)
			{
				bool ear_found = false;
				for (size_t i = 0; i < indices.size(); i++)
				{
					int i0 = indices[(i + indices.size() - 1) % indices.size()];
					int i1 = indices[i];
					int i2 = indices[(i + 1) % indices.size()];

					ImVec2 a = pts[i0], b = pts[i1], c = pts[i2];
					if (Cross(a, b, c) <= 0) continue; // reflex or collinear

					if (ContainsPointInTriangle(pts, indices, i0, i1, i2))
						continue;

					// It's an ear → draw triangle
					draw_list->AddTriangleFilled(a, b, c, color);

					// Remove ear tip
					indices.erase(indices.begin() + i);
					ear_found = true;
					break;
				}
				if (!ear_found)
				{
					// Degenerate / failed triangulation
					break;
				}
			}
		}
	}

private:
	// Cross product z-component (sign indicates convexity)
	static float Cross(const ImVec2& a, const ImVec2& b, const ImVec2& c)
	{
		return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
	}

	// Check if polygon is convex
	static bool IsConvex(const std::vector<ImVec2>& pts)
	{
		if (pts.size() < 4) return true;
		bool sign = Cross(pts[0], pts[1], pts[2]) > 0;
		for (size_t i = 1; i < pts.size(); i++)
		{
			float c = Cross(pts[i],
				pts[(i + 1) % pts.size()],
				pts[(i + 2) % pts.size()]);
			if ((c > 0) != sign) return false;
		}
		return true;
	}

	// Check if any other polygon vertex lies inside triangle (a,b,c)
	static bool ContainsPointInTriangle(const std::vector<ImVec2>& pts,
		const std::vector<int>& indices,
		int i0, int i1, int i2)
	{
		ImVec2 a = pts[i0], b = pts[i1], c = pts[i2];
		for (int j : indices)
		{
			if (j == i0 || j == i1 || j == i2) continue;
			if (PointInTriangle(pts[j], a, b, c))
				return true;
		}
		return false;
	}

	// Barycentric test
	static bool PointInTriangle(const ImVec2& p,
		const ImVec2& a,
		const ImVec2& b,
		const ImVec2& c)
	{
		float c1 = Cross(a, b, p);
		float c2 = Cross(b, c, p);
		float c3 = Cross(c, a, p);
		return (c1 >= 0 && c2 >= 0 && c3 >= 0) ||
			(c1 <= 0 && c2 <= 0 && c3 <= 0);
	}
};

std::vector<std::filesystem::path> get_files_with_extension(const std::filesystem::path& dir, const std::string& ext)
{
	std::vector<std::filesystem::path> result;

	if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
		return result;

	for (const auto& entry : std::filesystem::directory_iterator(dir))
	{
		if (entry.is_regular_file() && entry.path().extension() == ext)
		{
			result.push_back(entry.path());
		}
	}

	return result;
}

bool is_valid_filename(const char* name)
{
	if (!name || !*name) return false;
	std::string s(name);

	const std::string invalid = "<>:\"/\\|?*";
	if (s.find_first_of(invalid) != std::string::npos) return false;

	if (s.back() == ' ' || s.back() == '.') return false;

	static const char* reserved[] = {
		"CON","PRN","AUX","NUL",
		"COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9",
		"LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"
	};
	std::string upper;
	upper.resize(s.size());
	std::transform(s.begin(), s.end(), upper.begin(), ::toupper);
	for (auto r : reserved) {
		if (upper == r) return false;
	}

	return true;
}