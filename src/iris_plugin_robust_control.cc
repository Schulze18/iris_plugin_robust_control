// This plugin is based on the tutorial to control a Velodyne sensor

#ifndef _IRIS_PLUGIN_ROBUST_CONTROL_HH_
#define _IRIS_PLUGIN_ROBUST_CONTROL_HH_

// Include default libraries for Gazebo and ROS interface
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/common/common.hh>
#include <gazebo/math/gzmath.hh>
#include <thread>
#include <iostream>
#include <time.h>
#include <sys/time.h>
#include "ros/ros.h"
#include "ros/callback_queue.h"
#include "ros/subscribe_options.h"
#include <ignition/math/Pose3.hh>
// Include Msgs Types
#include <sensor_msgs/Imu.h> 
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/Vector3.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/Pose.h>
// Basic libraries for C and C++
#include <stddef.h>
#include <stdio.h>
#include <vector>
#include <math.h> 
// Including controller gains
#include "robust_gains.h"
#include "matrix_drque.h"

namespace gazebo
{
  // A plugin to control an Iris 3DR quadrotor.
  class IrisPluginRobustControl : public ModelPlugin
  {
    // Constructor
    public: IrisPluginRobustControl() {}

    // The load function is called by Gazebo when the plugin is inserted into simulation
    // \param[in] _model A pointer to the model that this plugin is attached to.
    // \param[in] _sdf A pointer to the plugin's SDF element.
    public: virtual void Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
    {

	   	// Just output a message for now
	    std::cerr << "\nThe iris plugin is attach to model" << _model->GetName() << "\n";

	   
		// Store the model pointer for convenience.
		this->model = _model;	
		
		// Initialize ros, if it has not already bee initialized.
		if (!ros::isInitialized())
		{
		  int argc = 0;
		  char **argv = NULL;
		  ros::init(argc, argv, "gazebo_client", ros::init_options::NoSigintHandler);
		}

		// Create our ROS node. This acts in a similar manner to the Gazebo node
		this->rosNode.reset(new ros::NodeHandle("gazebo_client"));

		//Create and subscribe to a topic with Quaternion type
		/*ros::SubscribeOptions so = ros::SubscribeOptions::create<geometry_msgs::Point>(
		"/" + this->model->GetName() + "/iris_ref",1,
		boost::bind(&IrisPluginExplicitMPC::OnRosMsg, this, _1), ros::VoidPtr(), &this->rosQueue);
		*/

		//Create and subscribe to a topic with Pose type	
		ros::SubscribeOptions so = ros::SubscribeOptions::create<geometry_msgs::Pose>(
		"/" + this->model->GetName() + "/iris_ref",1,
		boost::bind(&IrisPluginRobustControl::OnRosMsg, this, _1), ros::VoidPtr(), &this->rosQueue);

		// Store the subscriber for convenience.
		this->rosSub = this->rosNode->subscribe(so);

		// Spin up the queue helper thread.
		this->rosQueueThread = std::thread(std::bind(&IrisPluginRobustControl::QueueThread, this));

		// Create a topic to publish iris state
		this->state_pub = this->rosNode->advertise<sensor_msgs::Imu>("iris_state", 100);
		// Create a topic to publish the rotors velocities 
		this->vel_pub = this->rosNode->advertise<geometry_msgs::Quaternion>("vel_cmd", 100);
		
		// Configure Timer and callback function
		this->pubTimer = this->rosNode->createTimer(ros::Duration(0.01), &IrisPluginRobustControl::control_callback,this);
		
	}

