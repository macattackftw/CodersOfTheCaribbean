#pragma GCC optimize("-O3")
// #pragma GCC optimize("inline")
// #pragma GCC optimize("omit-frame-pointer")
// #pragma GCC optimize("unroll-loops")


/*
NOTES:
    I'm not sure if storing viable shots is better than recalculating them. Intuition
    is telling me to keep it as is (save each vector of Cubes).

    I recognize that it would be faster to test for movement/shot first so that I may
    not even have to calculate all the shot data but I think this will yield more
    accurate `ribution. Also, the first turn we are permitted more time. I may use
    the extra time to fill shot vectors.

    Shot options will be reworked. Currently I get all available shots and use random to
    determine which one will be scored. The way I'm setting up the simulations doesn't need
    this. Shots will be scored based on a shot template (which is based on boat speed),
    nearby vessels, mines, and incoming shots.

    ALSO, enemy vessels need to be ran before my ships, so we can predict where/what they
    are likely to do. I may keep a global variable or two that will classify the player based
    on if they are behaving as my model expects.

    Watched my boat drop a mine right in the path of one of my own ships. Need to detect nearby
    friendlies and not drop if they are likely to hit it.

    When collision with another ship happens my ships don't know it and think they are still at
    their previous speed. Need to make sure this is accounted for. Ship turned into a mine 
    because it thought it was going to be forward one hex.

*/


#include <iostream>
#include <string>
#include <vector>   // may optimze out
#include <algorithm>
#include <climits>
#include <utility>  // pair
#include <queue>    // priority queue
// #include <unordered_map>

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

enum class Option {STARBOARD, PORT, WAIT, FASTER, SLOWER, FIRE, MINE, NONE};

// Basic map unit. Coordinates exist as X,Y but are better suited to X,Y,Z. Most
// calculations will utilize XYZ.
struct Cube
{
    Cube () {}
    Cube (int X, int Y) : x( X - (Y - (Y & 1)) / 2), z(Y), Xo(X), Yo(Y) {y = -x - z;}
    Cube (int X, int Y, int Z) : x(X), y(Y), z(Z), Xo(X + (Z - (Z & 1)) / 2), Yo(Z) {}
    int x = INT_MAX;
    int y = -INT_MAX;
    int z = 0;

    int Xo = INT_MAX; // odd-r
    int Yo = INT_MAX; // odd-r
};
Cube _center(11, 10);

inline bool operator==(const Cube& lhs, const Cube& rhs) { return lhs.Xo == rhs.Xo && lhs.Yo == rhs.Yo; };
inline bool operator!=(const Cube& lhs, const Cube& rhs) { return !operator==(lhs, rhs); };

float ManhattanDist(const Cube &origin, const Cube &dest);
void InFront(Cube &c, int dir);
vector<Cube> _shot_template;
vector<Cube> _target_template;
vector<Cube> _mine_check_template;
// unordered_map<Cube, vector<Cube> > _shot_vectors;
// _shot_vectors.reserve(400);
vector<pair<Cube, vector<Cube> > > _shot_vectors;
void BuildShotTemplate();
void BuildMineCheckTemplate();
int GetRandomCutoffs(int* cutoffs, int shots_size, int mine, int moves);


struct Barrel
{
    Barrel() {}
    Barrel(int X, int Y, int Rum, int ID) : loc(Cube(X, Y)), rum(Rum), id(ID) {}
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
    Cannonball(int X, int Y, int Impact, int Turn, int ID) : loc(Cube(X, Y)), impact(Impact), turn(Turn), id(ID) {}
    Cube loc;

    int turn;
    int impact;     // turns till impact
    int id = -1;
};
vector<Cannonball> _cbs;

void AdvanceCannonBalls(vector<Cannonball> &cbs)
{
    for (unsigned int i = 0; i < cbs.size(); ++i)
        --cbs[i].impact;
}

struct Mine
{
    Mine() {}
    Mine(int X, int Y, int ID) : loc(Cube(X, Y)), id(ID) {}
    Cube loc;

    int id = -1;
};
vector<Mine> _mines;


struct ShipVec
{
    ShipVec() {}
    ShipVec(Cube Location, int Direction, int Speed) : loc(Location), dir(Direction), speed(Speed) {}
    Cube loc;
    int dir;
    int speed;
};

class Action
{
public:
    Action() {}
    // Action(ShipVec Ship_vector, Option option, Cube Action_Location = Cube()) : vec(Ship_vector), opt(option), action_loc(Action_Location) {EvaluateFitness();}
    Action(ShipVec Ship_vector, Option option, Action action, Cube Action_Location = Cube()) : vec(Ship_vector), opt(option), action_loc(Action_Location) {UpdateActions(action); EvaluateFitness(); }
    ShipVec vec;
    Cube action_loc;
    Option opt = Option::NONE;
    vector<Action> prev_actions;
    float fitness = 0.0f;

private:
    // Should never be more than 4 previous actions
    void UpdateActions(const Action &a)
    {
        // May be necessary to pull this out. Shots can have up to 331 and we can do this check
        // one time or 331 times, but it is ugly. Unless performance is too slow this will remain.
        if (a.opt != Option::NONE)
        {
            prev_actions = a.prev_actions;
            fitness = a.fitness;
            prev_actions.push_back(a);
        }
    }

