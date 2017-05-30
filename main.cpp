#pragma GCC optimize("-O3")
// #pragma GCC optimize("inline")
// #pragma GCC optimize("omit-frame-pointer")
// #pragma GCC optimize("unroll-loops")


/*
NOTES:


*/


#include <iostream>
#include <string>
#include <vector>   // may optimze out
#include <algorithm>
#include <climits>

using namespace std;

int _turn;

static unsigned int fr_seed;
inline void fast_srand ( int seed )
{
    //Seed the generator
    fr_seed = seed;
}
inline int fastrand ()
{
    //fastrand routine returns one integer, similar output value range as C lib.
    fr_seed = ( 214013 * fr_seed + 2531011 );
    return ( fr_seed >> 16 ) & 0x7FFF;
}

enum class Option {STARBOARD, PORT, WAIT, FASTER, SLOWER, FIRE, MINE};

// Basic map unit. Coordinates exist as X,Y but are better suited to X,Y,Z. Most
// calculations will utilize XYZ.
struct Cube
{
    Cube () {}
    Cube (int X, int Y) : x( X - (Y - (Y & 1)) / 2), z(Y), Xo(X), Yo(Y) {y = -x-z;}
    Cube (int X, int Y, int Z) : x(X), y(Y), z(Z), Xo(X + (Z - (Z & 1)) / 2), Yo(Z) {}
    int x = INT_MAX;
    int y = INT_MAX;
    int z = INT_MAX;

    int Xo = INT_MAX; // odd-r
    int Yo = INT_MAX; // odd-r
};


inline bool operator==(const Cube& lhs, const Cube& rhs){ return lhs.Xo == rhs.Xo && lhs.Yo == rhs.Yo; };
inline bool operator!=(const Cube& lhs, const Cube& rhs){ return !operator==(lhs,rhs); };

void InFront(Cube &c, int dir);
vector<Cube> _ShotTemplate;
void BuildShotTemplate();
int GetRandomCutoffs(int* cutoffs, int shots_size, int mine, int moves);


struct ShipVec
{
    ShipVec() {}
    ShipVec(Cube Location, int Direction, int Speed) : loc(Location), dir(Direction), speed(Speed) {}
    Cube loc;
    int dir;
    int speed;
};

struct Action
{
    Action() {}
    Action(ShipVec Ship_vector, Option option) : vec(Ship_vector), opt(option) {}
    Action(ShipVec Ship_vector, Option option, Cube Action_Location) : vec(Ship_vector), opt(option), action_loc(Action_Location) {}
    Action(Cube Location, int Direction, int Speed, Option option) : vec(ShipVec(Location, Direction, Speed)), opt(option) {}
    ShipVec vec;
    Cube action_loc;
    Option opt;
};


class Ship
{
public:
    Ship() {}
    Ship(int X, int Y, int ID, int Direction, int Rum, int Speed) : id(ID), rum(Rum), vec(ShipVec(Cube(X,Y),Direction,Speed)) {}
    Ship(int ID, int Rum, ShipVec Ship_vector) : id(ID), rum(Rum), vec(Ship_vector) {}
    ShipVec vec;

    int id = -1;
    int mine_dropped;       // Last turn a mine was dropped
    int fired_last;         // Last turn a shot was fired (can only fire every other)
    int rum;
    vector<Action> actions;
    int cutoffs[7];


    /*
        Fills the actions vector with all possible actions. Also fills the cutoffs
        array and returns the value to mod by to fill actions.
    */
    int FillActions(int sim_turn)
    {
        int shots = 0;
        int mines = 0;
        if (sim_turn - fired_last > 1)
            shots = TranslatePossibleShots(vec.loc, vec.dir, vec.speed);
        if (sim_turn - mine_dropped > 4)
        {
            PossibleMine();
            mines = 1;
        }

        return GetRandomCutoffs(cutoffs, shots, mines, PossibleMoves());
    }

