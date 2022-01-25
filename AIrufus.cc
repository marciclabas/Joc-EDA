#include "Player.hh"

#include <queue>
#include <map>
#include <array>
#include <functional>
#include <cmath>

#define INT_INFINITY 99999999

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME rufus

struct PLAYER_NAME : public Player {

  static Player* factory () {
      return new PLAYER_NAME;
  }

  class DirectionEvaluation {
    array<double, 4> evaluation;
  public:
    DirectionEvaluation();
    DirectionEvaluation(double all);
    DirectionEvaluation(function<double(const Dir)> evaluationFunction);
    DirectionEvaluation operator+(const DirectionEvaluation & other) const;
    void operator+=(const DirectionEvaluation & other);
    double & operator[](const Dir & direction);
    double operator[](const Dir & direction) const;
  };


  class DirectionBooleans {
    array<bool, 4> boolean;
  public:
    DirectionBooleans();
    DirectionBooleans(bool all);
    DirectionBooleans(Dir directionTrue);
    void operator+=(const DirectionBooleans & other);
    bool operator[](const Dir & direction) const;
  };

  struct NullPos : Pos {
    NullPos(): Pos(-1, -1) {}
  };

  bool isNull(const Pos & p) const;

  // constant parameters
  const double NULL_EVALUATION; // by-default evaluation of moving in any direction, i.e., of not staying still
  const double LOCAL_BFS_RANGE; // range of BFS performed locally (from every unit)
  const double MASK_EVALUATION; // evaluation of mask
  const double MASK_EVALUATION_IF_INFECTED; // idem but if the unit is infected
  const double VIRUS_EVALUATION; // evaluation of virus
  const double VIRUS_EVALUATION_IF_INFECTED; // idem but if the unit is infected
  const double ENEMY_UNIT_EVALUATION; // unit & enemy not infected
  const double INFECTED_ENEMY_UNIT_EVALUATION; // unit not infected; enemy infected
  const double ENEMY_UNIT_EVALUATION_IF_INFECTED; // unit infected; enemy not infected
  const double INFECTED_ENEMY_UNIT_EVALUATION_IF_INFECTED; // unit & enemy infected
  const double ALLIED_UNIT_EVALUATION; // unit & ally not infected
  const double INFECTED_ALLIED_UNIT_EVALUATION; // unit not infected; ally infected
  const double ALLIED_UNIT_EVALUATION_IF_INFECTED; // unit infected; ally not infected
  const double INFECTED_ALLIED_UNIT_EVALUATION_IF_INFECTED; // unit & ally infected
  const double LOCAL_CITY_EVALUATION; // evaluation of close-by city
  const double LOCAL_CITY_EVALUATION_IF_INFECTED;
  const double LOCAL_PATH_EVALUATION; // evaluation of close-by path
  const double LOCAL_PATH_EVALUATION_IF_INFECTED;
  const double LOCAL_WALL_EVALUATION; // evaluation of close-by path
  const double LOCAL_WALL_EVALUATION_IF_INFECTED;
  const double GLOBAL_CITY_OR_PATH_EVALUATION;
  const double ADJACENT_WALL_EVALUATION;
  const double ADJACENT_ENEMY_EVALUATION;
  const double ADJACENT_ALLIED_EVALUATION;
  const bool GLOBAL_OWNED_CITIES;

  // cell mask evaluation functions
  double MASK_EVALUATION_FUNCTION(const int distance, const Unit & myUnit) const;
  
  // cell virus evaluation functions
  double VIRUS_EVALUATION_FUNCTION(const int distance, const int virus, const Unit & myUnit) const;
  