    // Will evaluate the fitness of an action. It will be += because it will inherit
    // the fitness of the previous move.
    void EvaluateFitness()
    {
        // Cube movement = vec.loc;

        // fitness += (fastrand() % 100 + 0.0);
    }


};
// inline bool operator> (const Action& lhs, const Action& rhs){ return lhs.fitness > rhs.fitness; };
inline bool operator< (const Action& lhs, const Action& rhs) { return lhs.fitness < rhs.fitness; };
bool VecSort(const Action& lhs, const Action& rhs) {return lhs.fitness > rhs.fitness; };


float OnBarrel(const Cube &center, const int &dir, const vector<Barrel> &barrels);
float OnMine(const ShipVec &s, const vector<Mine> &mines);
float MineInFront(const ShipVec &s, const vector<Mine> &mines);
float OnCannonball(const ShipVec &s, const vector<Cannonball> &cbs);
float OnEdge(const Cube &center, const int &dir);


class Ship
{
public:
    Ship() {}
    Ship(int X, int Y, int ID, int Direction, int Rum, int Speed) : id(ID), rum(Rum), vec(ShipVec(Cube(X, Y), Direction, Speed)) {}
    // Ship(int ID, int Rum, ShipVec Ship_vector) : id(ID), rum(Rum), vec(Ship_vector) {}
    Ship(const Ship &s, Action Best_Action) : id(s.id), rum(s.rum), mine_dropped(s.mine_dropped), fired_last(s.fired_last), vec(Best_Action.vec), best_action(Best_Action) {}
    // vec = best_action.opt == Option::NONE ? s.vec : best_action.vec;}

    ShipVec vec;
    int id = -1;
    int mine_dropped = -10;         // Last turn a mine was dropped
    int fired_last = -10;           // Last turn a shot was fired (can only fire every other)
    int rum;
    vector<Action> actions;         // All possible actions for this ship at this time
    Action best_action;             // Actions carry all previous actions so this would be a map of actions
    int cutoffs[7];
    int cutoff;

    ShipVec goal;

    /*
        Fills the actions vector with all possible actions. Also fills the cutoffs
        array and returns the value to mod by to fill actions.
    */
    void FillActions(int sim_turn, const vector<Ship> &my_ships, const vector<Ship> &en_ships, const vector<Mine> &mines, const vector<Cannonball> &cbs, const vector<Barrel> &barrels)
    {
        actions.clear();
        int shots = 0;
        int mine_actions = 0;
        make_heap(actions.begin(), actions.end(), VecSort);

        // cerr << "sim: " << sim_turn << "\tfired: " << fired_last << endl;
        // if (sim_turn - fired_last > 1)
        if (false)
        {
            Cube bow = vec.loc;
            InFront(bow, vec.dir);
            vector<Cube> viable_shots;
            bool found = false;


            for (unsigned int i = 0; i < _shot_vectors.size(); ++i)
            {
                if (_shot_vectors[i].first == bow)
                {
                    found = true;
                    viable_shots = _shot_vectors[i].second;
                    break;
                }
            }
            if (!found)
            {
                viable_shots = TranslatePossibleShots(vec.loc, vec.dir, vec.speed);
                _shot_vectors.emplace_back(bow, viable_shots);
            }

            // unordered_map<Cube,vector<Cube> >::const_iterator found = _shot_vectors.find (bow);
            // if (found == _shot_vectors.end())
            // {
            //     viable_shots = TranslatePossibleShots(vec.loc, vec.dir, vec.speed);
            //     _shot_vectors.emplace(bow, viable_shots);
            // }
            // else
            // {
            //     viable_shots = found->second;
            // }

            shots = viable_shots.size();
            Cube new_loc = vec.loc;
            float expected_damage = 0.0f;
            if (vec.speed == 0)
            {
                expected_damage += FitnessModification(ShipVec(new_loc, vec.dir, vec.speed), mines, cbs, barrels, my_ships, en_ships, id);
            }
            else if (vec.speed == 1)
            {
                InFront(new_loc, vec.dir);
                expected_damage += FitnessModification(ShipVec(new_loc, vec.dir, vec.speed), mines, cbs, barrels, my_ships, en_ships, id);
            }
            else if (vec.speed == 2)
            {
                // Does not properly account for mines at the moment...not entirely sure how I want to deal with it.
                InFront(new_loc, vec.dir);
                InFront(new_loc, vec.dir);
                expected_damage += FitnessModification(ShipVec(new_loc, vec.dir, vec.speed), mines, cbs, barrels, my_ships, en_ships, id);

            }

            for (unsigned int i = 0; i < shots; ++i)
            {
                Action new_shot(ShipVec(new_loc, vec.dir, vec.speed), Option::FIRE, best_action, viable_shots[i]);
                new_shot.fitness += expected_damage;
                actions.push_back(new_shot);
                push_heap(actions.begin(), actions.end());

            }
        }
        if (sim_turn - mine_dropped > 4)
        {
            PossibleMine(mines, cbs, barrels, my_ships, en_ships);
            mine_actions = 1;
        }
        int moves = PossibleMoves(mines, cbs, barrels, my_ships, en_ships);
        cutoff = GetRandomCutoffs(cutoffs, shots, mine_actions, moves);
        // cerr << "action size: " << actions.size() << endl;
    }