    // Update the velocity applied to the Rotors
	public: void UpdateVelocity()
    {
	    // Store the value of some aerodynamic constant
		double k = 0.000015674;
		double b = 0.000000114;	

		// vel is an array that get the rotors velocities calculated by the MPC
		double vel[4] = {this->iris_rotor_vel[0], this->iris_rotor_vel[1], this->iris_rotor_vel[2], this->iris_rotor_vel[3]};
		//vel[0] = 490; vel[1] = 490; vel[2] = 490; vel[3] = 490;
		
		// Create 
		math::Vector3 thrust_force;

		//rotor_0 - Link 6
		//rotor_1 - Link 5
		//rotor_2 - Link 4
		//rotor_3 - Link 3

		//Force Application
		//thrust_force.Set(0,0,k*vel[0]*vel[0]);		
		thrust_force.Set(0,0,this->iris_KT*vel[0]*vel[0]);
		this->model->GetLinks()[3]->AddRelativeForce(thrust_force);

		//thrust_force.Set(0,0,k*vel[1]*vel[1]);
		thrust_force.Set(0,0,this->iris_KT*vel[1]*vel[1]);	
		this->model->GetLinks()[4]->AddRelativeForce(thrust_force);

		//thrust_force.Set(0,0,k*vel[2]*vel[2]);	
		thrust_force.Set(0,0,this->iris_KT*vel[2]*vel[2]);	
		this->model->GetLinks()[5]->AddRelativeForce(thrust_force);

		//thrust_force.Set(0,0,k*vel[3]*vel[3]);
		thrust_force.Set(0,0,this->iris_KT*vel[3]*vel[3]);	
		this->model->GetLinks()[6]->AddRelativeForce(thrust_force);

		//Torque Application 
		math::Vector3 angular_torque;
		angular_torque.Set(0,0,this->iris_KD*vel[0]*vel[0]);
		this->model->GetLinks()[3]->AddRelativeTorque(angular_torque);
		
		angular_torque.Set(0,0,this->iris_KD*vel[1]*vel[1]);
		this->model->GetLinks()[4]->AddRelativeTorque(angular_torque);

		angular_torque.Set(0,0,-this->iris_KD*vel[2]*vel[2]);
		this->model->GetLinks()[5]->AddRelativeTorque(angular_torque);

		angular_torque.Set(0,0,-this->iris_KD*vel[3]*vel[3]);
		this->model->GetLinks()[6]->AddRelativeTorque(angular_torque);

	}


	// Handle an incoming message from ROS
	// param[in] _msg A float value that is used to set the velocity of the Iris Rotors
	public: void OnRosMsg(const geometry_msgs::PoseConstPtr& msg)
	{
		//this->flag_inicio = 0;
		this->flag_inicio = 1;

		this->iris_state_ref[0] = msg->position.x;
		this->iris_state_ref[1] = msg->position.y;
		this->iris_state_ref[2] = msg->position.z;
		this->iris_state_ref[3] = msg->orientation.x;
		this->iris_state_ref[4] = msg->orientation.y;
		this->iris_state_ref[5] = msg->orientation.z;
		std::cout << this->iris_state_ref[0] << " " << this->iris_state_ref[1] << " " << this->iris_state_ref[2] << "\n";
		std::cout << this->iris_state_ref[3] << " " << this->iris_state_ref[4] << " " << this->iris_state_ref[5] << "\n";
	}

	// ROS helper function that processes messages
	private: void QueueThread()
	{
	  static const double timeout = 0.01;
	  while (this->rosNode->ok())
	  {
	    this->rosQueue.callAvailable(ros::WallDuration(timeout));
	  }
	}