  // cell units evaluation functions
  double ENEMY_UNIT_EVALUATION_FUNCTION(const int distance, const Unit & myUnit, const Unit & otherUnit, const int foundAlliesCounter, const int foundEnemiesCounter) const;
  double ALLIED_UNIT_EVALUATION_FUNCTION(const int distance, const Unit & myUnit, const Unit & otherUnit, const int foundAlliesCounter, const int foundEnemiesCounter) const;
  double ADJACENT_COMBAT_EVALUATION(const double EVALUATION_CONSTANT, const Unit & myUnit, const Unit & enemyUnit) const;

  double LOCAL_UNIT_EVALUATION_FUNCTION(const int distance, const Unit & myUnit, const Unit & otherUnit, int & foundAlliesCounter, int & foundEnemiesCounter) const;
  
  // cell type evaluation functions
  double LOCAL_CITY_EVALUATION_FUNCTION(const int distance, const Unit & myUnit) const;
  double LOCAL_PATH_EVALUATION_FUNCTION(const int distance, const Unit & myUnit) const;
  double LOCAL_WALL_EVALUATION_FUNCTION(const int distance, const Unit & myUnit) const;

  double LOCAL_CELLTYPE_EVALUATION_FUNCTION(const int distance, const Unit & myUnit, const CellType & cellType) const;

  DirectionEvaluation cellEvaluation(const int distance, const Cell & cell, const Unit & unit, const DirectionBooleans & reachableFrom, int & foundAlliesCounter, int & foundEnemiesCounter) const;

  const vector<Dir> & POSSIBLE_DIRECTIONS() const;

  PLAYER_NAME();

  Pos closestCityOrPathEstimation(const Pos & source) const;
  DirectionBooleans shortestPathDirections(const Pos & source, const Pos & destination) const;
  
  DirectionEvaluation localEvaluation(const Unit & myUnit, const int RANGE) const;
  DirectionEvaluation globalEvaluation(const Unit & myUnit) const;
  Dir chosenDirection(const DirectionEvaluation & evaluation);
  void play() override;
};

void PLAYER_NAME::play() {
  const double cpuStatus = status(me());
  for(const int unitID : my_units(me())) {
    const Unit u = unit(unitID);
    const DirectionEvaluation evaluation = [&] {
      if(cpuStatus < 0.5 or (cpuStatus < 0.8 and round() > 175))
        return localEvaluation(u, LOCAL_BFS_RANGE) + globalEvaluation(u);
      else if(cpuStatus < 0.9)
        return localEvaluation(u, 2);
      else
        return DirectionEvaluation { NULL_EVALUATION };
    }();
    const Dir direction = chosenDirection(evaluation);
    move(unitID, direction);
  }
}