    string BestAction()
    {
        string output = "WAIT ";
        // Run simulations

        return output;
    }

private:
    int PossibleMoves()
    {
        Cube new_loc = vec.loc;
        int new_starboard_dir = vec.dir == 0 ? 5 : vec.dir - 1;
        int new_port_dir = vec.dir == 5 ? 0 : vec.dir + 1;


        if (vec.speed == 0)
        {
            ShipVec wait(new_loc,vec.dir, 0);
            ShipVec starboard(new_loc,new_starboard_dir, 0);
            ShipVec port(new_loc, new_port_dir, 0);

            InFront(new_loc, vec.dir);
            ShipVec faster(new_loc, vec.dir, 1);

            actions.emplace_back(wait, Option::WAIT);
            actions.emplace_back(starboard, Option::STARBOARD);
            actions.emplace_back(port, Option::PORT);
            actions.emplace_back(faster, Option::FASTER);
            return 4;
        }
        else if (vec.speed == 1)
        {
            ShipVec slower(new_loc,vec.dir, 0);

            InFront(new_loc, vec.dir);
            ShipVec wait(new_loc,vec.dir, 1);
            ShipVec starboard(new_loc,new_starboard_dir, 1);
            ShipVec port(new_loc, new_port_dir, 1);

            InFront(new_loc, vec.dir);
            ShipVec faster(new_loc, vec.dir, 2);
            
            actions.emplace_back(wait, Option::WAIT);
            actions.emplace_back(slower, Option::SLOWER);
            actions.emplace_back(starboard, Option::STARBOARD);
            actions.emplace_back(port, Option::PORT);
            actions.emplace_back(faster, Option::FASTER);            
            return 5;
        }
        else
        {
            InFront(new_loc, vec.dir);
            ShipVec slower(new_loc,vec.dir, 1);

            InFront(new_loc, vec.dir);
            ShipVec wait(new_loc,vec.dir, 2);
            ShipVec starboard(new_loc,new_starboard_dir, 2);
            ShipVec port(new_loc, new_port_dir, 2);   

            actions.emplace_back(wait, Option::WAIT);
            actions.emplace_back(slower, Option::SLOWER);
            actions.emplace_back(starboard, Option::STARBOARD);
            actions.emplace_back(port, Option::PORT);
            return 4;
        }
    }

    /*
        Translates each Cube in _ShotTemplate to be based on the new bow.
        Only adds Cubes that are in the map boundry.
    */
    int TranslatePossibleShots(Cube center, int dir, int speed)
    {
        int ret_val = 0;
        Cube new_loc = center;
        if (speed > 0)
            InFront(new_loc, dir);
        if (speed > 1)
            InFront(new_loc, dir);

        // Shoot from bow...so move the center forward one to the bow
        InFront(center, dir);
        int dx = 1000 - center.x;
        int dy = 1000 - center.y;
        int dz = 1000 - center.z;
        // 331 = _ShotTemplate.size()
        for (unsigned int i = 0; i < 331; ++i)
        {
            Cube cube(_ShotTemplate[i].x - dx, _ShotTemplate[i].y - dy, _ShotTemplate[i].z - dz);
            if (cube.Xo >= 0 && cube.Xo < 23 && cube.Yo >= 0 && cube.Yo < 21)
            {
                actions.emplace_back(ShipVec(new_loc, dir, speed), Option::FIRE, cube);
                ++ret_val;
            }
        }
        return ret_val;
    }

    void PossibleMine()
    {
        Cube mine_drop = vec.loc;       // ship center
        int drop_dir = (vec.dir + 3) % 6;
        InFront(mine_drop, drop_dir);   // stern
        InFront(mine_drop, drop_dir);   // cell directly behind the ship
        Cube new_loc = vec.loc;
        if (vec.speed > 0)
            InFront(new_loc, vec.dir);
        if (vec.speed > 1)
            InFront(new_loc, vec.dir);
        actions.emplace_back(ShipVec(new_loc, vec.dir, vec.speed), Option::MINE, mine_drop);
    }

    /*
        shots_size: 0-331   mine: 0 or 1
        The cutoffs array stores cutoffs. I wanted at least 2 of the 5 actions to be 
        movement related. Since there are 331 possible shots and only 5 possible moves
        it is necessary to represent them as a percentage of total. I assume shots 
        make up 55% of the pool, mines 5%, and movement the remainder. In the event 
        there are no possible shots (fire last round), or mines, the function handles
        that elegantly (0 * anything = 0, 0 + anything = anything).

        Returns the amount fast_rand() will be % with.
    */
    int GetRandomCutoffs(int* cutoffs, int shots_size, int mine, int moves)
    {

        float total = shots_size == 0 ? 100.0 : shots_size / .55;

        float mine_tot = mine * total * .05;
        float remainder = total - shots_size - mine_tot;
        float move_tot = remainder / moves;
        float running_total = shots_size;

        cutoffs[0] = int(running_total);
        running_total += mine_tot;
        cutoffs[1] = int(running_total);
        for (unsigned int i = 2; i < moves + 2; ++i)
        {
            running_total += move_tot;
            cutoffs[2] = int(running_total);
        }
        
        return running_total;
    }
};
vector<Ship> _my_ships;
vector<Ship> _en_ships;


struct Barrel
{
    Barrel() {}
    Barrel(int X, int Y, int Rum, int ID) : loc(Cube(X,Y)), rum(Rum), id(ID) {}
    Cube loc;

    int id = -1;
    int rum;
    bool enroute = false;
    bool marked = false;    // marked for destruction...cannon ball incoming
};
vector<Barrel> _barrels;

struct Cannonball
{
    Cannonball() {}
    Cannonball(int X, int Y, int Impact, int Turn, int ID) : loc(Cube(X,Y)), impact(Impact), turn(Turn), id(ID) {}
    Cube loc;

    int turn;
    int impact;
    int id = -1;
    Ship* origin = nullptr;     // Primarily keeping track of this to identify which ship cannot fire
};
vector<Cannonball> _cbs;