	public: void control_callback(const ros::TimerEvent& event)
	{
		//ros::Time time = ros::Time::now();
		double time_ini = this->get_wall_time();
		double time_cpu_ini = this->get_cpu_time();	


		//this->model->GetWorldPose(); ---- Return the linear and angular position of Iris 
		//this->model->GetWorldPose().pos
		//this->model->GetWorldPose().rot.GetPitch();
		//this->model->GetWorldPose().rot.GetRoll();
		//this->model->GetWorldPose().rot.GetYaw();
		//this->model->GetWorldLinearVel(); ---- Return the linear velocity (xp, yp,zp)
		//this->model->GetWorldAngularVel(); ---- Return the angular velocity 
		
		// Get the current position and velocity
		math::Pose iris_pose = this->model->GetWorldPose();
		math::Vector3 iris_linear_vel, iris_angular_vel;
		iris_linear_vel = this->model->GetWorldLinearVel();
		iris_angular_vel = this->model->GetWorldAngularVel();
		
		// Update the state variables
		this->iris_state[0] = iris_pose.pos[0];
		this->iris_state[1] = iris_pose.pos[1];
		this->iris_state[2] = iris_pose.pos[2];
		this->iris_state[3] = iris_pose.rot.GetRoll();
		this->iris_state[4] = iris_pose.rot.GetPitch();
		this->iris_state[5] = iris_pose.rot.GetYaw();
		
		this->iris_state_vel[0] = iris_linear_vel[0];
		this->iris_state_vel[1] = iris_linear_vel[1];
		this->iris_state_vel[2] = iris_linear_vel[2];
		this->iris_state_vel[3] = iris_angular_vel[0];
		this->iris_state_vel[4] = iris_angular_vel[1];
		this->iris_state_vel[5] = iris_angular_vel[2];


		// Check the flag related to the first reference command
		if (this->flag_inicio){

			/*
			//Logic to ensure reference changes at the same time 
			this->tic_simu++;
			if (this->tic_simu == 1000)this->iris_state_ref[3] = 0.2;
			else if (this->tic_simu == 2000)this->iris_state_ref[4] = 0.3;
			else if (this->tic_simu == 3000)this->iris_state_ref[5] = -0.2;
			else if (this->tic_simu == 4000){
				this->iris_state_ref[2] = 2;
				this->iris_state_ref[3] = -0.3;
				this->iris_state_ref[4] = -0.1;
				this->iris_state_ref[5] = 0.3;
			}*/

			//Control Robust
			this->control_state_feedback();

			// Apply the new velocities
			this->UpdateVelocity();

			// Publish the state and velocities to external analysis
			this->pub_data();

		}

		// Store old state
		this->old_iris_state[0] = this->iris_state[0];
		this->old_iris_state[1] = this->iris_state[1];
		this->old_iris_state[2] = this->iris_state[2];
		this->old_iris_state[3] = this->iris_state[3];
		this->old_iris_state[4] = this->iris_state[4];
		this->old_iris_state[5] = this->iris_state[5];
		this->old_iris_state_vel[0] = this->old_iris_state_vel[0];
		this->old_iris_state_vel[1] = this->old_iris_state_vel[1];
		this->old_iris_state_vel[2] = this->old_iris_state_vel[2];
		this->old_iris_state_vel[3] = this->old_iris_state_vel[3];
		this->old_iris_state_vel[4] = this->old_iris_state_vel[4];
		this->old_iris_state_vel[5] = this->old_iris_state_vel[5];


		//ros::Time time2 = ros::Time::now();
		//std::cout << (this->get_wall_time() - time_ini) << "\n";
		this->delta_cpu_time = this->get_cpu_time() - time_cpu_ini;
		//std::cout << (this->get_cpu_time() - time_cpu_ini) << "\n";
		//std::cout << (this->delta_cpu_time) << "\n\n";
		
	}

	public: void pub_data(){
		
		// Create a variable to publish the state
		sensor_msgs::Imu pub_iris_state;
				

		ros::Time time2 = ros::Time::now();
		pub_iris_state.header.stamp = time2;

		// Position data
		pub_iris_state.orientation_covariance[0] = this->iris_state[0];
		pub_iris_state.orientation_covariance[1] = this->iris_state[1];
		pub_iris_state.orientation_covariance[2] = this->iris_state[2];
		pub_iris_state.orientation_covariance[3] = this->iris_state[3];
		pub_iris_state.orientation_covariance[4] = this->iris_state[4];
		pub_iris_state.orientation_covariance[5] = this->iris_state[5];

		// Time data
		pub_iris_state.orientation_covariance[8] = this->delta_cpu_time;

		// Velocity data
		pub_iris_state.angular_velocity_covariance[0] = this->iris_state_vel[0];
		pub_iris_state.angular_velocity_covariance[1] = this->iris_state_vel[1];
		pub_iris_state.angular_velocity_covariance[2] = this->iris_state_vel[2];
		pub_iris_state.angular_velocity_covariance[3] = this->iris_state_vel[3];
		pub_iris_state.angular_velocity_covariance[4] = this->iris_state_vel[4];
		pub_iris_state.angular_velocity_covariance[5] = this->iris_state_vel[5];


		//Control Action
		pub_iris_state.linear_acceleration_covariance[0] = this->control_action[0];
		pub_iris_state.linear_acceleration_covariance[1] = this->control_action[1];
		pub_iris_state.linear_acceleration_covariance[2] = this->control_action[2];
		pub_iris_state.linear_acceleration_covariance[3] = this->control_action[3];

		geometry_msgs::Quaternion pub_vel_rotor;

		// Rotors data
		pub_vel_rotor.x = this->iris_rotor_vel[0];
		pub_vel_rotor.y = this->iris_rotor_vel[1];
		pub_vel_rotor.z = this->iris_rotor_vel[2];
		pub_vel_rotor.w = this->iris_rotor_vel[3];

		// Publish data
		this->state_pub.publish(pub_iris_state);
		this->vel_pub.publish(pub_vel_rotor);

	}

