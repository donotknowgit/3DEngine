#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include <OBJparser.h>
#include <random>

using namespace std;



constexpr int width = 1600;
constexpr int height = 800;
constexpr int centerX = width / 2;
constexpr int centerY = height / 2;



// random gen
float getRandom(float rangeMin, float rangeMax) {
    std::default_random_engine gen(std::random_device{}());
    std::uniform_real_distribution<> r;
    return r(gen, std::uniform_real_distribution<>::param_type(rangeMin, rangeMax));
}

sf::Color getRandomColor(sf::Color c1, sf::Color c2) {
    float r = getRandom(1, 100);
    return r > 50 ? c1 : c2;
}

float rad(float deg) {
    return deg * pi / 180.0f;
}

float dot(vec3d a, vec3d b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3d vecProd(vec3d a, vec3d b) {
    vec3d ans;
    ans.x = a.y * b.z - a.z * b.y;
    ans.y = a.z * b.x - a.x * b.z;
    ans.z = a.x * b.y - a.y * b.z;
    return ans;
}

float cosVecAngle(vec3d& a, vec3d& b) { // get cos of the angle between 2 vectors
    float lenA = a.normEuc();
    float lenB = b.normEuc();

    float cosTheta = dot(a, b) / (lenA * lenB);

    // limit cosTheta vals
    cosTheta = max(-1.0f, min(1.0f, cosTheta));

    return cosTheta;
}

class Camera {
public:
    vec3d pos;          // pos in global coords
    vec3d front;        // local Z
    vec3d right;        // local X
    vec3d up;           // local Y
    float yaw, pitch;   // rotation angles

    Camera(vec3d _pos = { 3, 3, 3 }) :
        pos(_pos), front({ 0, 0, -1 }), right({ 1, 0, 0 }), up({ 0, 1, 0 }),
        yaw(0), pitch(0) {}

    void updateVectors() {
        front.x = cos(rad(yaw)) * cos(rad(pitch));
        front.y = sin(rad(pitch));
        front.z = sin(rad(yaw)) * cos(rad(pitch));
        front = front.normalize();

        right = vecProd(front, { 0, 1, 0 }).normalize();
        up = vecProd(right, front).normalize();
    }
};

vec3d applyCamera(vec3d point, Camera& cam) {
    // get translate point to cam coords
    vec3d translated = point - cam.pos;

    // projection on axises
    vec3d result;
    result.x = dot(translated, cam.right);
    result.y = dot(translated, cam.up);
    result.z = dot(translated, cam.front * (-1));

    return result;
}

class obj {
public:
    vector<vec3d> verts;        // vertices
    vector<vec3d> norms;        // normals
    vector<polygon> polys;      // polygons
    vec3d pos;                  // center pos
    vec3d front;                // local Z
    vec3d right;                // local X
    vec3d up;                   // local Y
    float mass;                 // mass
    vec3d vel;                  // velocity
    vec3d acc;                  // acceleration
    vec3d angVel;               // angular velocity
    vec3d angAcc;               // angular acceleration
    float scale;                // scale multiplier

    obj(vector<vec3d> _verts, vector<vec3d> _norms, vector<polygon> _polys, float _mass = 0, float _scale = 1) :
        verts(_verts), norms(_norms), polys(_polys), mass(_mass), scale(_scale), front({ 0, 0, -1 }), right({ 1, 0, 0 }), up({ 0, 1, 0 }) {
        setupPos(); setupScale();
    }

    void setupPos() {
        float c = 0.0f;
        for (auto& v : verts) {
            pos.x += v.x;
            pos.y += v.y;
            pos.z += v.z;
            c++;
        }
        pos.x /= c;
        pos.y /= c;
        pos.z /= c;
    }

    void setupScale() {
        pos = pos * scale;
        for (auto& v : verts) {
            v.x *= scale;
            v.y *= scale;
            v.z *= scale;
        }
    }

    void setPos(float x, float y, float z) {
        vec3d posOld = pos;
        for (auto& v : verts) {
            pos.x = x;
            pos.y = y;
            pos.z = z;
            v = v + pos - posOld;
        }
    }

    // move object (ignoring normals cause why should not we)
    void moveForward(float a) {
        pos = pos - front * a;
        for (auto& v : verts) {
            v = v - front * a;
        }
    }
    void moveBackward(float a) {
        pos = pos + front * a;
        for (auto& v : verts) {
            v = v + front * a;
        }
    }
    void moveRight(float a) {
        pos = pos + right * a;
        for (auto& v : verts) {
            v = v + right * a;
        }
    }
    void moveLeft(float a) {
        pos = pos - right * a;
        for (auto& v : verts) {
            v = v - right * a;
        }
    }
    void moveUp(float a) {
        pos = pos + up * a;
        for (auto& v : verts) {
            v = v + up * a;
        }
    }
    void moveDown(float a) {
        pos = pos - up * a;
        for (auto& v : verts) {
            v = v - up * a;
        }
    }

    // GLOBAL
    void moveUpGlobal(float a) {
        pos.y += a;
        for (auto& v : verts) {
            v.y = v.y + a;
        }
    }
    void moveDownGlobal(float a) {
        pos.y -= a;
        for (auto& v : verts) {
            v.y = v.y - a;
        }
    }

    void movecustom(vec3d& vec, float a) {
        pos = pos - vec * a;
        for (auto& v : verts) {
            v = v - vec * a;
        }
    }

    void rotate(vec3d ang) { // rotate object
        vec3d center = pos; // object center

        // rotate around center
        for (auto& v : verts) {
            // get local coords of v
            vec3d local = v - center;

            // apply rotation
            local = local.rotateVector(ang);

            // get global coords of rotated v
            v = center + local;
        }

        // rotate normals
        for (auto& n : norms) {
            n = n.rotateVector(ang);
        }

        // update local axises
        front = front.rotateVector(ang).normalize();
        right = right.rotateVector(ang).normalize();
        up = up.rotateVector(ang).normalize();
    }

    void rotateAroundLocalFront(float angle) {
        vec3d ang = front * -angle;

        vec3d center = pos; // object center

        // rotate around center
        for (auto& v : verts) {
            // get local coords of v
            vec3d local = v - center;

            // apply rotation
            local = local.rotateVector(ang);

            // get global coords of rotated v
            v = center + local;
        }

        // rotate normals
        for (auto& n : norms) {
            n = n.rotateVector(ang);
        }

        // update local axises
        front = front.rotateVector(ang).normalize();
        right = right.rotateVector(ang).normalize();
        up = up.rotateVector(ang).normalize();
    }

    void rotateCustom(vec3d ang, vec3d point) {
        vec3d center = point;

        // rotate around point
        for (auto& v : verts) {
            // get local coords of v
            vec3d local = v - center;

            // apply rotation
            local = local.rotateVector(ang);

            // get global coords of rotated v
            v = center + local;
        }

        // rotate normals
        for (auto& n : norms) {
            n = n.rotateVector(ang);
        }

        // update local axises
        front = front.rotateVector(ang).normalize();
        right = right.rotateVector(ang).normalize();
        up = up.rotateVector(ang).normalize();
    }
    //void draw(sf::RenderWindow& w, Camera& cam, vector<sf::Color> colors);
};

class light {
public:
    vec3d pos; // center pos
    vec3d front; // local Z
    vec3d right; // local X
    vec3d up; // local Y

    light(vec3d _pos) :
        pos(_pos), front({ 0, 0, -1 }), right({ 1, 0, 0 }), up({ 0, 1, 0 }) {
    }

    void setPos(float x, float y, float z) {
        vec3d p = { x, y, z };
        pos = p;
    }

    // move object
    void moveForward(float a) {
        pos = pos - front * a;
    }
    void moveBackward(float a) {
        pos = pos + front * a;
    }
    void moveRight(float a) {
        pos = pos + right * a;
    }
    void moveLeft(float a) {
        pos = pos - right * a;
    }
    void moveUp(float a) {
        pos = pos + up * a;
    }
    void moveDown(float a) {
        pos = pos - up * a;
    }
};

void draw(sf::RenderWindow& w, obj& o, Camera& cam, light& sun, vector<sf::Color> colors) {
    std::vector<sf::Vector2f> projections;
    std::vector<float> depths; // vector for sorting

    // verts and norms translation
    for (size_t i = 0; i < o.verts.size(); ++i) {
        // apply cam transformation to verts
        vec3d v = o.verts[i];

        vec3d transformed = applyCamera(v, cam);

        // perspective proj
        if (transformed.z > 0) {
            float depth = 1.0f / transformed.z;
            projections.emplace_back(
                transformed.x * depth * 200 + centerX,
                -transformed.y * depth * 200 + centerY
            );
            depths.push_back(transformed.z);
        }
        else {
            projections.emplace_back(-1000, -1000);
            depths.push_back(FLT_MAX);
        }
    }

    // get all polys to the pairs with idxes
    vector<pair<float, size_t>> sortedPolys;
    for (size_t i = 0; i < o.polys.size(); i++) {
        auto& p = o.polys[i];
        p.color = colors[i];
        sortedPolys.emplace_back((depths[p(0)] + depths[p(1)] + depths[p(2)]) / 3.0f, i);
    }

    // Z-sorting
    std::sort(sortedPolys.begin(), sortedPolys.end(),
        [](auto& a, auto& b) { return a.first > b.first; });

    // drawing polys
    for (const auto& [depth, i] : sortedPolys) {
        auto& p = o.polys[i];

        // if polygon is behind cam
        if (projections[p(0)].x < -999 ||
            projections[p(1)].x < -999 ||
            projections[p(2)].x < -999) continue;

        // normal to the current polygon
        vec3d normal = o.norms[p.vn.x].normalize();

        // vector from poly to cam
        vec3d viewDir = (cam.pos - (o.verts[p(0)] + o.verts[p(1)] + o.verts[p(2)]) / 3).normalize();
        vec3d lightDir = (sun.pos - (o.verts[p(0)] + o.verts[p(1)] + o.verts[p(2)]) / 3).normalize();

        // checking vesebelety through normal
        if (dot(normal, viewDir) >= 0.0f) {
            sf::ConvexShape triangle(3);
            triangle.setPoint(0, projections[p(0)]);
            triangle.setPoint(1, projections[p(1)]);
            triangle.setPoint(2, projections[p(2)]);

            if (dot(normal, lightDir) < 0.0f) triangle.setFillColor(sf::Color(0, 0, 0));
            else triangle.setFillColor(sf::Color(p.color.r * cosVecAngle(normal, lightDir), p.color.g * cosVecAngle(normal, lightDir), p.color.b * cosVecAngle(normal, lightDir)));
            //triangle.setOutlineColor(sf::Color(20, 255, 0));
            //triangle.setOutlineThickness(0.5);
            w.draw(triangle);
        }
    }
}

void drawScene(const std::vector<obj>& objects, sf::RenderWindow& w, Camera& cam, light& sun, const std::vector<sf::Color>& colors) {
    if (objects.empty()) return;
    // counting all verts, norms, polys
    size_t totalVerts = 0;
    size_t totalNorms = 0;
    size_t totalPolys = 0;
    for (const auto& o : objects) {
        totalVerts += o.verts.size();
        totalNorms += o.norms.size();
        totalPolys += o.polys.size();
    }

    // reserving mem
    std::vector<vec3d> V;
    std::vector<vec3d> N;
    std::vector<polygon> P;
    std::vector<sf::Color> C;
    V.reserve(totalVerts);
    N.reserve(totalNorms);
    P.reserve(totalPolys);
    C.reserve(totalPolys);

    // counters
    int prevVertsCount = 0;
    int prevNormsCount = 0;
    size_t globalPolyIdx = 0; // global polygon idx

    for (const auto& o : objects) {
        // collecting verts and norms all together
        V.insert(V.end(), o.verts.begin(), o.verts.end());
        N.insert(N.end(), o.norms.begin(), o.norms.end());

        // drawing polys of the current obj
        for (size_t localPolyIdx = 0; localPolyIdx < o.polys.size(); localPolyIdx++) {
            const auto& p = o.polys[localPolyIdx];
            P.emplace_back(
                p.v.x + prevVertsCount,
                p.v.y + prevVertsCount,
                p.v.z + prevVertsCount,
                p.vn.x + prevNormsCount,
                p.vn.y + prevNormsCount,
                p.vn.z + prevNormsCount
            );

            // accessing color for current poly via global idx
            C.push_back(colors[globalPolyIdx]);
            globalPolyIdx++;
        }

        // counters++
        prevVertsCount += o.verts.size();
        prevNormsCount += o.norms.size();
    }

    // drawing scene
    obj scene(V, N, P);
    draw(w, scene, cam, sun, C);
    //scene.draw(w, cam, C);
}

int main() {
    sf::RenderWindow window(sf::VideoMode(width, height), "UE 6");
    window.setFramerateLimit(144);

    vector<vec3d> vAxe;
    vector<vec3d> nAxe;
    vector<polygon> pAxe;

    vector<vec3d> vRat;
    vector<vec3d> nRat;
    vector<polygon> pRat;

    vector<vec3d> vCube;
    vector<vec3d> nCube;
    vector<polygon> pCube;

    loadOBJ("Axe.obj", vAxe, nAxe, pAxe);
    loadOBJ("Rat.obj", vRat, nRat, pRat);
    loadOBJ("cube.obj", vCube, nCube, pCube);

    obj axe(vAxe, nAxe, pAxe, 0, 2);
    obj rat(vRat, nRat, pRat, 0, 100);
    obj cube(vCube, nCube, pCube, 0, 2);

    // cam
    Camera cam{ {50, 100, 50} };

    // LIGHT
    light LIGHT({ 50, 100, 50 });

    sf::Vector2i lastMousePos = { 0, 0 };


    float speed = 3.0f;
    float ascSpeed = 4.0f;
    float rotSpeed = 0.03f;
    float sensitivity = 0.5f;

    sf::Color color0(255, 0, 208);
    sf::Color color1(90, 90, 90);
    sf::Color cubeColor(20, 255, 0);
    float x = 0.0f;


    sf::Mouse::setPosition({ 0, 0 });


    axe.setPos(0, 100, -70);

    cube.setPos(50, 100, 50);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        x += 0.05f;

        //axe.moveForward(sin(x));
        //axe.moveUp(cos(x));
        //axe.moveRight((sin(x) + cos(x)) / 2);
        //axe.moveForward(0.3 * cos(x));
        axe.rotate({ 0.05f * cos(x - 1.0f), 0, 0 });

        color0.r += 1 * speed;
        color0.g = 2 * color0.r;
        color0.b = 3 * color0.g;

        vector<sf::Color> cAxe;
        vector<sf::Color> cRat;
        vector<sf::Color> cCube;

        vector<sf::Color> colors;


        for (int i = 0; i < pAxe.size(); i++) {
            cAxe.push_back(color0);
            colors.push_back(color0);
        }
        for (int i = 0; i < pRat.size(); i++) {
            cRat.push_back(color1);
            colors.push_back(color1);
        }
        for (int i = 0; i < pCube.size(); i++) {
            cCube.push_back(cubeColor);
            colors.push_back(cubeColor);
        }


        // updating 1
        cam.updateVectors();

        // cam movement
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
            cam.pos = cam.pos - cam.front * speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
            cam.pos = cam.pos + cam.front * speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            cam.pos = cam.pos - cam.right * speed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            cam.pos = cam.pos + cam.right * speed;

        vec3d globalUp(0, 1, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            cam.pos = cam.pos + globalUp * 3 * ascSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
            cam.pos = cam.pos - globalUp * 3 * ascSpeed;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) speed = 9.0f;
        else speed = 3.0f;

        // LIGHT movement
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            LIGHT.pos = LIGHT.pos - cam.front * speed;
            cube.movecustom(cam.front, speed);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            LIGHT.pos = LIGHT.pos + cam.front * speed;
            vec3d antifront = cam.front * (-1);
            cube.movecustom(antifront, speed);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            LIGHT.pos = LIGHT.pos - cam.right * speed;
            cube.movecustom(cam.right, speed);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            LIGHT.pos = LIGHT.pos + cam.right * speed;
            vec3d antiright = cam.right * (-1);
            cube.movecustom(antiright, speed);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
            LIGHT.pos = LIGHT.pos + globalUp * 3 * ascSpeed;
            cube.moveUpGlobal(3 * ascSpeed);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
            LIGHT.pos = LIGHT.pos - globalUp * 3 * ascSpeed;
            cube.moveDownGlobal(3 * ascSpeed);
        }

        // cam rotation
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2i mouseDelta = sf::Mouse::getPosition(window) - lastMousePos;

        cam.yaw -= mouseDelta.x * sensitivity;
        cam.pitch += mouseDelta.y * sensitivity;
        cam.pitch = std::clamp(cam.pitch, -89.0f, 89.0f);

        cube.rotate({ 0, -mouseDelta.x * sensitivity * pi / 180, 0 });
        //cube.rotateAroundLocalFront(mouseDelta.y * sensitivity * pi / 180);

        // update after rotation
        cam.updateVectors();

        lastMousePos = mousePos;
        window.clear(sf::Color::Yellow);

        vector<obj> OBJS = { axe, rat, cube };

        drawScene(OBJS, window, cam, LIGHT, colors);

        //vec3d ang(0.0, 0.1, 0.0);

        //axe.rotate(ang.rad());
        //axe.draw(window, cam, cAxe);
        //rat.draw(window, cam, sf::Color(255, 255, 255));

        window.display();
    }
    return 0;
}