#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>

struct position
{
    unsigned int X;
    unsigned int Y;

    void zeroPos()
    {
        X = 0;
        Y = 0;
    }
};

class Blob
{
private:
    const unsigned short NORTH = 0;
    const unsigned short EAST = 1;
    const unsigned short SOUTH = 2;
    const unsigned short WEST = 3;

    struct position pos;
    int facing;
    int health;
    int food;

    // sightline

    int SPEED;
    int SIGHT_RANGE;
    int ATTACK_DAMAGE;
    int LOOKS;
    int INTELLIGENCE;

    std::vector<int> Characteristics = {SPEED, SIGHT_RANGE, ATTACK_DAMAGE, LOOKS, INTELLIGENCE};

    void PopulateGenome()
    {
        for (size_t i = 0; i < Characteristics.size(); i++)
        {
            GENOME[i] = Characteristics.at(i);
        }
    }

public:
    int GENOME[5];

    Blob()
    {
        SPEED = rand();
        SIGHT_RANGE = rand();
        ATTACK_DAMAGE = rand();
        LOOKS = rand();
        INTELLIGENCE = rand();

        PopulateGenome();
        pos.zeroPos();
        health = 100;
        food = 100;
    }

    Blob(int PAT_GENOME[], int MAT_GENOME[])
    {
        for (int i = 0; i < 3; i++)
        {
            GENOME[i] = pickOne(PAT_GENOME[i], MAT_GENOME[i]);
        }

        SPEED = GENOME[0];
        SIGHT_RANGE = GENOME[1];
        ATTACK_DAMAGE = GENOME[2];
        LOOKS = GENOME[3];
        INTELLIGENCE = GENOME[4];

        PopulateGenome();

        health = 100;
        food = 100;
    }

    void setPosition(unsigned int X, unsigned int Y)
    {
        pos.X = X;
        pos.Y = Y;
    }

    void move(unsigned int X, unsigned int Y)
    {
        int verticalSpeed = pos.Y - Y;
        int horizontalSpeed = pos.X - X;
        int costPerStep = floor(sqrt((verticalSpeed, 2) + pow(horizontalSpeed, 2)));
        int moves;

        for (size_t i = 0; i < moves; i++)
        {
        }
    }
};

int pickOne(int one, int two)
{
    return rand() % 2 == 0 ? one : two;
}