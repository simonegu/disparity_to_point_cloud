#include "disparity_to_point_cloud/disparity_to_point_cloud.hpp"

namespace d2pc {

void Disparity2PCloud::DisparityCb(const sensor_msgs::ImageConstPtr &msg) {
  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
  cv_bridge::CvImagePtr disparity = cv_bridge::toCvCopy(*msg, "mono8");
  cv::Size s = disparity->image.size();
  float disp, X, Y, Z;
  uchar *pf;
  uchar *pf_m1;
  uchar *pf_m2;
  uchar *pf_p1;
  uchar *pf_p2;
  tf::StampedTransform transform;
  try {
    // look up transform for a point in the camera optical frame to world
    // coordinates
    // TODO: investigate lookup transform version of this
    listener_.lookupTransform("/world", "/camera_optical_frame", ros::Time(0),
                              transform);
  }
  catch (tf::TransformException ex) {
    ROS_WARN("%s", ex.what());
  }
  // transformation in Rotation matrix and translation vector
  Eigen::Matrix3d R;
  tf::matrixTFToEigen(transform.getBasis(), R);
  // std::cout << "Rotation: " << R << std::endl;
  // get translation
  Eigen::Vector3d t(transform.getOrigin().x(), transform.getOrigin().y(),
                    transform.getOrigin().z());
  Eigen::Vector3d point;
  // cv::Mat discreateDisparity;
  // disparity->image.convertTo(discreateDisparity, CV_32FC1);
  cv::Mat medianFilterd = disparity->image;
  // cv::Mat medianFilterd;
  // cv::medianBlur(disparity->image, medianFilterd, 11);
  // cv::imshow("disparity", disparity->image);
  // cv::imshow("disparity median filter", medianFilterd);
  // cv::waitKey(0);  // Wait for a keystroke in the window
  // form 3 to width -3 : to remove outlire on the border
  for (int v = 30; v < s.height - 30; v++) {
    pf = medianFilterd.ptr<uchar>(v);
    pf_m1 = medianFilterd.ptr<uchar>(v - 1);
    pf_p1 = medianFilterd.ptr<uchar>(v + 1);
    pf_m2 = medianFilterd.ptr<uchar>(v - 2);
    pf_p2 = medianFilterd.ptr<uchar>(v + 2);
    for (int u = 30; u < s.width - 30; u++) {
      uchar value = pf[u];
      int count = 0;
      // a disparity of 1 pixel is really difficult to detect -> usally
      // outlier
      // minimal disparity 5 pixel
      if (value > 5) {
        // look in a 5x5 patch if there is consensus on the dispartiy if yes
        // keep point otherwise not
        if (abs(pf_m2[u - 1] - value) < threshold_) {
          count++;
        }
        if (abs(pf_m2[u] - value) < threshold_) {
          count++;
        }
        if (abs(pf_m2[u + 1] - value) < threshold_) {
          count++;
        }
        if (abs(pf_m2[u - 2] - value) < threshold_) {
          count++;
        }
        if (abs(pf_m2[u + 2] - value) < threshold_) {
          count++;
        }
        ////////////////////////////////////////////////////////////////////////
        if (abs(pf_m1[u - 2] - value) < threshold_) {
          count++;
        }
        if (abs(pf_m1[u - 1] - value) < threshold_) {
          count++;
        }
        if (abs(pf_m1[u] - value) < threshold_) {
          count++;
        }
        if (abs(pf_m1[u + 1] - value) < threshold_) {
          count++;
        }
        if (abs(pf_m1[u + 2] - value) < threshold_) {
          count++;
        }
        ////////////////////////////////////////////////////////////////////////
        if (abs(pf[u - 2] - value) < threshold_) {
          count++;
        }
        if (abs(pf[u - 1] - value) < threshold_) {
          count++;
        }
        if (abs(pf[u + 1] - value) < threshold_) {
          count++;
        }
        if (abs(pf[u + 2] - value) < threshold_) {
          count++;
        }
        ////////////////////////////////////////////////////////////////////////
        if (abs(pf_p1[u - 2] - value) < threshold_) {
          count++;
        }
        if (abs(pf_p1[u - 1] - value) < threshold_) {
          count++;
        }
        if (abs(pf_p1[u] - value) < threshold_) {
          count++;
        }
        if (abs(pf_p1[u + 1] - value) < threshold_) {
          count++;
        }
        if (abs(pf_p1[u + 2] - value) < threshold_) {
          count++;
        }
        ////////////////////////////////////////////////////////////////////////
        if (abs(pf_p2[u - 2] - value) < threshold_) {
          count++;
        }
        if (abs(pf_p2[u - 1] - value) < threshold_) {
          count++;
        }
        if (abs(pf_p2[u] - value) < threshold_) {
          count++;
        }
        if (abs(pf_p2[u + 1] - value) < threshold_) {
          count++;
        }
        if (abs(pf_p2[u + 2] - value) < threshold_) {
          count++;
        }

        if (count > min_count_) {
          disp = value / 8.0f;
          Z = fx_ * base_line_ / disp;
          X = (u - cx_) * Z / fx_;
          Y = (v - cy_) * Z / fy_;
          point << X, Y, Z;
          Eigen::Vector3d global_p = R * point + t;
          cloud->points.push_back(
              pcl::PointXYZ(global_p(0), global_p(1), global_p(2)));
        }
      }
    }
  }
  cloud->width = cloud->points.size();
  cloud->height = 1;
  cloud->is_dense = false;
  // send point cloud
  sensor_msgs::PointCloud2 output;
  pcl::toROSMsg(*cloud, output);
  // does not work on the rosbag with the tf broadcaster for the camera position
  output.header.stamp = disparity->header.stamp;
  output.header.frame_id = "/world";
  p_cloud_pub_.publish(output);
}

} /* d2pc */