    /*
        Since cutoffs for moves/mines were weighted they must be accounted for. Cutoffs[0] marks
        all available shots so it should return whichever shot it hit. cutoffs[1] marks mine
        and since it makes up 5% of the cutoff, must reference action[cutoffs[1]. Moves work
        the same. They make up some % of "cutoff" and so if the rand returned a value below
        the cutoff it must reference the start
    */
    Action InitialAction()
    {
        int cut = fastrand() % cutoff;
        int move = cutoffs[0] + (cutoffs[0] == cutoffs[1] ? 0 : 1);
        if (cut < cutoffs[0])
            return actions[cut];
        else if (cut < cutoffs[1])
            return actions[cutoffs[0]];
        else if (cut < cutoffs[2])
            return actions[move];
        else if (cut < cutoffs[3])
            return actions[move + 1];
        else if (cut < cutoffs[4])
            return actions[move + 2];
        else if (cut < cutoffs[5])
            return actions[move + 3];
        else //if (cut < cutoffs[6])
            return actions[move + 4];
    }

    float FitnessModification(const ShipVec &s, const vector<Mine> &mines, const vector<Cannonball> &cbs, const vector<Barrel> &barrels, const vector<Ship> &my_ships, const vector<Ship> &en_ships, int id)
    {
        float ret_val = 0.0;
        // vector<int> impact_ids;
        // zero speed is very vulnerable
        if (s.speed == 0)
            ret_val -= 9.0f;
        ret_val += OnEdge(s.loc, s.dir);
        ret_val += OnMine(s, mines);
        // cerr << "after mines: " << ret_val << endl;
        if (s.speed > 0)
            ret_val += MineInFront(s, mines);
        // cerr << "after mines: " << ret_val << endl;
        ret_val += OnCannonball(s, cbs);
        ret_val += OnShip(s.loc, s.dir, my_ships, en_ships, id);
        // cerr << "after onship: " << ret_val << endl;
        ret_val += ShipBuffer(s, my_ships, en_ships, id);
        // cerr << "after buffer: " << ret_val << endl;
        ret_val += OnBarrel(s.loc, s.dir, barrels);
        ret_val += NextToMine(s, mines, cbs);
        // cerr << "after mines: " << ret_val << endl;
        ret_val += BarrelFitnessBoost(s, barrels);
        if (ShipInFront(s, my_ships, en_ships, ret_val) == 0)
            ret_val -= 25.0f;

        return ret_val;
    }


    // Returns true if a ship is blocking this ship's path.
    int ShipInFront(const ShipVec &s, const vector<Ship> &my_ships, const vector<Ship> &en_ships, float &ret_val)
    {
        Cube bow = s.loc;
        InFront(bow, s.dir);
        Cube my_goal = bow;
        InFront(my_goal, s.dir);
        Cube boosted = my_goal;
        if (s.speed == 2)
        {
            InFront(boosted, s.dir);
        }
        
        for (unsigned int i = 0; i < my_ships.size(); ++i)
        {
            if (my_ships[i].id != id)
            {
                Cube en_bow = my_ships[i].vec.loc;
                InFront(en_bow, my_ships[i].vec.dir);
                if (my_ships[i].vec.speed == 2)
                    InFront(en_bow, my_ships[i].vec.dir);
                Cube en_goal = en_bow;
                InFront(en_goal, my_ships[i].vec.dir);
                // Assumes I'm not trying to box an enemy in
                if (bow == en_bow || bow == en_goal || boosted == en_bow || boosted == en_goal || my_goal == en_bow || my_goal == en_goal)
                {
                    if (my_ships[i].best_action.opt != Option::NONE && my_ships[i].best_action.opt == Option::FASTER)
                        ret_val -= 100.0f;
                    if (ManhattanDist(s.loc, my_ships[i].vec.loc) <= 4)
                        return 0;
                    else 
                        return 1;
                }
            }
        }

        for (unsigned int i = 0; i < en_ships.size(); ++i)
        {
            if (en_ships[i].id != id)
            {
                Cube en_bow = en_ships[i].vec.loc;
                InFront(en_bow, en_ships[i].vec.dir);
                if (en_ships[i].vec.speed == 2)
                    InFront(en_bow, en_ships[i].vec.dir);
                Cube en_goal = en_bow;
                InFront(en_goal, en_ships[i].vec.dir);
                // Assumes I'm not trying to box an enemy in
                if (bow == en_bow || bow == en_goal || boosted == en_bow || boosted == en_goal || my_goal == en_bow || my_goal == en_goal)
                {
                    if (ManhattanDist(s.loc, en_ships[i].vec.loc) <= 4)
                        return 0;
                    else 
                        return 1;
                }
            }
        }
        
        return s.speed;
    }

private:

