// Asteroids.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <string>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <cmath>

#include "olcConsoleGameEngine.h"

class AsteroidGame : public olcConsoleGameEngine
{
public:
    AsteroidGame()
    {
        m_sAppName = L"Asteroid Game";
    }

private:
    struct SpaceObject
    {
        float x;
        float y;
        float dirx;
        float diry;
        int size;
        float angle;
    };

    std::vector<SpaceObject> vecAsteroids;
    
    std::vector<SpaceObject> vecBullets;
    SpaceObject player;
    bool isDead = false;
    int score = 0;

    // std::vector stores the points as an array, std::pair stores a 2D vector point pair to reference a point in the 2D space
    std::vector<std::pair<float, float>> vecModelShip;
    std::vector<std::pair<float, float>> vecModelAsteroid;

protected:
    virtual bool OnUserCreate() override
    {
        // Player ship model vertices stored here
        vecModelShip =
        {
            { 0.0f, -5.0f },
            { -2.5, +2.5f },
            { +2.5f, +2.5f }
        }; 

        // Iterate through asteroid vertices and assign them in a circle formation with randomised points to create a jagged look
        int vertices = 20;
        for (int i = 0; i < vertices; i++)
        {
            float radius = (float)rand() / (float)RAND_MAX * 0.4f + 0.8f;
            float point = ((float)i / (float)vertices * (2 * M_PI));
            vecModelAsteroid.push_back(std::make_pair(radius * sinf(point), radius * cosf(point)));
        }

        ResetGame();

        return true;
    }

    void ResetGame()
    {
        vecAsteroids.clear();
        vecBullets.clear();
        
        // Instaniate asteroid using the spawn method I wrote to replace the chunks of spawning code
        SpawnAsteroid(2);

        // Instantiate player
        player.x = ScreenWidth() / 2.0f;
        player.y = ScreenHeight() / 2.0f;
        player.dirx = 0.0f;
        player.diry = 0.0f;
        player.angle = 0.0f;

        isDead = false;
        score = 0;
    }

