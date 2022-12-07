#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// A structure to represent a node in the grid
struct Node
{
  // x and y coordinates of the node
  int x, y;

  // distance from the source
  int f, g, h;
};

// A utility function to check if the given point is within the boundaries
// of the grid
bool isValid(int x, int y, int n)
{
  if (x < 0 || x >= n || y < 0 || y >= n)
    return false;
  return true;
}

// A utility function to calculate the 'h' heuristics.
// The heuristics is the Manhattan distance from the current cell to the
// destination cell
int calculateHValue(int x, int y, int destX, int destY)
{
  // Return using the distance formula
  return abs(x - destX) + abs(y - destY);
}

// A utility function to find the index of the node having minimum f value
// in the open list
int getMinFValueNode(struct Node *openList, int openListSize)
{
  int minValue = openList[0].f;
  int minValueIndex = 0;
  for (int i = 0; i < openListSize; i++)
  {
    if (openList[i].f < minValue)
    {
      minValue = openList[i].f;
      minValueIndex = i;
    }
  }
  return minValueIndex;
}

// A utility function to trace the path from the source to destination
void tracePath(struct Node *closedList, int closedListSize,
               int destX, int destY)
{
  // Initialize the path array
  int path[closedListSize];
  int pathIndex = closedListSize - 1;
  path[pathIndex--] = closedList[closedListSize - 1].f;

  // Loop through the closed list and store the path
  for (int i = closedListSize - 1; i >= 0; i--)
  {
    if (closedList[i].x == destX && closedList[i].y == destY)
    {
      // Store the path
      for (int j = i; j < closedListSize; j++)
      {
        path[pathIndex--] = closedList[j].f;
      }
      break;
    }
  }

  // Print the path
  printf("The path is: ");
  for (int i = 0; i < closedListSize; i++)
  {
    printf("%d ", path[i]);
  }
  printf("\n");
}

// A utility function to check if the given point is already present in
// the closed list
bool isPointInClosedList(struct Node *closedList, int closedListSize,
                         int x, int y)
{
  for (int i = 0; i < closedListSize; i++)
  {
    if (closedList[i].x == x && closedList[i].y == y)
    {
      return true;
    }
  }
  return false;
}