    /*
        Updates actions vector with the possible moves. Returns an integer
        representing the number of moves.
    */
    int PossibleMoves(const vector<Mine> &mines, const vector<Cannonball> &cbs, const vector<Barrel> &barrels, const vector<Ship> &my_ships, const vector<Ship> &en_ships)
    {
        Cube new_loc = vec.loc;
        int new_starboard_dir = vec.dir == 0 ? 5 : vec.dir - 1;
        int new_port_dir = vec.dir == 5 ? 0 : vec.dir + 1;
        cerr << endl << "cb size: " << cbs.size() << "\t" << id << ":\n";

        for (unsigned int i = 0; i < cbs.size(); ++i)
            cerr << "id: " << cbs[i].id << "\timpact: " << cbs[i].impact << endl;

        if (vec.speed == 0)
        {
            ShipVec wait_vec(new_loc, vec.dir, 0);
            ShipVec starboard_vec(new_loc, new_starboard_dir, 0);
            ShipVec port_vec(new_loc, new_port_dir, 0);

            InFront(new_loc, vec.dir);
            ShipVec faster_vec(new_loc, vec.dir, 1);

            Action wait(wait_vec, Option::WAIT, best_action);
            wait.fitness += FitnessModification(wait_vec, mines, cbs, barrels, my_ships, en_ships, id);
            cerr << "\twait fitness: " << wait.fitness << endl;
            actions.push_back(wait);
            push_heap(actions.begin(), actions.end());

            Action starboard(starboard_vec, Option::STARBOARD, best_action);
            starboard.fitness += FitnessModification(starboard_vec, mines, cbs, barrels, my_ships, en_ships, id);
            cerr << "\tstarboard fitness: " << starboard.fitness << endl;
            actions.push_back(starboard);
            push_heap(actions.begin(), actions.end());

            Action port(port_vec, Option::PORT, best_action);
            port.fitness += FitnessModification(port_vec, mines, cbs, barrels, my_ships, en_ships, id);
            cerr << "\tport fitness: " << port.fitness << endl;
            actions.push_back(port);
            push_heap(actions.begin(), actions.end());

            Action faster(faster_vec, Option::FASTER, best_action);
            faster.fitness += FitnessModification(faster_vec, mines, cbs, barrels, my_ships, en_ships, id);
            cerr << "\tfaster fitness: " << faster.fitness << endl;
            actions.push_back(faster);
            push_heap(actions.begin(), actions.end());
            return 4;
        }
        else if (vec.speed == 1)
        {

            ShipVec slower_vec(new_loc, vec.dir, 0);

            InFront(new_loc, vec.dir);
            ShipVec wait_vec(new_loc, vec.dir, 1);
            ShipVec starboard_vec(new_loc, new_starboard_dir, 1);
            ShipVec port_vec(new_loc, new_port_dir, 1);
            // float mine_damage = MineInFront(vec, mines);

            InFront(new_loc, vec.dir);
            ShipVec faster_vec(new_loc, vec.dir, 2);

            Action wait(wait_vec, Option::WAIT, best_action);
            wait.fitness += FitnessModification(wait_vec, mines, cbs, barrels, my_ships, en_ships, id);// + mine_damage;
            cerr << "\twait fitness: " << wait.fitness << endl;
            actions.push_back(wait);
            push_heap(actions.begin(), actions.end());

            Action slower(slower_vec, Option::SLOWER, best_action);
            slower.fitness += FitnessModification(slower_vec, mines, cbs, barrels, my_ships, en_ships, id);
            cerr << "\tslower fitness: " << slower.fitness << endl;
            actions.push_back(slower);
            push_heap(actions.begin(), actions.end());

            Action starboard(starboard_vec, Option::STARBOARD, best_action);
            starboard.fitness += FitnessModification(starboard_vec, mines, cbs, barrels, my_ships, en_ships, id);// + mine_damage;
            cerr << "\tstarboard fitness: " << starboard.fitness << endl;
            actions.push_back(starboard);
            push_heap(actions.begin(), actions.end());

            Action port(port_vec, Option::PORT, best_action);
            port.fitness += FitnessModification(port_vec, mines, cbs, barrels, my_ships, en_ships, id);// + mine_damage;
            cerr << "\tport fitness: " << port.fitness << endl;
            actions.push_back(port);
            push_heap(actions.begin(), actions.end());

            Action faster(faster_vec, Option::FASTER, best_action);
            faster.fitness += FitnessModification(faster_vec, mines, cbs, barrels, my_ships, en_ships, id);// + mine_damage;
            cerr << "\tfaster fitness: " << faster.fitness << endl;
            actions.push_back(faster);
            push_heap(actions.begin(), actions.end());
            return 5;
        }
        else
        {
            InFront(new_loc, vec.dir);
            ShipVec slower_vec(new_loc, vec.dir, 1);

            InFront(new_loc, vec.dir);
            ShipVec wait_vec(new_loc, vec.dir, 2);
            ShipVec starboard_vec(new_loc, new_starboard_dir, 2);
            ShipVec port_vec(new_loc, new_port_dir, 2);

            Action wait(wait_vec, Option::WAIT, best_action);
            wait.fitness += FitnessModification(wait_vec, mines, cbs, barrels, my_ships, en_ships, id);// + mine_damage;
            cerr << "\twait fitness: " << wait.fitness << endl;
            actions.push_back(wait);
            push_heap(actions.begin(), actions.end());

            Action slower(slower_vec, Option::SLOWER, best_action);
            slower.fitness += FitnessModification(slower_vec, mines, cbs, barrels, my_ships, en_ships, id);
            cerr << "\tslower fitness: " << slower.fitness << endl;
            actions.push_back(slower);
            push_heap(actions.begin(), actions.end());

            Action starboard(starboard_vec, Option::STARBOARD, best_action);
            starboard.fitness += FitnessModification(starboard_vec, mines, cbs, barrels, my_ships, en_ships, id);// + mine_damage;
            cerr << "\tstarboard fitness: " << starboard.fitness << endl;
            actions.push_back(starboard);
            push_heap(actions.begin(), actions.end());

            Action port(port_vec, Option::PORT, best_action);
            port.fitness += FitnessModification(port_vec, mines, cbs, barrels, my_ships, en_ships, id);// + mine_damage;
            cerr << "\tport fitness: " << port.fitness << endl;
            actions.push_back(port);
            push_heap(actions.begin(), actions.end());
            return 4;
        }
    }