    virtual bool OnUserUpdate(float fElapsedTime) override
    {
        if (isDead)
        {
            ResetGame();
        }

        // Clear the screen
        Fill(0, 0, ScreenWidth(), ScreenHeight(), PIXEL_SOLID, BG_BLACK);

        // Player steering
        if (m_keys[VK_LEFT].bHeld)
            player.angle -= 5.0f * fElapsedTime;
        if (m_keys[VK_RIGHT].bHeld)
            player.angle += 5.0f * fElapsedTime;

        // Player thrust
        if (m_keys[VK_UP].bHeld)
        {
            // Acceleration changes velocity (maths)
            player.dirx += sin(player.angle) * 20.0f * fElapsedTime;
            player.diry -= cos(player.angle) * 20.0f * fElapsedTime;
        }

        // Spawn bullets on the player's location, and get the sin and cos of the player's x and y coordinates and force the bullet forward on that vector
        if (m_keys[VK_SPACE].bReleased)
        {
            vecBullets.push_back({ player.x, player.y, 50.0f * sinf(player.angle), -50.0f * cosf(player.angle), 0, 0.0f });
        }
        
        // Velocity changes position
        player.x += player.dirx * fElapsedTime;
        player.y += player.diry * fElapsedTime;

        WrapCoordinates(player.x, player.y, player.x, player.y);

        // Draw player
        DrawWireFrameModel(vecModelShip, player.x, player.y, player.angle);

        // Draw score
        DrawString(2, 2, L"SCORE:" + std::to_wstring(score));

        for (auto& asteroid : vecAsteroids)
        {
            if (CheckOverlap(asteroid.x, asteroid.y, asteroid.size, player.x, player.y))
            {
                isDead = true;
            }
        }

        // Update and draw asteroids
        for (auto& asteroid : vecAsteroids)
        {
            asteroid.x += asteroid.dirx * fElapsedTime;
            asteroid.y += asteroid.diry * fElapsedTime;
            asteroid.angle += 0.5f * fElapsedTime;

            WrapCoordinates(asteroid.x, asteroid.y, asteroid.x, asteroid.y);

            DrawWireFrameModel(vecModelAsteroid, asteroid.x, asteroid.y, asteroid.angle, (float)asteroid.size, FG_RED);
        }

        // A temporary vector to hold new asteroids that spawn after one is destroyed
        std::vector<SpaceObject> newVecAsteroids;

        // Update and draw bullets
        for (auto& bullet : vecBullets)
        {
            bullet.x += bullet.dirx * fElapsedTime;
            bullet.y += bullet.diry * fElapsedTime;
            WrapCoordinates(bullet.x, bullet.y, bullet.x, bullet.y);

            Draw(bullet.x, bullet.y);

            // Check for asteroid collision
            for (auto& asteroid : vecAsteroids)
            {
                if (CheckOverlap(asteroid.x, asteroid.y, asteroid.size, bullet.x, bullet.y))
                {
                    bullet.x = -100.0f;

                    if (asteroid.size > 4)
                    {
                        float firstAngleOfImpact = ((float)rand() / (float)RAND_MAX) * (2 * M_PI);
                        float secondAngleOfImpact = ((float)rand() / (float)RAND_MAX) * (2 * M_PI);

                        newVecAsteroids.push_back({ asteroid.x, asteroid.y, 10.0f * sinf(firstAngleOfImpact), 10.0f * cosf(firstAngleOfImpact), (int)asteroid.size >> 1, 0.0f });
                        newVecAsteroids.push_back({ asteroid.x, asteroid.y, 10.0f * sinf(secondAngleOfImpact), 10.0f * cosf(secondAngleOfImpact), (int)asteroid.size >> 1, 0.0f });
                    }

                    // Send the asteroid to die and increase score by 100
                    asteroid.x = -1;
                    score += 100;
                }
            }
        }

        /* Push the newly created asteroids from the newVecAsteroids vector into the original vecAsteroids vector. 
           The reason we do this is because the game is iterating through the original vecAsteroids vector in the for loop above. Making changes inside the 
           iterating for loop above would crash the game. So we store them temporarily in newVecAsteroids, then once the iterating is done, we push the new 
           asteroids from newVecAsteroids into the original vecAsteroids vector */
        
        for (auto& newAsteroid : newVecAsteroids)
        {
            vecAsteroids.push_back(newAsteroid);
        }

        // Destroy bullets if they fly offscreen 
        if (vecBullets.size() > 0)
        {
            /* std::remove_if doesn't actually remove anything. It sorts the vector in a way that anything that fails the "if criteria" is put at the end of the vector */
            
            auto i = std::remove_if(vecBullets.begin(), vecBullets.end(), /* this is how we want it sorted */
                [&](SpaceObject obj) {return (obj.x < 1 || obj.y < 1 || obj.x >= ScreenWidth() - 1 || obj.y >= ScreenHeight() - 1); }); /* This is the "if criteria". Any object that isn't offscreen
                                                                                                                                        is put at the end of the vector. This makes use of the lambda function
                                                                                                                                        "[&]" */
            /* Any bullets that go offscreen will be put at the start of the vector. 
            So long as the iterator is not at the end of the vector (where the onscreen bullets are), 
            delete the bullets from the vector (the offscreen ones) */
            if (i != vecBullets.end())
            {
                vecBullets.erase(i);
            }
        }


        // Remove asteroid from the game
        if (vecAsteroids.size() > 0)
        {
            auto i = std::remove_if(vecAsteroids.begin(), vecAsteroids.end(),
                [&](SpaceObject obj) {return (obj.x == -1); });

            if (i != vecAsteroids.end())
            {
                vecAsteroids.erase(i);
            }
        }

        // Spawn more asteroids if the player destroys the current ones
        if (vecAsteroids.empty())
        {
            score += 1000;           

            if (score <= 5000)
                SpawnAsteroid(2);

            if (score > 5000 && score < 10000)
                SpawnAsteroid(3);

            if (score > 10000)
                SpawnAsteroid(4);
        }
              
        /* CRT TV Static
        for (int x = 0; x < ScreenWidth(); x++)
            for (int y = 0; y < ScreenHeight(); y++)
                Draw(x, y, olc::Pixel(rand() % 255, rand() % 255, rand() % 255, rand() % 255));*/

        return true;
    }

    void DrawWireFrameModel(const std::vector<std::pair<float, float>>& vecModelCoordinates, float x, float y, float angle = 0.0f, float scale = 1.0f, short col = FG_WHITE, short c = PIXEL_SOLID)
    {
        // pair.first = x coordinate
        // pair.second = y coordinate

        // Create translated model vector of coordinate pairs
        std::vector<std::pair<float, float>> vecTransformedCoordinates;
        int verts = vecModelCoordinates.size();
        vecTransformedCoordinates.resize(verts);

        // Rotate
        for (int i = 0; i < verts; i++)
        {
            vecTransformedCoordinates[i].first = vecModelCoordinates[i].first * cosf(angle) - vecModelCoordinates[i].second * sinf(angle);
            vecTransformedCoordinates[i].second = vecModelCoordinates[i].first * sinf(angle) + vecModelCoordinates[i].second * cosf(angle);
        }

        // Scale
        for (int i = 0; i < verts; i++)
        {
            vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first * scale;
            vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second * scale;
        }

        // Translate
        for (int i = 0; i < verts; i++)
        {
            vecTransformedCoordinates[i].first = vecTransformedCoordinates[i].first + x;
            vecTransformedCoordinates[i].second = vecTransformedCoordinates[i].second + y;
        }

        // Draw Closed Polygon
        for (int i = 0; i < verts + 1; i++)
        {
            int j = (i + 1);
            DrawLine((int)vecTransformedCoordinates[i % verts].first, (int)vecTransformedCoordinates[i % verts].second,
                (int)vecTransformedCoordinates[j % verts].first, (int)vecTransformedCoordinates[j % verts].second, c, col);
        }
    }

