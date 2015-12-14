//#define BOOST_TEST_MODULE collTest
#include <boost/test/unit_test.hpp>
#include <boost/test/execution_monitor.hpp>
using boost::unit_test_framework::test_suite;
using boost::unit_test_framework::test_case;
#include "Engine\Core\Collision.h"
#include "Engine/Core/AABB.h"
#include "Engine/Core/Ray.h"
#include <stdlib.h>//srand
#include "Engine/Core/OctTree.h"
//vs model
#include <sstream>

//ray vs model
#include "Engine\Core\ResourceManager.h"
#include "Engine\Rendering\Model.h"
#include "Engine\Core\Ray.h"

//vs memleaks
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
//#define DEBUG_CLIENTBLOCK new( _CLIENT_BLOCK, __FILE__, __LINE__)
//#define new DEBUG_CLIENTBLOCK

BOOST_AUTO_TEST_SUITE(collisionTests)

BOOST_AUTO_TEST_CASE(collisionTest)
{
    //memleak
    int* globalLeak = new int[5];

    //fixed seed
    srand(2);
    Ray ray;
    AABB someAABB;
    glm::vec3 minPos;
    glm::vec3 maxPos;
    bool z;
    int test = 0;
    for (size_t i = 0; i < 10; i++)
    {
        ray.Origin.x = rand() % 100;
        ray.Origin.y = rand() % 100;
        ray.Origin.z = rand() % 100;
        ray.Direction.x = rand() % 100;
        ray.Direction.y = rand() % 100;
        ray.Direction.z = rand() % 100;
        minPos.x = rand() % 100;
        minPos.y = rand() % 100;
        minPos.z = rand() % 100;
        maxPos.x = rand() % 100;
        maxPos.y = rand() % 100;
        maxPos.z = rand() % 100;

        someAABB = AABB(minPos, maxPos);
        z = Collision::RayVsAABB(ray, someAABB);
        if (z) ++test;
    }
    BOOST_CHECK(test >= 0);

    //_CrtDumpMemoryLeaks();
}

BOOST_AUTO_TEST_CASE(collisionTest2)
{
    //fixed seed
    srand(2);
    Ray ray;
    AABB someAABB;
    glm::vec3 minPos;
    glm::vec3 maxPos;
    bool z;
    int test = 0;
    for (size_t i = 0; i < 1000000; i++)
    {
        ray.Origin.x = rand() % 100;
        ray.Origin.y = rand() % 100;
        ray.Origin.z = rand() % 100;
        ray.Direction.x = rand() % 100;
        ray.Direction.y = rand() % 100;
        ray.Direction.z = rand() % 100;
        minPos.x = rand() % 100;
        minPos.y = rand() % 100;
        minPos.z = rand() % 100;
        maxPos.x = rand() % 100;
        maxPos.y = rand() % 100;
        maxPos.z = rand() % 100;

        someAABB = AABB(minPos, maxPos);
        z = Collision::RayAABBIntr(ray, someAABB);
        if (z) ++test;
    }
    BOOST_CHECK(test >= 0);
}

BOOST_AUTO_TEST_CASE(rayVsModelTest)
{
    //simple test

    Ray ray;
    ray.Origin = glm::vec3(-50, 0, 0);
    //ray.Direction = glm::vec3(-1, 0, 0);
    ray.Direction = glm::normalize(glm::vec3(1, 0, 0));
    //inte model, det kr�ver renderar grejs tydligen
    ResourceManager::RegisterType<RawModel>("RawModel");
    auto unitBox = ResourceManager::Load<RawModel>("Models/Core/UnitBox.obj");
    BOOST_CHECK(unitBox != nullptr);
    bool hit = Collision::RayVsModel(ray, unitBox->m_Vertices, unitBox->m_Indices);
    BOOST_CHECK(hit);
    ray.Direction = glm::normalize(glm::vec3(-1, 0, 0));
    hit = Collision::RayVsModel(ray, unitBox->m_Vertices, unitBox->m_Indices);
    BOOST_CHECK(!hit);
}

BOOST_AUTO_TEST_CASE(rayVsModelTest2)
{
    //advanced test, based on ray vs AABB
    srand(7676762);
    Ray ray;
    AABB someAABB;
    glm::vec3 minPos;
    glm::vec3 maxPos;
    bool z;
    int test = 0;
    minPos = glm::vec3(-0.5f, -0.5f, -0.5f);
    maxPos = glm::vec3(0.5f, 0.5f, 0.5f);
    someAABB = AABB(minPos, maxPos);
    ResourceManager::RegisterType<RawModel>("RawModel");
    //auto unitBox = ResourceManager::Load<RawModel>("Models/Core/UnitBox.obj");
    auto unitBox = ResourceManager::Load<RawModel>("Models/Core/UnitCube.obj");
    BOOST_CHECK(unitBox != nullptr);

    for (size_t i = 0; i < 1000000; i++)
    {
        ray.Origin.x = rand() % 100;
        ray.Origin.y = rand() % 100;
        ray.Origin.z = rand() % 100;
        ray.Direction.x = rand() % 100;
        ray.Direction.y = rand() % 100;
        ray.Direction.z = rand() % 100;
        ray.Origin /= 100;
        ray.Origin = glm::vec3(-2, 0, 0);
        ray.Direction /= 100;
        ray.Direction = glm::normalize(ray.Direction);

        z = Collision::RayVsAABB(ray, someAABB);
        if (z) {
            //hit
            bool hit = Collision::RayVsModel(ray, unitBox->m_Vertices, unitBox->m_Indices);
            if (!hit) {
                hit = hit;
                glm::vec3 outtttttttt;
                hit = Collision::RayVsModel(ray, unitBox->m_Vertices, unitBox->m_Indices, outtttttttt);
                hit = Collision::RayVsModel(ray, unitBox->m_Vertices, unitBox->m_Indices);
            }
            else {
                hit = hit;
            }
            BOOST_CHECK(hit);
        }
        //
        bool hit = Collision::RayVsModel(ray, unitBox->m_Vertices, unitBox->m_Indices);
        if (hit) {
            //hit
            z = Collision::RayVsAABB(ray, someAABB);
            if (!z) {
                z = z;
                z = Collision::RayVsAABB(ray, someAABB);
                glm::vec3 outtttttttt;
                hit = Collision::RayVsModel(ray, unitBox->m_Vertices, unitBox->m_Indices, outtttttttt);
                hit = Collision::RayVsModel(ray, unitBox->m_Vertices, unitBox->m_Indices);
            }
            else {
                z = z;
            }
            BOOST_CHECK(hit);
        }

    }
}


BOOST_AUTO_TEST_CASE(octTest)
{
    glm::vec3 mini = glm::vec3(-1, -1, -1);
    glm::vec3 maxi = glm::vec3(1, 1, 1);
    OctTree tree(AABB(mini, maxi), 2);
    tree.AddDynamicObject(AABB(mini, -0.9f*maxi));
    OctTree::Output data;
    glm::vec3 origin = 3.0f * mini;
    bool rayIntersected = tree.RayCollides({ origin , glm::normalize(mini - origin) }, data);
    BOOST_CHECK(rayIntersected);
    tree.ClearDynamicObjects();
    rayIntersected = tree.RayCollides({ origin , glm::normalize(mini - origin) }, data);
    BOOST_CHECK(!rayIntersected);
}

BOOST_AUTO_TEST_SUITE_END()