    /*
        Translates each Cube in _shot_template to be based on the new bow.
        Only adds Cubes that are in the map boundry.
    */
    vector<Cube> TranslatePossibleShots(Cube center, int dir, int speed)
    {
        vector<Cube> ret_val;

        // Shoot from bow...so move the center forward one to the bow
        InFront(center, dir);

        // 331 = _shot_template.size()
        for (unsigned int i = 0; i < 331; ++i)
        {
            Cube cube(_shot_template[i].x + center.x, _shot_template[i].y + center.y, _shot_template[i].z + center.z);
            if (cube.Xo >= 0 && cube.Xo < 23 && cube.Yo >= 0 && cube.Yo < 21)
                ret_val.push_back(cube);
        }
        return ret_val;
    }

    void PossibleMine(const vector<Mine> &mines, const vector<Cannonball> &cbs, const vector<Barrel> &barrels, const vector<Ship> &my_ships, const vector<Ship> &en_ships)
    {
        Cube mine_drop = vec.loc;       // ship center
        int drop_dir = (vec.dir + 3) % 6;
        InFront(mine_drop, drop_dir);   // stern
        InFront(mine_drop, drop_dir);   // cell directly behind the ship
        // cerr << "center: " << vec.loc.Xo << "," << vec.loc.Yo << endl;
        // cerr << "mine.loc: " << mine_drop.Xo << "," << mine_drop.Yo << endl;
        // If there is already a mine there do not place one.
        for (unsigned int i = 0; i < _mines.size(); ++i)
            if (_mines[i].loc == mine_drop)
                return;

        Cube new_loc = vec.loc;
        if (vec.speed > 0)
            InFront(new_loc, vec.dir);
        if (vec.speed > 1)
            InFront(new_loc, vec.dir);
        Action mine_action(ShipVec(new_loc, vec.dir, vec.speed), Option::MINE, best_action, mine_drop);
        mine_action.fitness += FitnessModification(mine_action.vec, mines, cbs, barrels, my_ships, en_ships, id);
        actions.push_back(mine_action);
        push_heap(actions.begin(), actions.end());
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

        float total = shots_size == 0 ? 100.0f : shots_size / .55f;
        float mine_tot = mine * total * .05f;
        float remainder = total - shots_size - mine_tot;
        float move_tot = remainder / moves;
        float running_total = shots_size;

        cutoffs[0] = int(running_total);
        running_total += mine_tot;
        cutoffs[1] = int(running_total);
        for (unsigned int i = 2; i < moves + 2; ++i)
        {
            running_total += move_tot;
            cutoffs[i] = int(running_total);
        }

        return running_total;
    }



    // Returns all hexes adjacent to the bow and stern.
    vector<Cube> TranslateAdjacentHexes(const ShipVec &s)
    {
        vector<Cube> ret_val;
        Cube bow = s.loc;
        InFront(bow, s.dir);
        Cube stern = s.loc;
        InFront(stern, (s.dir + 3) % 6 );

        // 7 = _shot_template.size()
        for (unsigned int i = 0; i < 7; ++i)
        {
            Cube bow_cube(_mine_check_template[i].x + bow.x, _mine_check_template[i].y + bow.y, _mine_check_template[i].z + bow.z);
            Cube stern_cube(_mine_check_template[i].x + stern.x, _mine_check_template[i].y + stern.y, _mine_check_template[i].z + stern.z);
            if (bow_cube.Xo >= 0 && bow_cube.Xo < 23 && bow_cube.Yo >= 0 && bow_cube.Yo < 21 &&
                    bow_cube != bow && bow_cube != s.loc && bow_cube != stern)
                ret_val.push_back(bow_cube);
            if (stern_cube.Xo >= 0 && stern_cube.Xo < 23 && stern_cube.Yo >= 0 && stern_cube.Yo < 21 &&
                    stern_cube != bow && stern_cube != s.loc && stern_cube != stern)
                ret_val.push_back(stern_cube);
        }
        return ret_val;
    }

    // Kind of heavy but checks each spot for a mine then checks if a shot is incoming. Incoming shots
    // have the potential to cause 10 damage to our boat so we should avoid them.
    float NextToMine(const ShipVec &s, const vector<Mine> &mines, const vector<Cannonball> &cbs)
    {
        float ret_val = 0.0f;
        // cerr << endl;
        vector<Cube> spots = TranslateAdjacentHexes(s);
        for (unsigned int i = 0; i < spots.size(); ++i)
        {
            // cerr << spots[i].Xo << "," << spots[i].Yo << endl;
            for (unsigned int j = 0; j < mines.size(); ++j)
                if (spots[i] == mines[j].loc)
                    for (unsigned int k = 0; k < cbs.size(); ++k)
                        if (cbs[k].impact < 2 && cbs[k].loc == spots[i])
                            ret_val += -10.0f;
        }
        return ret_val;
    }


    float BarrelFitnessBoost(const ShipVec &s, const vector<Barrel> &barrels)
    {
        float ret_val = 0.0f;
        Cube bow = s.loc;
        InFront(bow, s.dir);
        for (unsigned int i = 0; i < barrels.size(); ++i)
            ret_val += 50.0f / float(barrels[i].rum) / max(1.0f, ManhattanDist(bow, barrels[i].loc));

        return ret_val;
    }