	public: void control_state_feedback(){

		//Update control array state
		this->state_array_update();

		//Update Control
		this->calculate_control_feedback(K_lfr);

		//Convert control action to rotors velocities 
		this->control_action_to_rotor_velocity();

	}

	public: void state_array_update(){
		//z
		this->control_state_array[0] = this->iris_state[2];
		//zp
		this->control_state_array[1] = this->iris_state_vel[2];
		//Phi
		this->control_state_array[2] = this->iris_state[3];
		//Phi p
		this->control_state_array[3] =  this->iris_state_vel[3];
		//Theta
		this->control_state_array[4] =  this->iris_state[4];
		//Theta p
		this->control_state_array[5] =  this->iris_state_vel[4];
		//Psi
		this->control_state_array[6] =  this->iris_state[5];
		//Psi p
		this->control_state_array[7] =  this->iris_state_vel[5];
		//Error output
		this->control_state_array[8] = this->control_state_array[8] + (this->iris_state_ref[2] - this->iris_state[2])*0.01;
		this->control_state_array[9] = this->control_state_array[9] + (this->iris_state_ref[3] - this->iris_state[3])*0.01;
		this->control_state_array[10] = this->control_state_array[10] + (this->iris_state_ref[4] - this->iris_state[4])*0.01;
		this->control_state_array[11] = this->control_state_array[11] + (this->iris_state_ref[5] - this->iris_state[5])*0.01;


		//std::cout << this->control_state_array[0] << this->control_state_array[1] << this->control_state_array[8]  << "\n";
	}

	public: void calculate_control_feedback(double* K_temp){
		Matrix <double> control_state(number_control_states,1);
		control_state = this->control_state_array;

		Matrix <double> K(number_control_actions, number_control_states);
		K = K_temp;
		//std::cout << K.get(0,8) << "\n";
		
		Matrix <double> control = K*control_state;
		for (int it_control = 0; it_control < number_control_actions; it_control++){
        //array_control_action[it_control] = solution[it_control];
        	this->control_action[it_control] = control.get(it_control,0);
        	//std::cout << this->control_action[it_control] << " ";
        }
        ///std::cout << this->control_action[0];
        //std::cout << "\n";
    }

