# ABSTRACT
This project is part of the EDA (Estructures de Dades i Algorismes, <em>Data Structures and Algorithms</em>) subject belonging to the GEI's (Grau en Enginyeria Informàtica, <em>Bachelor Degree in Informatics Engineering</em>) curriculum at UPC (Universitat Politècnica de Catalunya). The present assignment consisted of coding an AI to play against other student's codes in a faculty-provided 2D strategy game.

An exteded explanation of the game's operation and the player's developement requirements is to be found in the `game.pdf` file. For the logisitcs of the held comptetition refer to the also provided `logistics.pdf`, though it might not be of much use to third parties. Lastly, my final player code (the one used to compete) is contained in the `AIrufus.cc` source file.
- [ABSTRACT](#abstract)
- [STRATEGY DESIGN OVERVIEW](#strategy-design-overview)
  - [Step 1 - Global evaluation: A* pathfinding to estimated closest objective](#step-1---global-evaluation-a-pathfinding-to-estimated-closest-objective)
  - [Step 2 - Local evaluation: BFS evaluating visited cells](#step-2---local-evaluation-bfs-evaluating-visited-cells)
  - [Step 3 - Combining global and local evaluations: directions evaluation](#step-3---combining-global-and-local-evaluations-directions-evaluation)
# STRATEGY DESIGN OVERVIEW
Follows a high-level view of the steps taken to direct each and every one of my player's units, at each round. Refer to the source code for further details.
## Step 1 - Global evaluation: A* pathfinding to estimated closest objective
Use A* algorithm to find path to an estimated closest city or path objective. Already owned cities or paths are considered if constant parameter `GLOBAL_OWNED_CITIES` is set to `true`. Both the closest estimation and the A* heuristic use a plain <em>manhattan distance</em> calculation.

Initial directions (or moves) leading to a closest path are added a certain value to their corresponding evaluations.
## Step 2 - Local evaluation: BFS evaluating visited cells
Perform a local (starting at each unit's location) Breadth-First Search of maximum range defined by constant parameter `LOCAL_BFS_RANGE`. Visited cells are evaluated using `cellEvaluation`, which takes in the distance from the starting unit's position, the unit data structure and the cell data structure, among others.

Initial directions (or moves) leading to a closest path to the evaluated cell are added the corresponding evaluation values.
## Step 3 - Combining global and local evaluations: directions evaluation
Every evaluation function (local and global) adds a certain, predefined constant value (or function of constants and dynamic values, such as distance) to a total evaluation for each direction. The move to make is decided by taking the direction with maximum evaluation.