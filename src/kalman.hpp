#include <vector>
#include <opencv2/opencv.hpp>
#include <boost/random.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/nondet_random.hpp>
#include <boost/random/normal_distribution.hpp>

#include "ros/ros.h"
#include "geometry_msgs/Twist.h"
#include "nav_msgs/Odometry.h"
#include "sensor_msgs/LaserScan.h"
#include <tf/tf.h>
#include "tf/transform_broadcaster.h"
#include <Eigen/Core>
#include <Eigen/Eigen>
#include <visualization_msgs/Marker.h>
#include "featuresextractor.h"

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_ros/publisher.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/common/transforms.h>
#include <tf/transform_listener.h>
#include <pcl_ros/transforms.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/registration/icp.h>
#include <pcl/registration/icp_nl.h>
#include <pcl/filters/voxel_grid.h>
struct rangle
{
    double range, angle;
    rangle(double r, double a) : range(r), angle(a) {}
};

class kalman
{
    typedef pcl::PointXYZI point_type;
    boost::shared_ptr<tf::TransformListener> listener;

    Eigen::Matrix4f laserToMapEigen;
    Eigen::Matrix4f laserToBaseEigen;

    FeaturesExtractor features_extractor;
    std::string base_link;
    std::string odom_link;
    std::string map_link;
    std::string laser_link;
    cv::Mat map;
    double dt;
    double gt_x, gt_y, gt_theta;
    double linear, angular;
    ros::Subscriber cmd_sub;
    ros::Subscriber laser_sub;
    ros::Subscriber bpgt_sub;
    ros::Publisher location_undertainty;
    ros::Publisher map_features_pub;
    ros::Publisher local_features_pub;

    tf::TransformBroadcaster tf_broadcaster;
    tf::Transform latest_tf_;
    pcl::PointCloud<point_type>::Ptr laser;
    ros::Time laser_time;

    cv::Vec3d X;           //!< predicted state (x'(k)): x(k)=A*x(k-1)+B*u(k)
    cv::Matx<double,3,3> F;   				//!< state transition matrix (F)
    cv::Matx<double,3,3> I;   				//!< state transition matrix (F)
    cv::Matx<double,3,3> H;  				//!< measurement matrix (H)
    cv::Matx<double,3,3> Q;					//!< process noise covariance matrix (Q)
    //cv::Mat measurementNoiseCov;//!< measurement noise covariance matrix (R)
    cv::Matx<double,3,3> K;               //!< Kalman gain matrix (K(k)): K(k)=P'(k)*Ht*inv(H*P'(k)*Ht+R)
    cv::Matx<double,3,3> P;
    cv::Matx<double,3,3> R;
    pcl::PointCloud<point_type>::Ptr map_features;

public:

    void publishFeatures()
    {
        map_features_pub.publish(map_features);
    }

    cv::Vec3d getX()
    {
        return X;
    }

    cv::Matx<double,3,3> getP()
    {
        return P;
    }

    void drawCovariance(const Eigen::Vector2f& mean, const Eigen::Matrix2f& covMatrix);
    void drawFeatures();

    kalman(ros::NodeHandle& nh, const cv::Mat& pmap, double x_init, double y_init, double theta_init, int spin_rate);
    void broadcast();

    void pose_callback(const nav_msgs::Odometry msg);
    void laser_callback(const sensor_msgs::LaserScan::ConstPtr& msg);


    void predict(const nav_msgs::Odometry msg);
    void correct(const cv::Vec3d & obs);

    cv::Point2i toImage(cv::Point2d p) const;

    //cv::Mat show_map(const std::string& win_name, bool draw) const;

    enum {OCCUPIED = 0, FREE = 255};
    const static int CV_TYPE = CV_64F;

    void angleOverflowCorrect(double& a)
    {
        while ((a) >  M_PI) a -= 2*M_PI;
        while ((a) < -M_PI) a += 2*M_PI;
    }
};