// BFS search
PLAYER_NAME::DirectionEvaluation PLAYER_NAME::localEvaluation(const Unit & u, const int RANGE) const {
  DirectionEvaluation evaluation = { NULL_EVALUATION };

  int foundAlliesCounter = 0;
  int foundEnemiesCounter = 0;
  
  struct Ticket {
    int distance;
    DirectionBooleans reachableFrom;
  };

  map<Pos, Ticket> visited;
  queue<map<Pos, Ticket>::const_iterator> scheduledAppointments;
  // insert to map: source and cells arround it (if aren't walls)
  visited[u.pos] = { 0, DirectionBooleans(false) };
  for(const Dir direction : POSSIBLE_DIRECTIONS()) {
    const Pos position = u.pos + direction;
    const auto foo = visited.insert({ position, { 1, DirectionBooleans(direction)}}); // would use structured bindings in c++17 :/
    const auto & iteratorPosition = foo.first;
    const bool & beenInserted = foo.second;
    assert(beenInserted); // check it was properly inserted
    // give ticket it not wall or contains enemy
    const Cell c = cell(position);
    const int unitID = c.unit_id;
    if(c.type == CellType::WALL) evaluation[direction] += ADJACENT_WALL_EVALUATION;
    else if(unitID != -1) {
      const Unit foundUnit = unit(unitID);
      const bool alliedUnit = foundUnit.player == me();
      if(alliedUnit) foundAlliesCounter++;
      else foundEnemiesCounter++;
      evaluation[direction] += alliedUnit
        ? ADJACENT_ALLIED_EVALUATION
        : ADJACENT_COMBAT_EVALUATION(ADJACENT_ENEMY_EVALUATION, u, foundUnit);
    }
    else scheduledAppointments.push(iteratorPosition);
  }

  while(not scheduledAppointments.empty()) {
    auto currentlyVisited = scheduledAppointments.front(); scheduledAppointments.pop();
    const Pos & currentPosition = currentlyVisited->first;
    const Ticket & currentTicket = currentlyVisited->second;

    if(currentTicket.distance > RANGE) return evaluation; // before evaluating the current cell, which is the first to surpass the max range

    // visit current cell
    evaluation += cellEvaluation(currentTicket.distance, cell(currentPosition), u, currentTicket.reachableFrom, foundAlliesCounter, foundEnemiesCounter);

    // hand tickets to its neighbours
    for(const Dir currentDirection : POSSIBLE_DIRECTIONS()) {
      const Pos neighbourPosition = currentPosition + currentDirection;
      // if visited
      const auto foundNeighbourPosition = visited.find(neighbourPosition);
      if(foundNeighbourPosition != visited.end()) {
        foundNeighbourPosition->second.reachableFrom += currentTicket.reachableFrom;
      }
      // if not visited
      else {
        const Cell c = cell(neighbourPosition);
        // if not wall, ask ticket
          if(c.type != CellType::WALL) {
          const int distance = currentTicket.distance + 1;
          const DirectionBooleans & reachableFrom = currentTicket.reachableFrom;
          const auto foo = visited.insert({ neighbourPosition, { distance, reachableFrom }});
          const auto insertedNeighbourPosition = foo.first;
          scheduledAppointments.push(insertedNeighbourPosition);
        }
      }
    }
  }
  return evaluation;
}


// returns the maximum direction (if positive) or NONE (if non-positive)
Dir PLAYER_NAME::chosenDirection(const DirectionEvaluation & evaluation) {
  const Dir maxHorizontal = isgreater(evaluation[Dir::LEFT], evaluation[Dir::RIGHT])
    ? Dir::LEFT
    : Dir::RIGHT;

  const Dir maxVertical = isgreater(evaluation[Dir::BOTTOM], evaluation[Dir::TOP])
    ? Dir::BOTTOM
    : Dir::TOP;

  const Dir max = isgreater(evaluation[maxHorizontal], evaluation[maxVertical])
    ? maxHorizontal
    : maxVertical;

  return max;
}

PLAYER_NAME::DirectionEvaluation PLAYER_NAME::cellEvaluation(const int distance, const Cell & c, const Unit & u, const DirectionBooleans & reachableFrom, int & foundAlliesCounter, int & foundEnemiesCounter) const {

  const double cellTypeEvaluation = LOCAL_CELLTYPE_EVALUATION_FUNCTION(distance, u, c.type);
  
  const double cellUnitEvaluation = c.unit_id != -1
    ? LOCAL_UNIT_EVALUATION_FUNCTION(distance, u, unit(c.unit_id), foundAlliesCounter, foundEnemiesCounter)
    : NULL_EVALUATION;
    

  const double cellMaskEvaluation = c.mask
    ? MASK_EVALUATION_FUNCTION(distance, u)
    : NULL_EVALUATION;

  const double cellVirusEvaluation = VIRUS_EVALUATION_FUNCTION(distance, c.virus, u);

  const double cellEvaluation = cellTypeEvaluation + cellUnitEvaluation + cellMaskEvaluation + cellVirusEvaluation;

  return DirectionEvaluation([&](const Dir direction) {
    return reachableFrom[direction]
      ? cellEvaluation
      : NULL_EVALUATION;
  });
}