    void WrapCoordinates(float intake_x, float intake_y, float &output_x, float &output_y)
    {
        output_x = intake_x;
        output_y = intake_y;

        if (intake_x < 0.0f) 
            output_x = intake_x + (float)ScreenWidth();

        if (intake_x >= (float)ScreenWidth())
            output_x = intake_x - (float)(ScreenWidth());

        if (intake_y < 0.0f)
            output_y = intake_y + (float)ScreenHeight();

        if (intake_y >= (float)ScreenHeight())
            output_y = intake_y - (float)(ScreenHeight());
    }

    // Overridden from the oldConsoleGameEngine header file to provide the wrapping function
    virtual void Draw(int x, int y, short c = 0x2588, short col = 0x000F)
    {
        float wrapX, wrapY;
        WrapCoordinates(x, y, wrapX, wrapY);

        // Call the original draw function with the new wrapped coordinates
        olcConsoleGameEngine::Draw(wrapX, wrapY, c, col);
    }

    bool CheckOverlap(float circleX, float circleY, float radius, float x, float y)
    {
        return sqrt((x - circleX) * (x - circleX) + (y - circleY) * (y - circleY)) < radius;
    }

    // Number to spawn should be at least 2 or greater
    void SpawnAsteroid(int numberToSpawn)
    {
        float sinOffset = RandFloatClamped(30.0f, 40.0f);
        float cosOffset = RandFloatClamped(30.0f, 40.0f);

        switch (numberToSpawn)
        {
        case 2:
            vecAsteroids.push_back({ sinOffset * sinf(player.angle - M_PI / 2.0f),
                                     cosOffset * cosf(player.angle - M_PI / 2.0f),
                                     10.0f * sinf(player.angle),
                                     10.0f * cosf(player.angle),
                                     (int)16, 0.0f });

            vecAsteroids.push_back({ sinOffset * sinf(player.angle + M_PI / 2.0f),
                                     cosOffset * cosf(player.angle + M_PI / 2.0f),
                                     10.0f * sinf(-player.angle),
                                     10.0f * cosf(-player.angle),
                                     (int)16, 0.0f });
            break;

        case 3:
            vecAsteroids.push_back({ sinOffset * sinf(player.angle - M_PI / 2.0f),
                                     cosOffset * cosf(player.angle - M_PI / 2.0f),
                                     10.0f * sinf(player.angle),
                                     10.0f * cosf(player.angle),
                                     (int)16, 0.0f });

            vecAsteroids.push_back({ sinOffset * sinf(player.angle + M_PI / 2.0f),
                                     cosOffset * cosf(player.angle + M_PI / 2.0f),
                                     10.0f * sinf(-player.angle),
                                     10.0f * cosf(-player.angle),
                                     (int)16, 0.0f });

            vecAsteroids.push_back({ sinOffset * sinf(player.angle - M_PI / 4.0f),
                                     cosOffset * cosf(player.angle + M_PI / 4.0f),
                                     10.0f * sinf(player.angle),
                                     10.0f * cosf(player.angle),
                                     (int)16, 0.0f });
            break;

        case 4:
            vecAsteroids.push_back({ sinOffset * sinf(player.angle - M_PI / 2.0f),
                                     cosOffset * cosf(player.angle - M_PI / 2.0f),
                                     10.0f * sinf(player.angle),
                                     10.0f * cosf(player.angle),
                                     (int)16, 0.0f });

            vecAsteroids.push_back({ sinOffset * sinf(player.angle + M_PI / 2.0f),
                                     cosOffset * cosf(player.angle + M_PI / 2.0f),
                                     10.0f * sinf(-player.angle),
                                     10.0f * cosf(-player.angle),
                                     (int)16, 0.0f });

            vecAsteroids.push_back({ sinOffset * sinf(player.angle - M_PI / 4.0f),
                                     cosOffset * cosf(player.angle + M_PI / 4.0f),
                                     10.0f * sinf(player.angle),
                                     10.0f * cosf(player.angle),
                                     (int)16, 0.0f });
            
            vecAsteroids.push_back({ sinOffset * sinf(player.angle + M_PI / 6.0f),
                                     cosOffset * cosf(player.angle - M_PI / 6.0f),
                                     10.0f * sinf(-player.angle),
                                     10.0f * cosf(-player.angle),
                                     (int)16, 0.0f });
            break;
        }
    }

    float RandFloatClamped(float min, float max)
    {
        return (min + 1) + (((float)rand()) / (float)RAND_MAX) * (max - (min + 1));
    }

};

int main()
{
    AsteroidGame asteroidGame;
    
    if (asteroidGame.ConstructConsole(200, 120, 8, 8))
        asteroidGame.Start();

    return 0;
}


// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
