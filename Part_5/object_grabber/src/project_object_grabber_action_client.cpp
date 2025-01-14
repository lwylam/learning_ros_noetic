// example_object_grabber_action_client: minimalist client
// wsn, November, 2016
// use with object_grabber action server called "objectGrabberActionServer"
// in file object_grabber_as.cpp

// 1. client gets gripper ID from param server
// 2. gets grasp strategy options from manip_properties(gripper_ID,object_ID)
// two primary fncs:
//  **   object_grab(object_id, object_pickup_pose, grasp_strategy, approach_strategy, depart_strategy)
//  **   object_dropoff(object_id, object_destination_pose, grasp_strategy, dropoff_strategy, depart_strategy)
//      
//  Have default args for grasp_strategy, depart_strategy:
//  i)  default: grab from above, approach from above, depart from above. 
//  ii) Grasp Strategy implies a grasp transform--to be used by action service for planning paths
// 
// Coords expressed as object frame w/rt named frame--which must have a kinematic path to system_ref_frame (e.g. base)


#include <ros/ros.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <object_grabber/object_grabberAction.h>
#include <object_manipulation_properties/object_ID_codes.h>
#include <generic_gripper_services/genericGripperInterface.h>

#include <Eigen/Eigen>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <xform_utils/xform_utils.h>

using namespace std;
XformUtils xformUtils; //type conversion utilities

// Globals 
bool g_got_callback = false;
int g_object_grabber_return_code;
actionlib::SimpleActionClient<object_grabber::object_grabberAction> *g_object_grabber_ac_ptr;


void objectGrabberDoneCb(const actionlib::SimpleClientGoalState& state,
                         const object_grabber::object_grabberResultConstPtr& result) {

    ROS_INFO(" objectGrabberDoneCb: server responded with state [%s]", state.toString().c_str());
    
    g_object_grabber_return_code = result->return_code;
    ROS_INFO("got result output = %d; ", g_object_grabber_return_code);
    
    g_got_callback=true; //flag that action server has called back
}


//test fnc to specify object pick-up and drop-off frames;
//should get pick-up frame from perception, and drop-off frame from perception or task

void set_example_object_frames(geometry_msgs::PoseStamped &object_poseStamped,
                               geometry_msgs::PoseStamped &object_dropoff_poseStamped) {
    //hard code an object pose; later, this will come from perception
    //specify reference frame in which this pose is expressed:
    //will require that "system_ref_frame" is known to tf
    object_poseStamped.header.frame_id = "system_ref_frame"; //set object pose; ref frame must be connected via tf

    object_poseStamped.pose.position.x =  0.5;
    object_poseStamped.pose.position.y = -0.35;
    object_poseStamped.pose.position.z =  0.84;

    object_poseStamped.pose.orientation.x = 0;
    object_poseStamped.pose.orientation.y = 0;
    object_poseStamped.pose.orientation.z = 0;
    object_poseStamped.pose.orientation.w = 1;

    object_poseStamped.header.stamp = ros::Time::now();

    object_dropoff_poseStamped = object_poseStamped; //specify desired drop-off pose of object
    object_dropoff_poseStamped.pose.position.x = 0.7;
    object_dropoff_poseStamped.pose.position.y = 0;
}

void move_to_waiting_pose() {
        ROS_INFO("sending command to move to waiting pose");

        g_got_callback=false; //reset callback-done flag
        
        object_grabber::object_grabberGoal object_grabber_goal;
        object_grabber_goal.action_code = object_grabber::object_grabberGoal::MOVE_TO_WAITING_POSE;
        g_object_grabber_ac_ptr->sendGoal(object_grabber_goal, &objectGrabberDoneCb);
}