double PLAYER_NAME::LOCAL_CELLTYPE_EVALUATION_FUNCTION(const int distance, const Unit & myUnit, const CellType & cellType) const {
  switch(cellType) {
      case CellType::CITY: return LOCAL_CITY_EVALUATION_FUNCTION(distance, myUnit);
      case CellType::PATH: return LOCAL_PATH_EVALUATION_FUNCTION(distance, myUnit);
      case CellType::WALL: return LOCAL_WALL_EVALUATION_FUNCTION(distance, myUnit);
      default: return NULL_EVALUATION;
    }
  assert(false);
  return -1;
}

  // cell mask evaluation functions
double PLAYER_NAME::MASK_EVALUATION_FUNCTION(const int distance, const Unit & myUnit) const {
  assert(distance != 0);
  const double maskEvaluation = myUnit.damage > 0
    ? MASK_EVALUATION_IF_INFECTED
    : MASK_EVALUATION;
  return maskEvaluation/pow(distance,3);
}

// cell virus evaluation functions
double PLAYER_NAME::VIRUS_EVALUATION_FUNCTION(const int distance, const int virus, const Unit & myUnit) const {
  assert(distance != 0);
  const double virusEvaluation = myUnit.damage > 0
    ? VIRUS_EVALUATION_IF_INFECTED
    : VIRUS_EVALUATION * virus; // maybe squared ?
  return virusEvaluation/pow(distance,3);
}

double PLAYER_NAME::LOCAL_UNIT_EVALUATION_FUNCTION(const int distance, const Unit & myUnit, const Unit & otherUnit, int & foundAlliesCounter, int & foundEnemiesCounter) const {
  const bool alliedUnit = otherUnit.player == me();
  if(alliedUnit) foundAlliesCounter++;
  else foundEnemiesCounter++;
   return alliedUnit
    ? ALLIED_UNIT_EVALUATION_FUNCTION(distance, myUnit, otherUnit, foundAlliesCounter, foundEnemiesCounter)
    : ENEMY_UNIT_EVALUATION_FUNCTION(distance, myUnit, otherUnit, foundAlliesCounter, foundEnemiesCounter);
}

// cell units evaluation functions
// [TO DO!] implement some evaluation of myUnit and otherUnit's hp!
double PLAYER_NAME::ENEMY_UNIT_EVALUATION_FUNCTION(const int distance, const Unit & myUnit, const Unit & enemyUnit, const int foundAlliesCounter, const int foundEnemiesCounter) const {
  assert(distance > 1);
  const double enemyEvaluation = [&] {
    if(myUnit.damage > 0)
      return enemyUnit.damage > 0
        ? INFECTED_ENEMY_UNIT_EVALUATION_IF_INFECTED
        : ENEMY_UNIT_EVALUATION_IF_INFECTED;
    else
      return enemyUnit.damage > 0
        ? INFECTED_ENEMY_UNIT_EVALUATION
        : ENEMY_UNIT_EVALUATION;
  }();
  const int healthDifference = myUnit.health - enemyUnit.health;
  //return (100 + healthDifference)*enemyEvaluation*pow(foundAlliesCounter,5)/pow(distance,6)/pow(foundEnemiesCounter, 5); // allied/enemies version
  return (100 + healthDifference)*enemyEvaluation*pow(foundAlliesCounter - foundEnemiesCounter,5)/pow(distance,6); // allied - enemies version 
}

double PLAYER_NAME::ADJACENT_COMBAT_EVALUATION(const double EVALUATION_CONSTANT, const Unit & myUnit, const Unit & enemyUnit) const {
    return EVALUATION_CONSTANT;
}

