#include "navigation.h"

void *navigation_thread(void *arg) {
    int iterations = 0;
    double heading;
    Triplet my_target_location;
    Triplet my_bot_location;

    ros::Rate loop_rate(LOOP_RATE);

    ROS_INFO("Navigation thread started");

    while (ros::ok()) {
        iterations++;
        switch (strategy) {
            case IGVCBasic:
            {
                pthread_mutex_lock(&pose_mutex);
                heading = pose.orientation.z;
                my_target_location = pose.position;
                pthread_mutex_unlock(&pose_mutex);

                my_target_location = navigation_space::IGVCBasicStrategy::getTargetLocation(
                        my_target_location.x,
                        my_target_location.y,
                        heading);

                pthread_mutex_lock(&target_location_mutex);
                target_location = my_target_location;
                pthread_mutex_unlock(&target_location_mutex);

                my_bot_location = navigation_space::IGVCBasicStrategy::getBotLocation();
                pthread_mutex_lock(&bot_location_mutex);
                bot_location = my_bot_location;
                pthread_mutex_unlock(&bot_location_mutex);

                break;
            }

            case FollowNose:
            {
                pthread_mutex_lock(&pose_mutex);
                heading = pose.orientation.z;
                pthread_mutex_unlock(&pose_mutex);

                navigation_space::FollowNoseStrategy::calibrateReferenceHeading(heading, iterations);

                if (iterations < 5) {
                    break;
                }

                my_target_location = navigation_space::FollowNoseStrategy::getTargetLocation(heading);
                pthread_mutex_lock(&target_location_mutex);
                target_location = my_target_location;
                pthread_mutex_unlock(&target_location_mutex);

                my_bot_location = navigation_space::FollowNoseStrategy::getBotLocation();
                pthread_mutex_lock(&bot_location_mutex);
                bot_location = my_bot_location;
                pthread_mutex_unlock(&bot_location_mutex);

                break;
            }

            case TrackWayPoint:
            {
                pthread_mutex_lock(&pose_mutex);
                heading = pose.orientation.z;
                my_target_location = pose.position;
                pthread_mutex_unlock(&pose_mutex);

                my_target_location = navigation_space::TrackWayPointStrategy::getTargetLocation(
                        my_target_location.x,
                        my_target_location.y,
                        heading);

                pthread_mutex_lock(&target_location_mutex);
                target_location = my_target_location;
                pthread_mutex_unlock(&target_location_mutex);

                my_bot_location = navigation_space::TrackWayPointStrategy::getBotLocation();
                pthread_mutex_lock(&bot_location_mutex);
                bot_location = my_bot_location;
                pthread_mutex_unlock(&bot_location_mutex);

                break;
            }

            case LaserTestOnly:
            case FusionTestOnly:
            case PlannerTestOnly:
            {
                pthread_mutex_lock(&target_location_mutex);
                my_target_location.x = target_location.x = 500;
                my_target_location.y = target_location.y = 900;
                my_target_location.z = target_location.z = 90;
                pthread_mutex_unlock(&target_location_mutex);

                pthread_mutex_lock(&bot_location_mutex);
                bot_location.x = 500;
                bot_location.y = 100;
                bot_location.z = 90;
                pthread_mutex_unlock(&bot_location_mutex);
            }
        }

        ROS_DEBUG("[NAV] [TARGET] : (%d, %d)", my_target_location.x, my_target_location.y);

        loop_rate.sleep();
    }

    ROS_INFO("Navigation Exited");

    return NULL;
}