    float OnShip(const Cube &center, const int &dir, const vector<Ship> &my_ships, const vector<Ship> &en_ships, int id)
    {
        Cube stern = center;
        InFront(stern, (dir + 3) % 6);
        Cube bow = center;
        InFront(bow, dir);

        float ret_val = 0.0f;
        for (unsigned int i = 0; i < my_ships.size(); ++i)
        {
            if (my_ships[i].id != id)
            {
                Cube en_center = my_ships[i].vec.loc;
                Cube en_stern = en_center;
                InFront(en_stern, (my_ships[i].vec.dir + 3) % 6);
                Cube en_bow = en_center;
                InFront(en_bow, my_ships[i].vec.dir);
                // Assumes I'm not trying to box an enemy in
                if (stern == en_stern || stern == en_center || stern == en_bow)
                    ret_val -= 50.0f;
                if (center == en_stern || center == en_center || center == en_bow)
                    ret_val -= 50.0f;
                if (bow == en_stern || bow == en_center || bow == en_bow)
                    ret_val -= 50.0f;
            }
        }

        for (unsigned int i = 0; i < en_ships.size(); ++i)
        {
            if (en_ships[i].id != id)
            {
                Cube en_center = en_ships[i].vec.loc;
                Cube en_stern = en_center;
                InFront(en_stern, (en_ships[i].vec.dir + 3) % 6);
                Cube en_bow = en_center;
                InFront(en_bow, en_ships[i].vec.dir);
                // Assumes I'm not trying to ram
                if (stern == en_stern || stern == en_center || stern == en_bow)
                    ret_val -= 50.0f;
                if (center == en_stern || center == en_center || center == en_bow)
                    ret_val -= 50.0f;
                if (bow == en_stern || bow == en_center || bow == en_bow)
                    ret_val -= 50.0f;
            }
        }

        return ret_val;
    }


    float ShipBuffer(const ShipVec &s, const vector<Ship> &my_ships, const vector<Ship> &en_ships, int id)
    {
        Cube stern = s.loc;
        InFront(stern, (s.dir + 3) % 6);
        Cube bow = s.loc;
        InFront(bow, s.dir);
        vector<Cube> buffer_spots = TranslateAdjacentHexes(s);

        float ret_val = 0.0f;


        for (unsigned int j = 0; j < my_ships.size(); ++j)
        {
            if (my_ships[j].id != id)
            {
                Cube en_center = my_ships[j].vec.loc;
                Cube en_stern = en_center;
                InFront(en_stern, (my_ships[j].vec.dir + 3) % 6);
                Cube en_bow = en_center;
                InFront(en_bow, s.dir);
                for (unsigned int i = 0; i < buffer_spots.size(); ++i)
                    if (buffer_spots[i] == en_stern || buffer_spots[i] == en_center || buffer_spots[i] == en_bow)
                        ret_val -= 10.0f;
            }
        }

        for (unsigned int j = 0; j < en_ships.size(); ++j)
        {
            if (en_ships[j].id != id)
            {
                Cube en_center = en_ships[j].vec.loc;
                Cube en_stern = en_center;
                InFront(en_stern, (en_ships[j].vec.dir + 3) % 6);
                Cube en_bow = en_center;
                InFront(en_bow, s.dir);

                for (unsigned int i = 0; i < buffer_spots.size(); ++i)
                    if (buffer_spots[i] == en_stern || buffer_spots[i] == en_center || buffer_spots[i] == en_bow)
                        ret_val -= 10.0f;
            }
        }

        return ret_val;
    }


};
vector<Ship> _my_ships;
vector<Ship> _en_ships;


class FitnessEvolution
{
public:
    FitnessEvolution()
    {

    }

    void FillShips()
    {
        my_ships = _my_ships;
        en_ships = _en_ships;
        barrels = _barrels;
        cbs = _cbs;
        mines = _mines;
        sim_turn = _turn;
        for (unsigned int j = 0; j < my_ships.size(); ++j)
        {
            vector<Action> ship_actions(5);
            my_ship_moves.emplace_back(ship_actions);
        }
        for (unsigned int j = 0; j < en_ships.size(); ++j)
        {
            vector<Action> ship_actions(5);
            en_ship_moves.emplace_back(ship_actions);
        }
        // for (unsigned int i = 0; i < 5; ++i)
        {
            for (unsigned int j = 0; j < my_ships.size(); ++j)
            {
                vector<Cannonball> cbs_my_ship = cbs;
                my_ships[j].FillActions(sim_turn, my_ships, en_ships, mines, cbs_my_ship, barrels);
                // my_ship_moves[j][0] = my_ships[j].InitialAction();
                vector<Action> sim_ship_turn;
                make_heap(sim_ship_turn.begin(), sim_ship_turn.end());

                AdvanceCannonBalls(cbs_my_ship);
                // Take top 10% of this ship's actions
                // cerr << "ship action size: " << my_ships[j].actions.size() << endl;
                for (unsigned int i = 0; i < my_ships[j].actions.size(); ++i)// * .1 + 1; ++i)
                {
                    Ship sim_ship(my_ships[j], my_ships[j].actions.front());

                    // Build all possible actions and rank them by fitness
                    sim_ship.FillActions(sim_turn + 1, my_ships, en_ships, mines, cbs_my_ship, barrels);

                    // Take only top 1% (+1) and add them to sim_ship_turn's actions
                    for (unsigned int k = 0; k < sim_ship.actions.size(); ++k)// * .01 + 1; ++k)
                    {
                        sim_ship_turn.push_back(sim_ship.actions.front());
                        push_heap(sim_ship_turn.begin(), sim_ship_turn.end());
                        pop_heap(sim_ship.actions.begin(), sim_ship.actions.end());
                        sim_ship.actions.pop_back();
                    }
                    pop_heap(my_ships[j].actions.begin(), my_ships[j].actions.end());
                    my_ships[j].actions.pop_back();
                }

                my_ship_moves[j][0] = sim_ship_turn.front().prev_actions[0];
                my_ships[j].best_action = sim_ship_turn.front().prev_actions[0];

                // sim_ship_turn now contains the best fitness in heap order for turn 1 and 2


                // for (unsigned int j = 0; j < en_ships.size(); ++j)
                // {
                //     en_ships[j].FillActions(sim_turn);
                //     en_ship_moves.at(j).at(0) = en_ships[j].InitialAction();

                // }

            }
        }
    }