	public: void control_action_to_rotor_velocity() {
		double trust_z = this->control_action[0] + this->iris_mass*this->gravity;
		//std::cout << "trust z: " << trust_z << "\n";
		
		double vel_0_temp = (trust_z/(4*this->iris_KT) - this->control_action[1] / (4 * this->iris_KT*this->iris_lx) - this->control_action[2] / (4 * this->iris_KT*this->iris_ly) + this->control_action[3] / (4 * this->iris_KD));
		if (vel_0_temp > 0) this->iris_rotor_vel[0] = sqrt(vel_0_temp);
		else this->iris_rotor_vel[0] = 0;

		double vel_1_temp = ( trust_z/(4*this->iris_KT) + this->control_action[1]/(4*this->iris_KT*this->iris_lx) + this->control_action[2]/(4*this->iris_KT*this->iris_ly)  + this->control_action[3] / (4 * this->iris_KD));
		if (vel_1_temp > 0) this->iris_rotor_vel[1] = sqrt(vel_1_temp);
		else this->iris_rotor_vel[1] = 0;

		double vel_2_temp = (trust_z/ (4 * this->iris_KT) + this->control_action[1] / (4 * this->iris_KT*this->iris_lx) - this->control_action[2] / (4 * this->iris_KT*this->iris_ly)  - this->control_action[3] / (4 * this->iris_KD));
		if (vel_2_temp > 0) this->iris_rotor_vel[2] = sqrt(vel_2_temp);
		else this->iris_rotor_vel[2] = 0;

		double vel_3_temp = (trust_z/ (4 * this->iris_KT) - this->control_action[1] / (4 * this->iris_KT*this->iris_lx) + this->control_action[2] / (4 * this->iris_KT*this->iris_ly)  - this->control_action[3] / (4 * this->iris_KD));
		if (vel_3_temp > 0) this->iris_rotor_vel[3] = sqrt(vel_3_temp);
		else this->iris_rotor_vel[3] = 0;

		//std::cout << vel_0_temp << " " << vel_1_temp << " " << vel_2_temp << " " << vel_3_temp << "\n\n";

	}

	// Return the Wall time expended
	public: double get_wall_time(){
   		 struct timeval time;
   		 if (gettimeofday(&time,NULL)){
        	//  Handle error
       		return 0;
    	}
    		return (double)time.tv_sec + (double)time.tv_usec * .000001;
	}

	// Return the time expended by the CPU
	public: double get_cpu_time(){
    		return (double)clock() / CLOCKS_PER_SEC;
	}

	// A node used for transport
	private: transport::NodePtr node;

	// A subscriber to a named topic.
	private: transport::SubscriberPtr sub;

	// Pointer to the model.
	private: physics::ModelPtr model;

	// Pointer to the joint.
	private: physics::JointPtr joint;

	// A node use for ROS transport
	private: std::unique_ptr<ros::NodeHandle> rosNode;
	
	// A ROS subscriber
	private: ros::Subscriber rosSub;

	// Publisher of Iris States
	private: ros::Publisher state_pub; 

	//Publisher of Iris Volicities
	private: ros::Publisher vel_pub;

	// A ROS callbackqueue that helps process messages
	private: ros::CallbackQueue rosQueue;

	// A thread the keeps running the rosQueue
	private: std::thread rosQueueThread;  

	// Publisher Timer
	private: ros::Timer pubTimer;

	//Aerodynamics parameters
	private: double iris_KT = 0.000015670;

	private: double iris_KD = 0.0000002551;

	private: double iris_lx = 0.13;

	private: double iris_ly = 0.21;

	private: double iris_mass = 1.37;
	
	private: double gravity = 9.81;

	// Variables related to the currnte and old Irist State
	private: double iris_rotor_vel[4] = {0,0,0,0};
    
	private: double iris_state[6] = {0,0,0,0,0,0};
	
	private: double old_iris_state[6] = {0,0,0,0,0,0};

	private: double iris_state_vel[6] = {0,0,0,0,0,0};

	private: double old_iris_state_vel[6] = {0,0,0,0,0,0};

	//Controller Variables
	private: double iris_state_ref[6] = {0,0,0,0,0,0};

	private: double iris_state_vel_ref[6] = {0,0,0,0,0,0};

	private: double delta_control_action[4] = {0,0,0,0};

	private: double max_delta_control_action[4] = {0,0,0,0};

	private: double control_action[4] = {0,0,0,0};

	private: double old_control_action[4] = {0,0,0,0};

	private: double max_control_action[4] = {0,0,0,0};

	private: double control_state_array[12]= {0,0,0,0,0,0,0,0,0,0,0,0};

    // A flag to initialize the controller only after the first reference command
	private: int flag_inicio = 0;

	// Store the time expended by the CPU
	private: double delta_cpu_time = 0;

	// Variable to update reference
	private: int tic_simu = 0;

	};

  // Tell Gazebo about this plugin, so that Gazebo can call Load on this plugin.
  GZ_REGISTER_MODEL_PLUGIN(IrisPluginRobustControl)
}
#endif
