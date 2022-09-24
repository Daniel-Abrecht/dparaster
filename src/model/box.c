#include <dparaster/geometry.h>

const Geometry box = {
  .triangle_count = 12,
  .attribute = {

    [AIN_POSITION] = {
      .vertex = (const Vector[]){
        {{-1,-1,-1, 1}}, {{ 1,-1,-1, 1}}, {{-1, 1,-1, 1}}, {{ 1, 1,-1, 1}},
        {{-1,-1, 1, 1}}, {{ 1,-1, 1, 1}}, {{-1, 1, 1, 1}}, {{ 1, 1, 1, 1}},
      },
      .index = (const unsigned[][3]){
        {1,0,2},  {1,2,3}, // Front
        {3,2,6},  {3,6,7}, // Bottom
        {7,6,4},  {7,4,5}, // Back
        {5,4,0},  {5,0,1}, // Top
        {0,6,2},  {0,4,6}, // Left
        {3,5,1},  {3,7,5}, // Right
      },
    },
//
    [AIN_COLOR] = {
      .vertex = (const Vector[]){
        {{1,0,0,1}}, {{0,1,0,1}}, {{0,0,1,1}},
        {{1,1,0,1}}, {{0,1,1,1}}, {{1,0,1,1}},
      },
/*
      // Flat colors
      .index = (const unsigned[][3]){
        {0,0,0},  {0,0,0}, // Front
        {1,1,1},  {1,1,1}, // Bottom
        {2,2,2},  {2,2,2}, // Back
        {3,3,3},  {3,3,3}, // Top
        {4,4,4},  {4,4,4}, // Left
        {5,5,5},  {5,5,5}, // Right
      }
*/
      // Rainbow colors
      .index = (const unsigned[][3]){
        {1,0,2},  {1,2,3}, // Front
        {1,0,2},  {1,2,3}, // Bottom
        {1,0,2},  {1,2,3}, // Back
        {1,0,2},  {1,2,3}, // Top
        {1,0,2},  {1,3,0}, // Left
        {1,0,2},  {1,3,0}, // Right
      }

    },

    [AIN_TEXCOORD] = {
      .vertex = (const Vector[]){
        {{0,0}}, {{0,1}},
        {{1,0}}, {{1,1}},
      },
      .index = (const unsigned[][3]){
        {1,0,2},  {1,2,3}, // Front
        {1,0,2},  {1,2,3}, // Bottom
        {2,3,1},  {2,1,0}, // Back
        {1,0,2},  {1,2,3}, // Top
        {3,0,2},  {3,1,0}, // Left
        {0,3,1},  {0,2,3}, // Right
      }
    }

  },
};