void grab_object(geometry_msgs::PoseStamped object_pickup_poseStamped) {
    ROS_INFO("sending a grab-object command");
    g_got_callback=false; //reset callback-done flag
    object_grabber::object_grabberGoal object_grabber_goal;
    object_grabber_goal.action_code  = object_grabber::object_grabberGoal::GRAB_OBJECT;             // Action to be performed 
    object_grabber_goal.object_id    = ObjectIdCodes::COKE_CAN_UPRIGHT;                                 // Obj to manipulate                 
    object_grabber_goal.object_frame = object_pickup_poseStamped;                                   // Obj's current pose
    object_grabber_goal.grasp_option = object_grabber::object_grabberGoal::DEFAULT_GRASP_STRATEGY;  // changed code in action server
    object_grabber_goal.speed_factor = 1.0;
    ROS_INFO("sending goal to grab object: ");
    g_object_grabber_ac_ptr->sendGoal(object_grabber_goal, &objectGrabberDoneCb);
}

void   dropoff_object(geometry_msgs::PoseStamped object_dropoff_poseStamped) {
    ROS_INFO("sending a dropoff-object command");
    object_grabber::object_grabberGoal object_grabber_goal;
    object_grabber_goal.action_code  = object_grabber::object_grabberGoal::DROPOFF_OBJECT;          // Action to be performed 
    object_grabber_goal.object_id    = ObjectIdCodes::COKE_CAN_UPRIGHT;                                 // Obj to manipulate                
    object_grabber_goal.object_frame = object_dropoff_poseStamped;                                  // Obj's current pose
    object_grabber_goal.grasp_option = object_grabber::object_grabberGoal::DEFAULT_GRASP_STRATEGY;  // changed code in action server
    object_grabber_goal.speed_factor = 1.0;
    ROS_INFO("sending goal to dropoff object: ");
    g_object_grabber_ac_ptr->sendGoal(object_grabber_goal, &objectGrabberDoneCb);
} 

int main(int argc, char** argv) {
    ros::init(argc, argv, "example_object_grabber_action_client");
    ros::NodeHandle nh;

    geometry_msgs::PoseStamped object_pickup_poseStamped;
    geometry_msgs::PoseStamped object_dropoff_poseStamped;
    
    // 1. Specify object PICK-UP and DROP-OFF frames using simple test fnc
    //more generally, pick-up comes from perception and drop-off comes from task
    set_example_object_frames(object_pickup_poseStamped, object_dropoff_poseStamped);

    //2. Instantiate an action client of object_grabber_action_service:
    actionlib::SimpleActionClient<object_grabber::object_grabberAction> object_grabber_ac("object_grabber_action_service", true);
    g_object_grabber_ac_ptr = &object_grabber_ac; // make available to fncs

    ROS_INFO("waiting for server: ");
    bool server_exists = false;
    while ((!server_exists)&&(ros::ok())) {
        server_exists = object_grabber_ac.waitForServer(ros::Duration(0.5)); // 
        ros::spinOnce();
        ros::Duration(0.5).sleep();
        ROS_INFO("retrying...");
    }
    ROS_INFO("connected to object_grabber action server"); // if here, then we connected to the server; 

    // 3. Move to WAITING pose
    move_to_waiting_pose();
    while(!g_got_callback) {
        ROS_INFO("waiting on move...");
        ros::Duration(0.5).sleep(); //could do something useful
    }

    // 4. Grab
    grab_object(object_pickup_poseStamped);
    while(!g_got_callback) {
        ROS_INFO("waiting on grab...");
        ros::Duration(0.5).sleep(); //could do something useful
    }    

    // 5. Move to DROP-OFF
    dropoff_object(object_dropoff_poseStamped);
    while(!g_got_callback) {
        ROS_INFO("waiting on dropoff...");
        ros::Duration(0.5).sleep(); //could do something useful
    }

    // // 6. Return to WAITING pose
    // move_to_waiting_pose();
    // while(!g_got_callback) {
    //     ROS_INFO("waiting on move...");
    //     ros::Duration(0.5).sleep(); //could do something useful
    // }
    return 0;
}
