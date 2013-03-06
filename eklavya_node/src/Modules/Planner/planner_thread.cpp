#include <stdio.h>
#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include "../../eklavya2.h"
#include "planner.h"

char** local_map;

void *planner_thread(void *arg) {
  int iterations = 0;
  double heading;
  Triplet my_bot_location;
  Triplet my_target_location;
  
  int argc;
  char *argv[0];
  
  local_map = new char*[MAP_MAX];
  for (int i = 0; i < MAP_MAX; i++) {
    local_map[i] = new char[MAP_MAX];
  }
  
  //ros::init(argc, argv, "planner");
  //ros::NodeHandle n;
  
  //ros::Publisher planner_pub = n.advertise<geometry_msgs::Twist>("cmd_vel", 1000);
  
  printf("Initiating Planner\n");
  planner_space::Planner::loadPlanner();
  printf("\tPlanner Initiated\n");

  time_t start = time(0);

  while(1) {
    if(iterations > 1000) {
      time_t finish = time(0);
      double fps = (iterations + 0.0) / (finish - start);
      cout << "[INFO] FPS: " << fps << endl;
      break;
    }
    
    pthread_mutex_lock(&bot_location_mutex);
    my_bot_location = bot_location; // Bot
    pthread_mutex_unlock(&bot_location_mutex);
    
    my_bot_location.x = 500; my_bot_location.y = 100; my_bot_location.z = 90;
    pthread_mutex_lock(&target_location_mutex);
    my_target_location = target_location; // Target
    pthread_mutex_unlock(&target_location_mutex);
    
    srand(rand() * time(0));
    double randx = 800; 
    double randy = 800; 
    
    //randx = 100 + rand() % 800; randy = 100 + rand() % 800;
    
    my_target_location.x = randx; my_target_location.y = randy; my_target_location.z = 90;    

    pthread_mutex_lock(&map_mutex);
    for(int i = 0; i < MAP_MAX; i++) {
      for(int j = 0; j < MAP_MAX; j++) {
        local_map[i][j] = global_map[i][j];
      }
    }
    pthread_mutex_unlock(&map_mutex);
    
    planner_space::Planner::findPath(my_bot_location, my_target_location);
    
    iterations++;
  }
}

