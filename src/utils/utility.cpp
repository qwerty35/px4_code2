//
// Created by jbs on 21. 1. 22..
//
#include <utils/utility.h>

namespace px4_code2 {


    double getYaw (const geometry_msgs::Quaternion& q){
        tf::Quaternion qq;
        qq.setX(q.x);
        qq.setY(q.y);
        qq.setZ(q.z);
        qq.setW(q.w);
        qq.normalize();
        double r,p,y;
        tf::Matrix3x3 Q(qq); Q.getRPY(r,p,y);
        return y;
    }

    double interpolate(const vector<double>& xData,const vector<double>& yData, const double& x,bool extrapolate){

        int size = xData.size();
        int i = 0;                                                                  // find left end of interval for interpolation
        if ( x >= xData[size - 2] )                                                 // special case: beyond right end
        {
            i = size - 2;
        }
        else
        {
            while ( x > xData[i+1] ) i++;
        }
        double xL = xData[i], yL = yData[i], xR = xData[i+1], yR = yData[i+1];      // points on either side (unless beyond ends)
        if ( !extrapolate )                                                         // if beyond ends of array and not extrapolating
        {
            if ( x < xL ) {yR = yL; }
            if ( x > xR ) yL = yR;
        }

        double dydx = ( yR - yL ) / ( xR - xL );                                    // gradient
        return yL + dydx * ( x - xL );                                              // linear interpolation
    }


    geometry_msgs::PoseStamped convertTo(const nav_msgs::Odometry &odom) {
        geometry_msgs::PoseStamped poseStamped;
        poseStamped.pose.position = odom.pose.pose.position;
        poseStamped.pose.orientation = odom.pose.pose.orientation;
        poseStamped.header = odom.header;
        return poseStamped;
    }

    Trajectory::Trajectory(TrajGenObj *trajPtr,double fixedYaw, double duration) {
        int N = 50; //TODO
        double dt = duration/N;

        for (int n = 0 ; n < N ; n++){
            double t = dt*n;
            TrajVector p = trajPtr->eval(t,0);
            ts.push_back(t);
            xs.push_back(p(0));
            ys.push_back(p(1));
            zs.push_back(p(2));
            yaws.push_back(fixedYaw);
        }
    }
    void Mission::loadTrajectory(const Trajectory &traj) {

        missionTraj = traj; triggerTime = ros::Time::now();
        path.header.frame_id = frame;
        path.poses.clear();

        for (int n = 0 ; n < traj.ts.size() ; n++){
            path.poses.push_back(getCurDesPose(traj.ts[n]));
        }


    }

    geometry_msgs::PoseStamped Mission::getCurDesPose(double t) {
        lastEmitTime = t;
//        cout << t << "/" << missionTraj.ts.back() << endl;
        bool extrapolate = false;
        if (t > missionTraj.ts.back()){
            ROS_WARN_THROTTLE(3,"Time evaluation %f of trajectory exceed limit %f. Clamped", t, missionTraj.ts.back());
            t = missionTraj.ts.back() - 0.0001;
        }

        if (t < missionTraj.ts.front()){
            ROS_WARN_THROTTLE(3,"Time evaluation %f of trajectory is too early %f. Clamped", t, missionTraj.ts.front());
            t = missionTraj.ts.front() + 0.0001;
        }

        double x= interpolate(missionTraj.ts,missionTraj.xs,t,extrapolate);
        double y= interpolate(missionTraj.ts,missionTraj.ys,t,extrapolate);
        double z= interpolate(missionTraj.ts,missionTraj.zs,t,extrapolate);
        double yaw= interpolate(missionTraj.ts,missionTraj.yaws,t,extrapolate);

        tf::Quaternion q;
        q.setRPY(0,0,yaw); q.normalize();
//        cout << q.x() << " , " << q.y() << " , " << q.z() << " , " << q.w() << endl;

        geometry_msgs::PoseStamped poseStamped;
        poseStamped.header.frame_id = frame;
        poseStamped.header.stamp = ros::Time::now();
        poseStamped.pose.position.x = x;
        poseStamped.pose.position.y = y;
        poseStamped.pose.position.z = z;

        poseStamped.pose.orientation.x = q.x();
        poseStamped.pose.orientation.y = q.y();
        poseStamped.pose.orientation.z = q.z();
        poseStamped.pose.orientation.w = q.w();
        return poseStamped;
    }

    geometry_msgs::PoseStamped Mission::emitDesPose(ros::Time t) {
        if (isActive) {
            double tEval = emittedDuration + (t - triggerTime).toSec();
            lastEmitPose = getCurDesPose(tEval);
        }
        return lastEmitPose;
    }


}