double PLAYER_NAME::ALLIED_UNIT_EVALUATION_FUNCTION(const int distance, const Unit & myUnit, const Unit & alliedUnit, const int foundAlliesCounter, const int foundEnemiesCounter) const {
  assert(distance != 0);
  const double alliedEvaluation = [&] {
    if(myUnit.damage > 0)
      return alliedUnit.damage > 0
        ? INFECTED_ALLIED_UNIT_EVALUATION_IF_INFECTED
        : ALLIED_UNIT_EVALUATION_IF_INFECTED;
    else
      return alliedUnit.damage > 0
        ? INFECTED_ALLIED_UNIT_EVALUATION
        : ALLIED_UNIT_EVALUATION;
  }();
  return alliedEvaluation/pow(foundAlliesCounter,2);
}

// cell type evaluation functions
double PLAYER_NAME::LOCAL_CITY_EVALUATION_FUNCTION(const int distance, const Unit & myUnit) const {
  assert(distance != 0);
  const double cityEvaluation = myUnit.damage > 0
    ? LOCAL_CITY_EVALUATION_IF_INFECTED
    : LOCAL_CITY_EVALUATION;
  return cityEvaluation/pow(distance,2);
}

double PLAYER_NAME::LOCAL_PATH_EVALUATION_FUNCTION(const int distance, const Unit & myUnit) const {
  assert(distance != 0);
  const double pathEvaluation = myUnit.damage > 0
    ? LOCAL_PATH_EVALUATION_IF_INFECTED
    : LOCAL_PATH_EVALUATION;
  return pathEvaluation/pow(distance,2);
}

double PLAYER_NAME::LOCAL_WALL_EVALUATION_FUNCTION(const int distance, const Unit & myUnit) const {
  assert(distance != 0);
  const double wallEvaluation = myUnit.damage > 0
    ? LOCAL_WALL_EVALUATION_IF_INFECTED
    : LOCAL_WALL_EVALUATION;
  return wallEvaluation/pow(distance,3);
}

int manhattanDistance(const Pos & a, const Pos & b) {
  return abs(a.i - b.i) + abs(a.j - b.j);
}

Pos PLAYER_NAME::closestCityOrPathEstimation(const Pos & source) const {
  Pos closestPosition = { -1, -1 };
  int shortestDistance = INT_INFINITY;

  auto updateClosest = [&](int numberOfIDs, std::function<int(int)> ownerID, std::function<vector<Pos>(int)> positions) {
    for(int id = 0; id < numberOfIDs; id++) {
      if(ownerID(id) != me() or GLOBAL_OWNED_CITIES) {
        for(const Pos candidatePosition : positions(id)) {
          const int distanceCandidate = manhattanDistance(source, candidatePosition);
          if(distanceCandidate < shortestDistance) {
            shortestDistance = distanceCandidate;
            closestPosition = candidatePosition;
          }
        }
      }
    }
  };

  updateClosest(nb_cities(), [&](int id) { return city_owner(id); }, [&](int cityID) { return city(cityID); });
  updateClosest(nb_paths(), [&](int id) { return path_owner(id); }, [&](int pathID) { return path(pathID).second; });

  return closestPosition;
}