    vector<Action> sim_my_ships;

    int sim_turn;
    vector<Ship> my_ships;
    vector<Ship> en_ships;

    vector<Barrel> barrels;
    vector<Cannonball> cbs;
    vector<Mine> mines;

    vector<vector<Action> > my_ship_moves;
    vector<vector<Action> > en_ship_moves;
    // Generate the random arrangements but save unique generations of shot vectors


};

















int main()
{
    _turn = 0;
    BuildShotTemplate();
    BuildMineCheckTemplate();
    FitnessEvolution genetic_algo;

    while (1) {
        vector<Ship> my_ships, en_ships;
        vector<Cannonball> cbs;
        vector<Barrel> barrels;
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
                Ship new_ship(x, y, entityId, arg1, arg3, arg2);
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
                            my_ships.push_back(_my_ships[j]);
                        }
                    }
                    if (!found)
                        my_ships.push_back(new_ship);


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
                            en_ships.push_back(_en_ships[j]);
                        }
                    }
                    if (!found)
                        en_ships.push_back(new_ship);
                }
            }
            else if (entityType == "BARREL")
            {
                for (unsigned int j = 0; j < _barrels.size(); ++j)
                {
                    if (_barrels[j].id == entityId)
                    {
                        found = true;
                        barrels.push_back(_barrels[j]);
                    }
                }

                if (!found)
                    barrels.emplace_back(x, y, arg1, entityId);
            }
            else if (entityType == "CANNONBALL")
            {
                for (unsigned int j = 0; j < _cbs.size(); ++j)
                {
                    if (_cbs[j].id == entityId)
                    {
                        found = true;
                        _cbs[j].impact = arg2;
                        cbs.push_back(_cbs[j]);
                    }
                }

                if (!found)
                {
                    // Update ship fired_last now or later???
                    cbs.emplace_back(x, y, arg2, _turn, entityId);
                }
            }
            else if (entityType == "MINE")
            {
                for (unsigned int j = 0; j < _mines.size(); ++j)
                    if (_mines[j].id == entityId)
                        found = true;

                if (!found)
                    _mines.emplace_back(x, y, entityId);
            }

        }
        swap(my_ships, _my_ships);
        swap(en_ships, _en_ships);
        swap(cbs, _cbs);
        swap(barrels, _barrels);
        float I_should_have_a_better_solution = 0.0f;
        for (unsigned int i = 0; i < _my_ships.size(); ++i)
            _my_ships[i].vec.speed =_my_ships[i].ShipInFront(_my_ships[i].vec, _my_ships, _en_ships, I_should_have_a_better_solution);
        

        // Need function to update _mines

        genetic_algo.FillShips();
        for (int i = 0; i < genetic_algo.my_ships.size(); i++)
        {
            cerr << "fitness: " << genetic_algo.my_ship_moves[i][0].fitness << endl;
            if (genetic_algo.my_ship_moves[i][0].opt == Option::FIRE)
            {
                cout << "FIRE " << genetic_algo.my_ship_moves[i][0].action_loc.Xo << " " << genetic_algo.my_ship_moves[i][0].action_loc.Yo << endl;
                _my_ships[i].fired_last = _turn;
            }
            else if (genetic_algo.my_ship_moves[i][0].opt == Option::MINE)
            {
                cout << "MINE" << endl;
                _my_ships[i].mine_dropped = _turn;
            }
            else if (genetic_algo.my_ship_moves[i][0].opt == Option::PORT)
                cout << "PORT" << endl;
            else if (genetic_algo.my_ship_moves[i][0].opt == Option::STARBOARD)
                cout << "STARBOARD" << endl;
            else if (genetic_algo.my_ship_moves[i][0].opt == Option::FASTER)
                cout << "FASTER" << endl;
            else if (genetic_algo.my_ship_moves[i][0].opt == Option::SLOWER)
                cout << "SLOWER" << endl;
            else
                cout << "WAIT" << endl;

            _my_ships[i].actions.clear();
        }
        ++_turn;
    }



    return 0;
}

/*
    Fills the vector of all hexes within a radius of a point. Generates 331
    Cubes (shot locations)
*/
void BuildShotTemplate()
{
    for (int i = -10; i <= 10; ++i)
        for (int j = -10; j <= 10; ++j)
            for (int k = -10; k <= 10; ++k)
                if (i + j + k == 0)
                    _shot_template.emplace_back(i, j, k);
}

void BuildTargetTemplate()
{
    for (int i = -3; i <= 3; ++i)
        for (int j = -3; j <= 3; ++j)
            for (int k = -3; k <= 3; ++k)
                if (i + j + k == 0)
                    _target_template.emplace_back(i, j, k);
}

void BuildMineCheckTemplate()
{
    for (int i = -1; i <= 1; ++i)
        for (int j = -1; j <= 1; ++j)
            for (int k = -1; k <= 1; ++k)
                if (i + j + k == 0)
                    _mine_check_template.emplace_back(i, j, k);
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
    c.Xo = c.x + (c.z - (c.z & 1)) / 2;
    c.Yo = c.z;
}

