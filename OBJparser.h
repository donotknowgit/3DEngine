#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>



constexpr float pi = 3.1415926535897932384626433832795;



struct vec3d {
    float x, y, z;

    vec3d(float _x = 0, float _y = 0, float _z = 0) :
        x(_x), y(_y), z(_z) {}

    vec3d rad() {
        vec3d ans(x * pi / 180.0f, y * pi / 180.0f, z * pi / 180.0f);
        return ans;
    }

    float normEuc() {
        float ans = sqrt(x * x + y * y + z * z);
        return ans;
    }

    vec3d normalize() {
        float d = sqrt(x * x + y * y + z * z);
        vec3d ans(x / d, y / d, z / d);
        return ans;
    }

    vec3d rotateVector(vec3d ang) {
        vec3d v = *this;
        // Вращение вокруг оси X
        float xRotY = v.y * cos(ang.x) - v.z * sin(ang.x);
        float xRotZ = v.y * sin(ang.x) + v.z * cos(ang.x);
        v.y = xRotY;
        v.z = xRotZ;

        // Вращение вокруг оси Y
        float yRotX = v.x * cos(ang.y) - v.z * sin(ang.y);
        float yRotZ = v.x * sin(ang.y) + v.z * cos(ang.y);
        v.x = yRotX;
        v.z = yRotZ;

        // Вращение вокруг оси Z
        float zRotX = v.x * cos(ang.z) - v.y * sin(ang.z);
        float zRotY = v.x * sin(ang.z) + v.y * cos(ang.z);
        v.x = zRotX;
        v.y = zRotY;

        return v;
    }

    vec3d operator +(vec3d a) {
        vec3d ans;
        ans.x = x + a.x;
        ans.y = y + a.y;
        ans.z = z + a.z;
        return ans;
    }

    vec3d operator -(vec3d a) {
        vec3d ans;
        ans.x = x - a.x;
        ans.y = y - a.y;
        ans.z = z - a.z;
        return ans;
    }

    vec3d operator *(float a) {
        vec3d ans(x * a, y * a, z * a);
        return ans;
    }

    vec3d operator /(float a) {
        vec3d ans(x / a, y / a, z / a);
        return ans;
    }

    bool operator ==(const vec3d& v) {
        return (x == v.x && y == v.y && z == v.z) ? 1 : 0;
    }
};

struct polygon {
    vec3d v; // индексы точек
    vec3d vn; // индексы нормалей
    sf::Color color;

    polygon(int v1 = 0, int v2 = 0, int v3 = 0, int n1 = 0, int n2 = 0, int n3 = 0, sf::Color _color = sf::Color(255, 0, 208)) {
        v.x = v1;
        v.y = v2;
        v.z = v3;
        vn.x = n1;
        vn.y = n2;
        vn.z = n3;
        color = _color;
    }

    float operator ()(int i) {
        if (i == 0) return v.x;
        else if (i == 1) return v.y;
        else if (i == 2) return v.z;
        else {
            std::cout << "wow, what a huge polygon, containing more than 3 vertices >=)\n";
            return 0;
        }
    }
};

polygon writePolygon(std::vector<std::string> parts) {
    polygon face;
    for (size_t i = 0; i < 3 && i < parts.size(); i++) {
        std::replace(parts[i].begin(), parts[i].end(), '/', ' ');
        std::istringstream viss(parts[i]);

        int vIdx = -1, vtIdx = -1, vnIdx = -1;
        viss >> vIdx >> vtIdx >> vnIdx;

        // Заполняем индексы вершин
        if (i == 0) {
            face.v.x = vIdx > 0 ? vIdx - 1 : 0;
            face.vn.x = vnIdx > 0 ? vnIdx - 1 : 0;
        }
        if (i == 1) {
            face.v.y = vIdx > 0 ? vIdx - 1 : 0;
            face.vn.y = vnIdx > 0 ? vnIdx - 1 : 0;
        }
        if (i == 2) {
            face.v.z = vIdx > 0 ? vIdx - 1 : 0;
            face.vn.z = vnIdx > 0 ? vnIdx - 1 : 0;
        }
    }
    return face;
}

bool loadOBJ(const std::string& path,
    std::vector<vec3d>& vertices,
    std::vector<vec3d>& normals,
    std::vector<polygon>& faces) {

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "lol, file cannot be opened " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        // Парсинг вершин
        if (type == "v") {
            vec3d v;
            iss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        // Парсинг нормалей
        else if (type == "vn") {
            vec3d n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        // Парсинг полигонов
        else if (type == "f") {
            std::vector<std::string> parts;  // Храним части типа "1//2", "3/4/5" и т.д.

            // Извлекаем все элементы полигона
            std::string part;
            while (iss >> part) {
                parts.push_back(part);
            }

            // если 4 вершины в полигоне, делим его на 2 полигона
            if (parts.size() == 4) {
                std::vector<std::string> parts1 = { parts[0], parts[1], parts[2] };
                std::vector<std::string> parts2 = { parts[0], parts[2], parts[3] };
                polygon face1 = writePolygon(parts1);
                polygon face2 = writePolygon(parts2);
                faces.push_back(face1);
                faces.push_back(face2);
            }

            // если 3 вершины в полигоне, читаем их (для 5 пока не написано)
            else {
                polygon face = writePolygon(parts);
                faces.push_back(face);
            }
        }
    }

    file.close();
    return true;
}