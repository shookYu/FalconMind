/**
 * @file denied_env_nodes.cpp
 * @brief pybind11 bindings for denied environment SDK modules
 * 
 * 为拒止环境SDK模块创建Python绑定，供Builder Flow调用
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>

#include "falconmind/sdk/navigation/GPSDefender.h"
#include "falconmind/sdk/control/IBVSController.h"
#include "falconmind/sdk/perception/MonocularDistanceEstimator.h"
#include "falconmind/sdk/high_level/DeniedEnvMission.h"

namespace py = pybind11;
using namespace falconmind::sdk;

PYBIND11_MODULE(denied_env_nodes, m) {
    m.doc() = "FalconMind SDK Denied Environment Nodes - Python Bindings";
    
    // =========================================================================
    // GPS Defender Module
    // =========================================================================
    
    py::module_ nav = m.def_submodule("navigation", "Navigation utilities");
    
    // SpoofingAlertLevel enum
    py::enum_<navigation::SpoofingAlertLevel>(nav, "SpoofingAlertLevel")
        .value("NONE", navigation::SpoofingAlertLevel::NONE)
        .value("SUSPECTED", navigation::SpoofingAlertLevel::SUSPECTED)
        .value("DETECTED", navigation::SpoofingAlertLevel::DETECTED)
        .value("CRITICAL", navigation::SpoofingAlertLevel::CRITICAL);
    
    // GNSSMeasurement
    py::class_<navigation::GNSSMeasurement>(nav, "GNSSMeasurement")
        .def(py::init<>())
        .def_readwrite("latitude", &navigation::GNSSMeasurement::latitude)
        .def_readwrite("longitude", &navigation::GNSSMeasurement::longitude)
        .def_readwrite("altitude", &navigation::GNSSMeasurement::altitude)
        .def_readwrite("velocity_north", &navigation::GNSSMeasurement::velocity_north)
        .def_readwrite("velocity_east", &navigation::GNSSMeasurement::velocity_east)
        .def_readwrite("velocity_down", &navigation::GNSSMeasurement::velocity_down)
        .def_readwrite("num_satellites", &navigation::GNSSMeasurement::num_satellites)
        .def_readwrite("hdop", &navigation::GNSSMeasurement::hdop)
        .def_readwrite("vdop", &navigation::GNSSMeasurement::vdop)
        .def_readwrite("pseudoranges", &navigation::GNSSMeasurement::pseudoranges);
    
    // IMUMeasurement
    py::class_<navigation::IMUMeasurement>(nav, "IMUMeasurement")
        .def(py::init<>())
        .def_readwrite("accel", &navigation::IMUMeasurement::accel)
        .def_readwrite("gyro", &navigation::IMUMeasurement::gyro)
        .def_readwrite("temperature", &navigation::IMUMeasurement::temperature);
    
    // VisualPosition
    py::class_<navigation::VisualPosition>(nav, "VisualPosition")
        .def(py::init<>())
        .def_readwrite("north", &navigation::VisualPosition::north)
        .def_readwrite("east", &navigation::VisualPosition::east)
        .def_readwrite("down", &navigation::VisualPosition::down)
        .def_readwrite("confidence", &navigation::VisualPosition::confidence);
    
    // SpoofingReport::Details
    py::class_<navigation::SpoofingReport::Details>(nav, "SpoofingDetails")
        .def(py::init<>())
        .def_readwrite("raim_check_passed", &navigation::SpoofingReport::Details::raim_check_passed)
        .def_readwrite("imu_consistency_passed", &navigation::SpoofingReport::Details::imu_consistency_passed)
        .def_readwrite("vins_consistency_passed", &navigation::SpoofingReport::Details::vins_consistency_passed)
        .def_readwrite("max_pseudorange_residual", &navigation::SpoofingReport::Details::max_pseudorange_residual)
        .def_readwrite("velocity_difference", &navigation::SpoofingReport::Details::velocity_difference)
        .def_readwrite("position_difference", &navigation::SpoofingReport::Details::position_difference);
    
    // SpoofingReport
    py::class_<navigation::SpoofingReport>(nav, "SpoofingReport")
        .def(py::init<>())
        .def_readwrite("level", &navigation::SpoofingReport::level)
        .def_readwrite("confidence", &navigation::SpoofingReport::confidence)
        .def_readwrite("reason", &navigation::SpoofingReport::reason)
        .def_readwrite("recommended_action", &navigation::SpoofingReport::recommended_action)
        .def_readwrite("details", &navigation::SpoofingReport::details);
    
    // GPSDefenderConfig
    py::class_<navigation::GPSDefenderConfig>(nav, "GPSDefenderConfig")
        .def(py::init<>())
        .def_readwrite("raim_threshold", &navigation::GPSDefenderConfig::raim_threshold)
        .def_readwrite("velocity_diff_threshold", &navigation::GPSDefenderConfig::velocity_diff_threshold)
        .def_readwrite("position_diff_threshold", &navigation::GPSDefenderConfig::position_diff_threshold)
        .def_readwrite("satellite_jump_threshold", &navigation::GPSDefenderConfig::satellite_jump_threshold)
        .def_readwrite("dop_jump_threshold", &navigation::GPSDefenderConfig::dop_jump_threshold)
        .def_readwrite("detection_window_size", &navigation::GPSDefenderConfig::detection_window_size)
        .def_readwrite("consecutive_anomaly_threshold", &navigation::GPSDefenderConfig::consecutive_anomaly_threshold);
    
    // GPSDefender
    py::class_<navigation::GPSDefender, std::shared_ptr<navigation::GPSDefender>>(
        nav, "GPSDefender"
    )
        .def(py::init<const navigation::GPSDefenderConfig&>(),
             py::arg("config") = navigation::GPSDefenderConfig())
        .def("initialize", &navigation::GPSDefender::initialize)
        .def("process_gnss", &navigation::GPSDefender::processGNSS,
             py::arg("gnss"))
        .def("process_imu", &navigation::GPSDefender::processIMU,
             py::arg("imu"))
        .def("update_vins_position", &navigation::GPSDefender::updateVINSPosition,
             py::arg("position"))
        .def("get_alert_level", &navigation::GPSDefender::getAlertLevel)
        .def("is_gnss_reliable", &navigation::GPSDefender::isGNSSReliable)
        .def("reset", &navigation::GPSDefender::reset);
    
    // Convenience functions
    nav.def("create_gps_defender", &navigation::createGPSDefender,
            py::arg("config") = navigation::GPSDefenderConfig(),
            "Create a GPS defender with standard configuration");
    
    nav.def("create_strict_gps_defender", &navigation::createStrictGPSDefender,
            "Create a GPS defender with strict detection thresholds");
    
    // =========================================================================
    // IBVS Controller Module
    // =========================================================================
    
    py::module_ ctrl = m.def_submodule("control", "Control utilities");
    
    // CameraParameters
    py::class_<control::CameraParameters>(ctrl, "CameraParameters")
        .def(py::init<>())
        .def_readwrite("width", &control::CameraParameters::width)
        .def_readwrite("height", &control::CameraParameters::height)
        .def_readwrite("fx", &control::CameraParameters::fx)
        .def_readwrite("fy", &control::CameraParameters::fy)
        .def_readwrite("cx", &control::CameraParameters::cx)
        .def_readwrite("cy", &control::CameraParameters::cy)
        .def("get_image_center", &control::CameraParameters::getImageCenter);
    
    // ImageSpaceTarget
    py::class_<control::ImageSpaceTarget>(ctrl, "ImageSpaceTarget")
        .def(py::init<>())
        .def_readwrite("u", &control::ImageSpaceTarget::u)
        .def_readwrite("v", &control::ImageSpaceTarget::v)
        .def_readwrite("area_ratio", &control::ImageSpaceTarget::area_ratio)
        .def_static("from_pixel", &control::ImageSpaceTarget::fromPixel,
                   py::arg("pixel_x"), py::arg("pixel_y"), py::arg("camera"));
    
    // VelocityCommand
    py::class_<control::VelocityCommand>(ctrl, "VelocityCommand")
        .def(py::init<>())
        .def_readwrite("vx", &control::VelocityCommand::vx)
        .def_readwrite("vy", &control::VelocityCommand::vy)
        .def_readwrite("vz", &control::VelocityCommand::vz)
        .def_readwrite("yaw_rate", &control::VelocityCommand::yaw_rate)
        .def("saturate", &control::VelocityCommand::saturate,
             py::arg("max_v_xy"), py::arg("max_v_z"), py::arg("max_yaw_rate"));
    
    // IBVSConfig
    py::class_<control::IBVSConfig>(ctrl, "IBVSConfig")
        .def(py::init<>())
        .def_readwrite("desired_distance", &control::IBVSConfig::desired_distance)
        .def_readwrite("desired_height", &control::IBVSConfig::desired_height)
        .def_readwrite("distance_tolerance", &control::IBVSConfig::distance_tolerance)
        .def_readwrite("height_tolerance", &control::IBVSConfig::height_tolerance)
        .def_readwrite("max_speed", &control::IBVSConfig::max_speed)
        .def_readwrite("max_vertical_speed", &control::IBVSConfig::max_vertical_speed)
        .def_readwrite("max_yaw_rate", &control::IBVSConfig::max_yaw_rate)
        .def_readwrite("kp_distance", &control::IBVSConfig::kp_distance)
        .def_readwrite("ki_distance", &control::IBVSConfig::ki_distance)
        .def_readwrite("kd_distance", &control::IBVSConfig::kd_distance)
        .def_readwrite("kp_position", &control::IBVSConfig::kp_position)
        .def_readwrite("kp_height", &control::IBVSConfig::kp_height)
        .def_readwrite("kp_yaw", &control::IBVSConfig::kp_yaw)
        .def_readwrite("enable_adaptive_gain", &control::IBVSConfig::enable_adaptive_gain)
        .def_readwrite("camera", &control::IBVSConfig::camera);
    
    // TrackingQuality
    py::class_<control::TrackingQuality>(ctrl, "TrackingQuality")
        .def(py::init<>())
        .def_readwrite("distance_error", &control::TrackingQuality::distance_error)
        .def_readwrite("position_error_u", &control::TrackingQuality::position_error_u)
        .def_readwrite("position_error_v", &control::TrackingQuality::position_error_v)
        .def_readwrite("quality_score", &control::TrackingQuality::quality_score)
        .def_readwrite("is_distance_good", &control::TrackingQuality::is_distance_good)
        .def_readwrite("is_position_good", &control::TrackingQuality::is_position_good);
    
    // IBVSController
    py::class_<control::IBVSController, std::shared_ptr<control::IBVSController>>(
        ctrl, "IBVSController"
    )
        .def(py::init<const control::IBVSConfig&>(),
             py::arg("config") = control::IBVSConfig())
        .def("initialize", &control::IBVSController::initialize)
        .def("compute_control", &control::IBVSController::computeControl,
             py::arg("target"), py::arg("current_distance"), py::arg("current_height"))
        .def("compute_quality", &control::IBVSController::computeQuality,
             py::arg("current_distance"), py::arg("current_height"), py::arg("target"))
        .def("set_desired_distance", &control::IBVSController::setDesiredDistance,
             py::arg("distance"))
        .def("set_desired_height", &control::IBVSController::setDesiredHeight,
             py::arg("height"))
        .def("get_config", &control::IBVSController::getConfig)
        .def("update_config", &control::IBVSController::updateConfig,
             py::arg("config"))
        .def("reset", &control::IBVSController::reset)
        .def("is_at_target", &control::IBVSController::isAtTarget,
             py::arg("current_distance"), py::arg("current_height"), py::arg("target"));
    
    ctrl.def("create_ibvs_controller", &control::createIBVSController,
            py::arg("config") = control::IBVSConfig(),
            "Create an IBVS controller");
    
    ctrl.def("create_conservative_ibvs_controller", &control::createConservativeIBVSController,
            "Create a conservative IBVS controller (stable but slower)");
    
    ctrl.def("create_aggressive_ibvs_controller", &control::createAggressiveIBVSController,
            "Create an aggressive IBVS controller (fast but may overshoot)");
    
    // =========================================================================
    // Monocular Distance Estimator Module
    // =========================================================================
    
    py::module_ percep = m.def_submodule("perception", "Perception utilities");
    
    // ObjectDimensions
    py::class_<perception::ObjectDimensions>(percep, "ObjectDimensions")
        .def(py::init<>())
        .def_readwrite("height", &perception::ObjectDimensions::height)
        .def_readwrite("width", &perception::ObjectDimensions::width)
        .def_readwrite("depth", &perception::ObjectDimensions::depth);
    
    // DistanceEstimationMethod
    py::enum_<perception::DistanceEstimationMethod>(percep, "DistanceEstimationMethod")
        .value("KNOWN_SIZE", perception::DistanceEstimationMethod::KNOWN_SIZE)
        .value("NEURAL_NETWORK", perception::DistanceEstimationMethod::NEURAL_NETWORK)
        .value("STEREO_DISPARITY", perception::DistanceEstimationMethod::STEREO_DISPARITY);
    
    // DistanceEstimate
    py::class_<perception::DistanceEstimate>(percep, "DistanceEstimate")
        .def(py::init<>())
        .def_readwrite("distance", &perception::DistanceEstimate::distance)
        .def_readwrite("confidence", &perception::DistanceEstimate::confidence)
        .def_readwrite("method", &perception::DistanceEstimate::method)
        .def_readwrite("target_class", &perception::DistanceEstimate::target_class);
    
    // MonocularCameraIntrinsics
    py::class_<perception::MonocularCameraIntrinsics>(percep, "MonocularCameraIntrinsics")
        .def(py::init<>())
        .def_readwrite("fx", &perception::MonocularCameraIntrinsics::fx)
        .def_readwrite("fy", &perception::MonocularCameraIntrinsics::fy)
        .def_readwrite("cx", &perception::MonocularCameraIntrinsics::cx)
        .def_readwrite("cy", &perception::MonocularCameraIntrinsics::cy)
        .def_readwrite("width", &perception::MonocularCameraIntrinsics::width)
        .def_readwrite("height", &perception::MonocularCameraIntrinsics::height)
        .def_static("from_fov", &perception::MonocularCameraIntrinsics::fromFOV,
                   py::arg("fov_degrees"), py::arg("width"), py::arg("height"),
                   py::arg("is_horizontal") = true);
    
    // MonocularDistanceEstimator
    py::class_<perception::MonocularDistanceEstimator, 
               std::shared_ptr<perception::MonocularDistanceEstimator>>(
        percep, "MonocularDistanceEstimator"
    )
        .def(py::init<const perception::MonocularCameraIntrinsics&>(),
             py::arg("intrinsics") = perception::MonocularCameraIntrinsics())
        .def("initialize", &perception::MonocularDistanceEstimator::initialize)
        .def("register_object_type", &perception::MonocularDistanceEstimator::registerObjectType,
             py::arg("class_name"), py::arg("height"), 
             py::arg("width") = 0.0, py::arg("depth") = 0.0)
        .def("estimate", &perception::MonocularDistanceEstimator::estimate,
             py::arg("bbox"), py::arg("class_name"))
        .def("estimate_with_height", &perception::MonocularDistanceEstimator::estimateWithHeight,
             py::arg("bbox"), py::arg("real_height"))
        .def("estimate_from_area", &perception::MonocularDistanceEstimator::estimateFromArea,
             py::arg("bbox"), py::arg("class_name"))
        .def("get_registered_classes", &perception::MonocularDistanceEstimator::getRegisteredClasses)
        .def("get_object_dimensions", &perception::MonocularDistanceEstimator::getObjectDimensions,
             py::arg("class_name"))
        .def_static("register_default_types", &perception::MonocularDistanceEstimator::registerDefaultTypes,
                   py::arg("estimator"));
    
    percep.def("create_distance_estimator", 
              &perception::createDistanceEstimator,
              py::arg("intrinsics") = perception::MonocularCameraIntrinsics(),
              "Create a distance estimator with default object types");
    
    // =========================================================================
    // Denied Environment Mission Module
    // =========================================================================
    
    py::module_ hl = m.def_submodule("high_level", "High-level mission APIs");
    
    // SearchArea
    py::class_<high_level::SearchArea>(hl, "SearchArea")
        .def(py::init<>())
        .def_readwrite("boundary", &high_level::SearchArea::boundary)
        .def_readwrite("altitude", &high_level::SearchArea::altitude)
        .def_readwrite("speed", &high_level::SearchArea::speed)
        .def_readwrite("pattern", &high_level::SearchArea::pattern);
    
    // TargetSelection
    py::class_<high_level::TargetSelection>(hl, "TargetSelection")
        .def(py::init<>())
        .def_readwrite("track_id", &high_level::TargetSelection::track_id)
        .def_readwrite("class_name", &high_level::TargetSelection::class_name)
        .def_readwrite("confidence", &high_level::TargetSelection::confidence)
        .def_readwrite("estimated_distance", &high_level::TargetSelection::estimated_distance)
        .def_readwrite("confirmed", &high_level::TargetSelection::confirmed);
    
    // TrackingParameters
    py::class_<high_level::TrackingParameters>(hl, "TrackingParameters")
        .def(py::init<>())
        .def_readwrite("desired_distance", &high_level::TrackingParameters::desired_distance)
        .def_readwrite("desired_height", &high_level::TrackingParameters::desired_height)
        .def_readwrite("distance_tolerance", &high_level::TrackingParameters::distance_tolerance)
        .def_readwrite("height_tolerance", &high_level::TrackingParameters::height_tolerance)
        .def_readwrite("max_speed", &high_level::TrackingParameters::max_speed)
        .def_readwrite("tracking_timeout", &high_level::TrackingParameters::tracking_timeout);
    
    // DeniedEnvMissionCallbacks
    py::class_<high_level::DeniedEnvMissionCallbacks>(hl, "DeniedEnvMissionCallbacks")
        .def(py::init<>())
        .def_readwrite("on_phase_transition", &high_level::DeniedEnvMissionCallbacks::onPhaseTransition)
        .def_readwrite("on_vins_progress", &high_level::DeniedEnvMissionCallbacks::onVINSProgress)
        .def_readwrite("on_vins_complete", &high_level::DeniedEnvMissionCallbacks::onVINSComplete)
        .def_readwrite("on_spoofing_alert", &high_level::DeniedEnvMissionCallbacks::onSpoofingAlert)
        .def_readwrite("on_targets_detected", &high_level::DeniedEnvMissionCallbacks::onTargetsDetected)
        .def_readwrite("on_tracking_update", &high_level::DeniedEnvMissionCallbacks::onTrackingUpdate)
        .def_readwrite("on_error", &high_level::DeniedEnvMissionCallbacks::onError)
        .def_readwrite("on_mission_complete", &high_level::DeniedEnvMissionCallbacks::onMissionComplete);
    
    // DeniedEnvMissionConfig
    py::class_<high_level::DeniedEnvMissionConfig>(hl, "DeniedEnvMissionConfig")
        .def(py::init<>())
        .def_readwrite("vins_init_timeout", &high_level::DeniedEnvMissionConfig::vins_init_timeout)
        .def_readwrite("vins_required_features", &high_level::DeniedEnvMissionConfig::vins_required_features)
        .def_readwrite("gps_defense_config", &high_level::DeniedEnvMissionConfig::gps_defense_config)
        .def_readwrite("tracking_params", &high_level::DeniedEnvMissionConfig::tracking_params)
        .def_readwrite("ibvs_config", &high_level::DeniedEnvMissionConfig::ibvs_config)
        .def_readwrite("camera_intrinsics", &high_level::DeniedEnvMissionConfig::camera_intrinsics)
        .def_readwrite("target_classes", &high_level::DeniedEnvMissionConfig::target_classes)
        .def_readwrite("callbacks", &high_level::DeniedEnvMissionConfig::callbacks);
    
    // DeniedEnvMission
    py::class_<high_level::DeniedEnvMission, std::shared_ptr<high_level::DeniedEnvMission>>(
        hl, "DeniedEnvMission"
    )
        .def(py::init<const high_level::DeniedEnvMissionConfig&>(),
             py::arg("config"))
        .def("initialize", &high_level::DeniedEnvMission::initialize)
        .def("initialize_vins", &high_level::DeniedEnvMission::initializeVINS)
        .def("get_vins_progress", &high_level::DeniedEnvMission::getVINSProgress)
        .def("start_search", &high_level::DeniedEnvMission::startSearch,
             py::arg("area"))
        .def("select_target", &high_level::DeniedEnvMission::selectTarget,
             py::arg("track_id"))
        .def("confirm_target", &high_level::DeniedEnvMission::confirmTarget)
        .def("start_tracking", 
             py::overload_cast<int>(&high_level::DeniedEnvMission::startTracking),
             py::arg("track_id"))
        .def("get_tracking_quality", &high_level::DeniedEnvMission::getTrackingQuality)
        .def("is_tracking", &high_level::DeniedEnvMission::isTracking)
        .def("abort_and_return", &high_level::DeniedEnvMission::abortAndReturn)
        .def("return_to_launch", &high_level::DeniedEnvMission::returnToLaunch)
        .def("land", &high_level::DeniedEnvMission::land)
        .def("get_gps_defender", &high_level::DeniedEnvMission::getGPSDefender)
        .def("get_ibvs_controller", &high_level::DeniedEnvMission::getIBVSController);
    
    hl.def("create_denied_env_mission", &high_level::createDeniedEnvMission,
          py::arg("config"),
          "Create a denied environment mission");
    
    hl.def("create_standard_denied_env_mission", &high_level::createStandardDeniedEnvMission,
          "Create a standard denied environment mission with default configuration");
}