int Quadrant(const Cube &a, const Cube &b)
{

    int dx = b.x - a.x;
    int dy = b.y - a.y;
    int dz = b.z - a.z;

    if (dz <= 0 && dy < 0)
        return 0;
    else if (dy >= 0 && dx > 0)
        return 1;
    else if (dx <= 0 && dz < 0)
        return 2;
    else if (dz >= 0 && dy > 0)
        return 3;
    else if (dy <= 0 && dx < 0)
        return 4;
    else
        return 5;
}


// Finds any barrels that will be hit this turn and returns that cumulative value
// Has to take a custom Barrel vector to represent where barrels are for this specific simulation
// Probabaly unecessary to pass the vector as this would only change with ship destruction.
float OnBarrel(const Cube &center, const int &dir, const vector<Barrel> &barrels)
{
    Cube stern = center;
    InFront(stern, (dir + 3) % 6);
    Cube bow = center;
    InFront(bow, dir);

    float ret_val = 0.0;
    for (unsigned int i = 0; i < barrels.size(); ++i)
        if (barrels[i].loc == bow || barrels[i].loc == center || barrels[i].loc == stern)
            ret_val += float(barrels[i].rum);
    return ret_val;
}


// Finds any mines that will be hit this turn and returns that cumulative value
// Has to take a custom Mine vector to represent where mines are for this specific simulation
float OnMine(const ShipVec &s, const vector<Mine> &mines)
{
    Cube stern = s.loc;
    InFront(stern, (s.dir + 3) % 6);
    Cube bow = s.loc;
    InFront(bow, s.dir);

    float ret_val = 0.0;
    for (unsigned int i = 0; i < mines.size(); ++i)
    {
        if (mines[i].loc == bow)
            ret_val -= 25.0;
        else if (mines[i].loc == s.loc)
            ret_val -= 25.0;
        else if (mines[i].loc == stern)
            ret_val -= 25.0;

    }
    return ret_val;
}

float MineInFront(const ShipVec &s, const vector<Mine> &mines)
{
    float ret_val = 0.0;
    Cube bow = s.loc;
    InFront(bow, s.dir);
    InFront(bow, s.dir);
    // Deal with mines in front
    if (s.speed == 1)
    {
        for (unsigned int i = 0; i < mines.size(); ++i)
            if (mines[i].loc == bow)
                ret_val -= 25;
    }
    else if (s.speed == 2)
    {
        Cube bow2 = bow;
        InFront(bow2, s.dir);
        for (unsigned int i = 0; i < mines.size(); ++i)
            if (mines[i].loc == bow || mines[i].loc == bow2)
                ret_val -= 25;
    }
    return ret_val;
}


// Finds any cannonball that will hit this turn and returns that cumulative value
// Has to take a custom Cannonball vector to represent where cannonballs are for this specific simulation
float OnCannonball(const ShipVec &s, const vector<Cannonball> &cbs)
{
    Cube stern = s.loc;
    InFront(stern, (s.dir + 3) % 6);
    Cube bow = s.loc;
    InFront(bow, s.dir);
    // cerr << bow.Xo << "," << bow.Yo << "\t" << s.loc.Xo << "," << s.loc.Yo << "\t" << stern.Xo << "," << stern.Yo << "\t";
    float ret_val = 0.0f;
    for (unsigned int i = 0; i < cbs.size(); ++i)
    {
        if (s.speed == 0 && cbs[i].impact < 3)
        {
            if (cbs[i].loc == bow)
                ret_val -= 25.0f;
            else if (cbs[i].loc == s.loc)
                ret_val -= 50.0f;
            else if (cbs[i].loc == stern)
                ret_val -= 25.0f;
        }
        else if (s.speed > 0 && cbs[i].impact < 2)
        {
            if (cbs[i].loc == bow)
                ret_val -= 25.0f;
            else if (cbs[i].loc == s.loc)
                ret_val -= 50.0f;
            else if (cbs[i].loc == stern)
                ret_val -= 25.0f;
        }
    }
    return ret_val;
}


// I consider edges to be relatively unsafe since it limits your movement options
// Collisions with the edge put you at a stop and could incur a 50 rum hit
float OnEdge(const Cube &center, const int &dir)
{
    Cube bow = center;
    InFront(bow, dir);

    float ret_val = 0.0;
    if (bow.Xo < 4 || bow.Xo > 18 || bow.Yo < 4 || bow.Yo > 16)
    {
        ret_val -= 2;
        if (bow.Xo < 3 || bow.Xo > 19 || bow.Yo < 3 || bow.Yo > 17)
        {
            ret_val -= 4;
            if (bow.Xo < 2 || bow.Xo > 20 || bow.Yo < 2 || bow.Yo > 18)
            {
                ret_val -= 8;
                if (bow.Xo < 0 || bow.Xo > 22 || bow.Yo < 0 || bow.Yo > 20)
                    ret_val -= 25;
            }
        }

    }
    if (center.Xo == 0 || center.Xo == 22 || center.Yo == 0 || center.Yo == 20)
        ret_val -= 5;
    // Collision with edge
    if (center.Xo < 0 || center.Xo > 22 || center.Yo < 0 || center.Yo > 20)
        ret_val -= 50;

    return ret_val;
}

float ManhattanDist(const Cube &origin, const Cube &dest)
{
    return (abs(origin.x - dest.x) + abs(origin.y - dest.y) + abs(origin.z - dest.z)) / 2.0;
}