// implements A* algorithm, with manhattan distance to destination as heuristic
PLAYER_NAME::DirectionBooleans PLAYER_NAME::shortestPathDirections(const Pos & source, const Pos & destination) const {
  struct Ticket {
    int localGoal;
    int globalGoal;
    DirectionBooleans reachableFrom;
    bool visited;
  };

  using PosTicket = map<Pos, Ticket>::iterator;

  struct PosTicketCompare {
      bool operator()(const PosTicket & a, const PosTicket & b) { return a->second.globalGoal > b->second.globalGoal; };
  };

  map<Pos, Ticket> visited;
  priority_queue<PosTicket, vector<PosTicket>, PosTicketCompare> scheduledAppointments;
  // insert to map: source and cells arround it (if aren't walls)
  visited[source] = { 0, manhattanDistance(source, destination), DirectionBooleans(false), true };
  for(const Dir direction : POSSIBLE_DIRECTIONS()) {
    const Pos position = source + direction;
    const auto foo = visited.insert({ position, { 1, 1 + manhattanDistance(position, destination), DirectionBooleans(direction), false}}); // changed me mind: C++ 17 strctured bindings *-*
    const auto & iteratorPosition = foo.first;
    const bool & beenInserted = foo.second;
    assert(beenInserted); // check it was properly inserted
    // give ticket if not wall or contains enemy
    const Cell c = cell(position);
    if(c.type != CellType::WALL and c.unit_id == -1) scheduledAppointments.push(iteratorPosition);
  }

  while(not scheduledAppointments.empty()) {
    PosTicket currentlyVisited = scheduledAppointments.top(); scheduledAppointments.pop();
    const Pos & currentPosition = currentlyVisited->first;
    Ticket & currentTicket = currentlyVisited->second;
    if(currentPosition == destination) return currentTicket.reachableFrom;
    else if(not currentTicket.visited) { // if not visited
      currentTicket.visited = true;

      for(const Dir currentDirection : POSSIBLE_DIRECTIONS()) {
        const Pos neighbourPosition = currentPosition + currentDirection;
        if(cell(neighbourPosition).type == CellType::WALL) {
          visited[neighbourPosition].visited = true; // to avoid searching later
        }
        else  { 
          PosTicket neighbour = visited.find(neighbourPosition);
          // not visited though already initialized (has been neighbour of some visited node)
          // relax
          if(neighbour != visited.end()) {
            const int candidateLocal = currentTicket.localGoal + 1;
            Ticket & neighbourTicket = neighbour->second;
            if(candidateLocal < neighbourTicket.localGoal) {
              neighbourTicket.localGoal = candidateLocal;
              neighbourTicket.globalGoal = candidateLocal + manhattanDistance(neighbourPosition, destination);
              neighbourTicket.reachableFrom = currentTicket.reachableFrom;
            }
            else if(candidateLocal == neighbourTicket.localGoal) {
              neighbourTicket.reachableFrom += currentTicket.reachableFrom;
            }
            scheduledAppointments.push(neighbour);
          }
          else  { // not visited as neighbour
            auto foo = visited.insert({ neighbourPosition, { currentTicket.localGoal + 1, currentTicket.localGoal + 1 + manhattanDistance(neighbourPosition, destination), currentTicket.reachableFrom, false }});
            PosTicket & newNeighbour = foo.first;
            const bool & beenInserted = foo.second;
            assert(beenInserted);
            scheduledAppointments.push(newNeighbour);
          }
        }
      }
    }
    // else we ignore it and keep going
  }
  return visited[destination].reachableFrom;
}

PLAYER_NAME::DirectionEvaluation PLAYER_NAME::globalEvaluation(const Unit & myUnit) const {
  const Pos estimatedClosestCityOrPath = closestCityOrPathEstimation(myUnit.pos);
  const DirectionBooleans directionsToClosestCity = isNull(estimatedClosestCityOrPath)
    ? DirectionBooleans(false)
    : shortestPathDirections(myUnit.pos, estimatedClosestCityOrPath);

  return DirectionEvaluation([&](const Dir direction) {
    return directionsToClosestCity[direction]
      ? GLOBAL_CITY_OR_PATH_EVALUATION
      : NULL_EVALUATION;
  });
}