struct Mine
{
    Mine() {}
    Mine(int X, int Y, int ID) : loc(Cube(X,Y)), id(ID) {}
    Cube loc;

    int id = -1;
};
vector<Mine> _mines;



int main()
{
    _turn = 0;
    BuildShotTemplate();

    while (1) {
        vector<Ship> my_ships,en_ships;
        int myShipCount; // the number of remaining ships
        cin >> myShipCount; cin.ignore();
        int entityCount; // the number of entities (e.g. ships, mines or cannonballs)
        cin >> entityCount; cin.ignore();
        for (int i = 0; i < entityCount; i++) {
            int entityId;
            string entityType;
            int x;
            int y;
            int arg1;
            int arg2;
            int arg3;
            int arg4;
            cin >> entityId >> entityType >> x >> y >> arg1 >> arg2 >> arg3 >> arg4; cin.ignore();
            bool found = false;
            if (entityType == "SHIP")
            {
                Ship new_ship(x,y, entityId,arg1,arg3,arg2);                
                if (arg4 == 1)
                {
                    for (unsigned int j = 0; j < _my_ships.size(); ++j)
                    {
                        if (_my_ships[j].id == new_ship.id)
                        {
                            found = true;
                            _my_ships[j].vec.loc = new_ship.vec.loc;
                            _my_ships[j].vec.dir = new_ship.vec.dir;
                            _my_ships[j].vec.speed = new_ship.vec.speed;
                            _my_ships[j].rum = new_ship.rum;                            
                        }
                    }
                    if (!found)
                        _my_ships.emplace_back(new_ship);

                }
                else
                {
                    for (unsigned int j = 0; j < _en_ships.size(); ++j)
                    {
                        if (_en_ships[j].id == new_ship.id)
                        {
                            found = true;
                            _en_ships[j].vec.loc = new_ship.vec.loc;
                            _en_ships[j].vec.dir = new_ship.vec.dir;
                            _en_ships[j].vec.speed = new_ship.vec.speed;
                            _en_ships[j].rum = new_ship.rum;                            
                        }
                    }
                    if (!found)
                        _en_ships.emplace_back(new_ship);
                }
            }
            else if (entityType == "BARREL")
            {
                Barrel new_barrel(x,y,arg1,entityId);
                for (unsigned int j = 0; j < _barrels.size(); ++j)
                {
                    if (_barrels[j].id == new_barrel.id)
                    {
                        found = true;
                        _barrels[j].loc = new_barrel.loc;
                        _barrels[j].rum = new_barrel.rum;
                    }
                }
                if (!found)
                    _barrels.emplace_back(new_barrel);                
            }
            else if (entityType == "CANNONBALL")
            {
                Cannonball new_cb(x,y,arg2,_turn,entityId);
                for (unsigned int j = 0; j < _cbs.size(); ++j)
                    if (_cbs[j].id == new_cb.id)
                        found = true;

                if (!found)
                {
                    for (unsigned int j = 0; j < _en_ships.size(); ++j)
                        if (arg2 == _en_ships[j].id)
                            new_cb.origin = &_en_ships[j];     // Update now or later??? (ship that launched)
                    _cbs.emplace_back(new_cb);                    
                }
            }
            else if (entityType == "MINE")
            {
                Mine new_mine(x,y,entityId);
                for (unsigned int j = 0; j < _mines.size(); ++j)
                    if (_mines[j].id == new_mine.id)
                        found = true;

                if (!found)
                    _mines.emplace_back(new_mine);                    
            }

        }
        for (int i = 0; i < myShipCount; i++) {

            // Write an action using cout. DON'T FORGET THE "<< endl"
            // To debug: cerr << "Debug messages..." << endl;

            cout << "MOVE 11 10" << endl; // Any valid action, such as "WAIT" or "MOVE x y"
        }
    }



    return 0;
}

/*  
    Fills the vector of all hexes within a radius of a point. Generates 331
    Cubes (shot locations)
*/
void BuildShotTemplate()
{
    Cube bow(1000,1000,1000);
    // Can be optimized:
    for (unsigned int i = -10 + bow.x; i <= 10 + bow.x; ++i)
        for (unsigned int j = -10 + bow.y; j <= 10 + bow.y; ++j)
            for (unsigned int k = -10 + bow.z; k <= 10 + bow.z; ++k)
                if (i + j + k == 0)
                        _ShotTemplate.emplace_back(i,j,k);
}





void InFront(Cube &c, int dir)
{
    if (dir == 0)
    {
        c.x += 1;
        c.y -= 1;
    }
    else if (dir == 3)
    {
        c.x -= 1;
        c.y += 1;
    }
    else if (dir == 1)
    {
        c.x += 1;
        c.z -= 1;
    }
    else if (dir == 4)
    {
        c.x -= 1;
        c.z += 1;
    }
    else if (dir == 2)
    {
        c.y += 1;
        c.z -= 1;
    }
    else
    {
        c.y -= 1;
        c.z += 1;
    }
}
