// DEFAULT PARAMETERS
PLAYER_NAME::PLAYER_NAME():
  NULL_EVALUATION(0),
  LOCAL_BFS_RANGE(25),
  MASK_EVALUATION(50),
  MASK_EVALUATION_IF_INFECTED(0),
  VIRUS_EVALUATION(-20),
  VIRUS_EVALUATION_IF_INFECTED(5),
  ENEMY_UNIT_EVALUATION(25),
  INFECTED_ENEMY_UNIT_EVALUATION(-25),
  ENEMY_UNIT_EVALUATION_IF_INFECTED(30),
  INFECTED_ENEMY_UNIT_EVALUATION_IF_INFECTED(25),
  ALLIED_UNIT_EVALUATION(70),
  INFECTED_ALLIED_UNIT_EVALUATION(-5),
  ALLIED_UNIT_EVALUATION_IF_INFECTED(-20),
  INFECTED_ALLIED_UNIT_EVALUATION_IF_INFECTED(70),
  LOCAL_CITY_EVALUATION(50),
  LOCAL_CITY_EVALUATION_IF_INFECTED(25),
  LOCAL_PATH_EVALUATION(50),
  LOCAL_PATH_EVALUATION_IF_INFECTED(25),
  LOCAL_WALL_EVALUATION(0),
  LOCAL_WALL_EVALUATION_IF_INFECTED(0),
  GLOBAL_CITY_OR_PATH_EVALUATION(70),
  ADJACENT_WALL_EVALUATION(-INFINITY),
  ADJACENT_ENEMY_EVALUATION(INFINITY),
  ADJACENT_ALLIED_EVALUATION(-INFINITY),
  GLOBAL_OWNED_CITIES(false)
{}

const vector<Dir> & PLAYER_NAME::POSSIBLE_DIRECTIONS() const {
  static vector<Dir> directions = { Dir::BOTTOM, Dir::RIGHT, Dir::TOP, Dir::LEFT };
  return directions;
}


// DirectionEvaluation class
PLAYER_NAME::DirectionEvaluation::DirectionEvaluation() {

}

PLAYER_NAME::DirectionEvaluation::DirectionEvaluation(double all) : evaluation{ all, all, all, all } {}

PLAYER_NAME::DirectionEvaluation::DirectionEvaluation(function<double(const Dir)> evaluationFunction)
: evaluation({evaluationFunction(Dir::BOTTOM), evaluationFunction(Dir::RIGHT), evaluationFunction(Dir::TOP), evaluationFunction(Dir::LEFT)}) {}

PLAYER_NAME::DirectionEvaluation PLAYER_NAME::DirectionEvaluation::operator+(const DirectionEvaluation & other) const {
  return DirectionEvaluation([&](const Dir direction) {
    return (*this)[direction] + other[direction];
  });
}

void PLAYER_NAME::DirectionEvaluation::operator+=(const DirectionEvaluation & other) {
  *this = *this + other;
}

double & PLAYER_NAME::DirectionEvaluation::operator[](const Dir & direction) {
  int index = static_cast<int>(direction);
  assert(index >= 0 and index <= 3);
  return evaluation[index];
}

double PLAYER_NAME::DirectionEvaluation::operator[](const Dir & direction) const {
  int index = static_cast<int>(direction);
  assert(index >= 0 and index <= 3);
  return evaluation[index];
}

// DirectionBooleans class
PLAYER_NAME::DirectionBooleans::DirectionBooleans() { 
}

PLAYER_NAME::DirectionBooleans::DirectionBooleans(bool all) : boolean{ all, all, all, all } {}

PLAYER_NAME::DirectionBooleans::DirectionBooleans(Dir directionTrue) : boolean{ false, false, false, false } {
  boolean[directionTrue] = true;
}

void PLAYER_NAME::DirectionBooleans::operator+=(const DirectionBooleans & other) {
  auto thisIt = this->boolean.begin();
  auto otherIt = other.boolean.cbegin();
  while(otherIt != other.boolean.cend()) {
    assert(thisIt != this->boolean.end());
    *thisIt |= *otherIt;
    thisIt++;
    otherIt++;
  }
}

bool PLAYER_NAME::DirectionBooleans::operator[](const Dir & direction) const {
  int index = static_cast<int>(direction);
  assert(index >= 0 and index <= 3);
  return boolean[index];
}

bool PLAYER_NAME::isNull(const Pos & p) const {
  return p.i == -1;
}

/**
 * Do not modify the following lines.
 */
RegisterPlayer(PLAYER_NAME);
