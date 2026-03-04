"""
Visual Tracking Service

视觉跟踪服务，实现基于DeepSORT的目标跟踪和视觉伺服控制。
"""

import numpy as np
from datetime import datetime, timedelta
from typing import List, Tuple, Optional, Dict, Any
from dataclasses import dataclass
from enum import Enum, auto


class TrackingState(Enum):
    """跟踪状态"""
    SEARCHING = auto()      # 搜索目标
    ACQUIRED = auto()       # 已获取目标
    LOCKED = auto()         # 锁定跟踪
    LOST = auto()           # 目标丢失
    RECOVERING = auto()     # 尝试恢复


@dataclass
class BoundingBox:
    """边界框"""
    x1: int
    y1: int
    x2: int
    y2: int
    confidence: float
    
    @property
    def width(self) -> int:
        return self.x2 - self.x1
        
    @property
    def height(self) -> int:
        return self.y2 - self.y1
        
    @property
    def center(self) -> Tuple[int, int]:
        return ((self.x1 + self.x2) // 2, (self.y1 + self.y2) // 2)
        
    @property
    def area(self) -> int:
        return self.width * self.height


@dataclass
class Detection:
    """检测结果"""
    bbox: BoundingBox
    class_id: int
    class_name: str
    confidence: float
    timestamp: datetime


@dataclass
class Track:
    """跟踪目标"""
    track_id: int
    detections: List[Detection]
    features: np.ndarray  # DeepSORT外观特征
    kalman_state: np.ndarray  # Kalman滤波器状态
    age: int  # 总帧数
    time_since_update: int  # 未更新帧数
    is_confirmed: bool
    hits: int  # 成功匹配次数
    
    @property
    def last_detection(self) -> Detection:
        return self.detections[-1]
        
    @property
    def velocity(self) -> np.ndarray:
        """估计速度 (像素/帧)"""
        if len(self.detections) < 2:
            return np.zeros(2)
        
        prev = self.detections[-2].bbox.center
        curr = self.detections[-1].bbox.center
        return np.array([curr[0] - prev[0], curr[1] - prev[1]])


@dataclass
class CameraParameters:
    """相机参数"""
    width: int
    height: int
    fx: float  # 焦距 x
    fy: float  # 焦距 y
    cx: float  # 光心 x
    cy: float  # 光心 y
    
    @property
    def image_center(self) -> Tuple[int, int]:
        return (int(self.cx), int(self.cy))


@dataclass
class TrackingCommand:
    """跟踪控制输出"""
    vx: float  # 前向速度 (m/s)
    vy: float  # 右向速度 (m/s)
    vz: float  # 垂直速度 (m/s)
    yaw_rate: float  # 偏航角速度 (rad/s)
    distance_estimate: float  # 估计距离 (m)
    tracking_quality: float  # 跟踪质量 0-1


class DeepSORTTracker:
    """
    DeepSORT多目标跟踪器
    
    算法流程：
    1. YOLOv8检测目标
    2. 外观特征提取（OSNet）
    3. 级联匹配（马氏距离 + 余弦距离）
    4. IOU匹配（未匹配检测与未确认跟踪）
    5. Kalman滤波器预测与更新
    """
    
    # 参数
    MAX_AGE = 30  # 最大未更新帧数
    MIN_HITS = 3  # 确认所需最小匹配次数
    IOU_THRESHOLD = 0.3
    COSINE_DISTANCE_THRESHOLD = 0.2
    MAHALANOBIS_THRESHOLD = 9.4877  # 卡方分布95%置信度
    
    def __init__(self, camera_params: CameraParameters):
        self.camera = camera_params
        self.tracks: List[Track] = []
        self.next_track_id = 1
        
        # 特征提取模型（实际加载OSNet）
        self.feature_extractor = None  # Placeholder for OSNet model
        
    def update(self, detections: List[Detection], frame: np.ndarray) -> List[Track]:
        """
        更新跟踪器
        
        Args:
            detections: YOLO检测结果
            frame: 当前帧图像
            
        Returns:
            当前活跃跟踪列表
        """
        # 1. 预测已有跟踪
        for track in self.tracks:
            self._predict_kalman(track)
            
        # 2. 提取外观特征
        features = self._extract_features(detections, frame)
        
        # 3. 级联匹配
        matched, unmatched_dets, unmatched_tracks = self._cascade_match(
            detections, features
        )
        
        # 4. IOU匹配剩余
        matched_iou, unmatched_dets, unmatched_tracks = self._iou_match(
            unmatched_dets, unmatched_tracks, detections
        )
        
        matched.extend(matched_iou)
        
        # 5. 更新匹配跟踪
        for track_idx, det_idx in matched:
            self._update_track(self.tracks[track_idx], detections[det_idx], features[det_idx])
            
        # 6. 创建新跟踪
        for det_idx in unmatched_dets:
            self._create_track(detections[det_idx], features[det_idx])
            
        # 7. 删除过期跟踪
        self._delete_dead_tracks()
        
        # 8. 标记确认跟踪
        for track in self.tracks:
            if not track.is_confirmed and track.hits >= self.MIN_HITS:
                track.is_confirmed = True
                
        return [t for t in self.tracks if t.is_confirmed]
        
    def _extract_features(self, detections: List[Detection], frame: np.ndarray) -> List[np.ndarray]:
        """提取外观特征"""
        features = []
        
        for det in detections:
            # 裁剪目标区域
            x1, y1, x2, y2 = det.bbox.x1, det.bbox.y1, det.bbox.x2, det.bbox.y2
            crop = frame[y1:y2, x1:x2]
            
            if crop.size == 0:
                features.append(np.zeros(128))  # 默认特征
                continue
                
            # 使用OSNet提取128维特征（简化实现）
            # 实际使用: feature = self.feature_extractor(crop)
            feature = self._simple_feature(crop)
            features.append(feature)
            
        return features
        
    def _simple_feature(self, image: np.ndarray) -> np.ndarray:
        """简化特征提取（实际使用深度学习）"""
        # 颜色直方图作为简化特征
        import cv2
        hist = cv2.calcHist([image], [0, 1, 2], None, [8, 8, 8], [0, 256, 0, 256, 0, 256])
        hist = cv2.normalize(hist, hist).flatten()
        
        # 扩展到128维
        feature = np.resize(hist, 128)
        feature = feature / np.linalg.norm(feature)  # L2归一化
        
        return feature
        
    def _predict_kalman(self, track: Track):
        """Kalman滤波器预测"""
        # 简化的Kalman预测（实际使用filterpy或自定义实现）
        # 状态: [u, v, s, r, u_dot, v_dot, s_dot]
        # u,v: 中心坐标, s: 尺度, r: 宽高比
        
        if track.kalman_state is None:
            det = track.last_detection
            u, v = det.bbox.center
            s = det.bbox.area
            r = det.bbox.width / det.bbox.height
            track.kalman_state = np.array([u, v, s, r, 0, 0, 0], dtype=float)
        else:
            # 匀速预测
            track.kalman_state[0] += track.kalman_state[4]  # u += u_dot
            track.kalman_state[1] += track.kalman_state[5]  # v += v_dot
            track.kalman_state[2] += track.kalman_state[6]  # s += s_dot
            
        track.age += 1
        track.time_since_update += 1
        
    def _cascade_match(self, detections: List[Detection], features: List[np.ndarray]) -> Tuple[List[Tuple[int, int]], List[int], List[int]]:
        """级联匹配"""
        matched = []
        unmatched_dets = list(range(len(detections)))
        unmatched_tracks = []
        
        # 按age分层匹配
        track_indices = list(range(len(self.tracks)))
        track_indices = sorted(track_indices, key=lambda i: self.tracks[i].age, reverse=True)
        
        for track_idx in track_indices:
            track = self.tracks[track_idx]
            
            if track.time_since_update > self.MAX_AGE:
                unmatched_tracks.append(track_idx)
                continue
                
            # 计算与所有未匹配检测的距离
            best_match = None
            best_dist = float('inf')
            
            for det_idx in unmatched_dets:
                # 马氏距离
                mahal_dist = self._mahalanobis_distance(track, detections[det_idx])
                
                # 余弦距离
                cos_dist = self._cosine_distance(track.features, features[det_idx])
                
                # 加权距离
                total_dist = 0.5 * mahal_dist + 0.5 * cos_dist
                
                if total_dist < best_dist and mahal_dist < self.MAHALANOBIS_THRESHOLD:
                    best_dist = total_dist
                    best_match = det_idx
                    
            if best_match is not None and best_dist < self.COSINE_DISTANCE_THRESHOLD:
                matched.append((track_idx, best_match))
                unmatched_dets.remove(best_match)
            else:
                unmatched_tracks.append(track_idx)
                
        return matched, unmatched_dets, unmatched_tracks
        
    def _iou_match(self, unmatched_dets: List[int], unmatched_tracks: List[int], 
                   detections: List[Detection]) -> Tuple[List[Tuple[int, int]], List[int], List[int]]:
        """IOU匹配"""
        matched = []
        
        if not unmatched_dets or not unmatched_tracks:
            return matched, unmatched_dets, unmatched_tracks
            
        # 构建IOU矩阵
        iou_matrix = np.zeros((len(unmatched_tracks), len(unmatched_dets)))
        
        for i, track_idx in enumerate(unmatched_tracks):
            track = self.tracks[track_idx]
            for j, det_idx in enumerate(unmatched_dets):
                iou_matrix[i, j] = self._iou(
                    track.last_detection.bbox,
                    detections[det_idx].bbox
                )
                
        # 匈牙利算法匹配（简化实现使用贪婪）
        # 实际使用: scipy.optimize.linear_sum_assignment
        matched_tracks = set()
        matched_dets = set()
        
        while True:
            max_iou = self.IOU_THRESHOLD
            best_pair = None
            
            for i, track_idx in enumerate(unmatched_tracks):
                if track_idx in matched_tracks:
                    continue
                for j, det_idx in enumerate(unmatched_dets):
                    if det_idx in matched_dets:
                        continue
                    if iou_matrix[i, j] > max_iou:
                        max_iou = iou_matrix[i, j]
                        best_pair = (i, j, track_idx, det_idx)
                        
            if best_pair is None:
                break
                
            i, j, track_idx, det_idx = best_pair
            matched.append((track_idx, det_idx))
            matched_tracks.add(track_idx)
            matched_dets.add(det_idx)
            
        remaining_dets = [d for d in unmatched_dets if d not in matched_dets]
        remaining_tracks = [t for t in unmatched_tracks if t not in matched_tracks]
        
        return matched, remaining_dets, remaining_tracks
        
    def _mahalanobis_distance(self, track: Track, detection: Detection) -> float:
        """计算马氏距离"""
        # 简化的距离计算
        pred_center = (track.kalman_state[0], track.kalman_state[1])
        det_center = detection.bbox.center
        
        return np.sqrt((pred_center[0] - det_center[0])**2 + 
                      (pred_center[1] - det_center[1])**2)
        
    def _cosine_distance(self, f1: np.ndarray, f2: np.ndarray) -> float:
        """计算余弦距离"""
        return 1 - np.dot(f1, f2) / (np.linalg.norm(f1) * np.linalg.norm(f2))
        
    def _iou(self, box1: BoundingBox, box2: BoundingBox) -> float:
        """计算IOU"""
        x1 = max(box1.x1, box2.x1)
        y1 = max(box1.y1, box2.y1)
        x2 = min(box1.x2, box2.x2)
        y2 = min(box2.y2, box2.y2)
        
        if x2 <= x1 or y2 <= y1:
            return 0.0
            
        intersection = (x2 - x1) * (y2 - y1)
        union = box1.area + box2.area - intersection
        
        return intersection / union if union > 0 else 0
        
    def _update_track(self, track: Track, detection: Detection, feature: np.ndarray):
        """更新跟踪"""
        track.detections.append(detection)
        
        # 更新外观特征（EMA）
        alpha = 0.9
        track.features = alpha * track.features + (1 - alpha) * feature
        track.features = track.features / np.linalg.norm(track.features)
        
        # 更新Kalman状态（简化）
        u, v = detection.bbox.center
        s = detection.bbox.area
        r = detection.bbox.width / detection.bbox.height
        
        track.kalman_state[0] = u
        track.kalman_state[1] = v
        track.kalman_state[2] = s
        track.kalman_state[3] = r
        
        track.hits += 1
        track.time_since_update = 0
        
    def _create_track(self, detection: Detection, feature: np.ndarray):
        """创建新跟踪"""
        track = Track(
            track_id=self.next_track_id,
            detections=[detection],
            features=feature,
            kalman_state=None,
            age=1,
            time_since_update=0,
            is_confirmed=False,
            hits=1
        )
        
        self.tracks.append(track)
        self.next_track_id += 1
        
    def _delete_dead_tracks(self):
        """删除过期跟踪"""
        self.tracks = [
            t for t in self.tracks 
            if t.time_since_update <= self.MAX_AGE
        ]
        
    def get_track_by_id(self, track_id: int) -> Optional[Track]:
        """通过ID获取跟踪"""
        for track in self.tracks:
            if track.track_id == track_id:
                return track
        return None


class VisualServoController:
    """
    视觉伺服控制器
    
    基于图像的视觉伺服 (IBVS)
    控制目标：保持目标在图像中心，维持指定距离
    """
    
    def __init__(self, camera: CameraParameters):
        self.camera = camera
        
        # PID参数
        self.Kp_xy = 0.5   # 水平比例增益
        self.Ki_xy = 0.1   # 水平积分增益
        self.Kd_xy = 0.2   # 水平微分增益
        
        self.Kp_z = 0.3    # 垂直比例增益
        self.Kp_yaw = 0.2  # 偏航比例增益
        
        # 积分项
        self.integral_x = 0
        self.integral_y = 0
        
        # 上一时刻误差
        self.prev_ex = 0
        self.prev_ez = 0
        
        # 已知目标尺寸（用于距离估计）
        self.target_real_height = 1.7  # 人员平均身高 (米)
        
    def compute_control(self, track: Track, desired_distance: float = 30.0,
                       desired_height: float = 10.0) -> TrackingCommand:
        """
        计算视觉伺服控制指令
        
        Args:
            track: 跟踪目标
            desired_distance: 期望距离
            desired_height: 期望高度
            
        Returns:
            控制指令
        """
        det = track.last_detection
        bbox = det.bbox
        
        # 图像中心
        cx, cy = self.camera.image_center
        
        # 目标在图像中的位置误差
        tx, ty = bbox.center
        ex = tx - cx  # 水平误差（像素）
        ey = ty - cy  # 垂直误差（像素）
        
        # 距离估计
        current_distance = self._estimate_distance(bbox)
        ez = current_distance - desired_distance  # 距离误差
        
        # 跟踪质量评估
        tracking_quality = min(1.0, track.hits / 10)
        
        # 如果目标太小（太远），先接近
        if current_distance > desired_distance * 1.5:
            # 接近模式
            vx = 3.0  # 最大前进速度
            vy = -self.Kp_xy * ex * 0.01  # 水平对准
            vz = 0
            yaw_rate = -self.Kp_yaw * ex * 0.01
        else:
            # 跟踪模式 - PID控制
            
            # 积分项更新
            self.integral_x += ex
            self.integral_x = np.clip(self.integral_x, -1000, 1000)  # 抗积分饱和
            
            # 微分项
            deriv_x = ex - self.prev_ex
            deriv_z = ez - self.prev_ez
            
            # IBVS控制律
            # vx: 控制距离（前后移动）
            vx = -self.Kp_xy * ez - self.Kd_xy * deriv_z
            
            # vy: 控制水平位置（左右移动）
            vy = -self.Kp_xy * ex * 0.01 - self.Ki_xy * self.integral_x * 0.001
            
            # vz: 控制垂直位置（上下移动）
            # 结合高度控制和目标在图像中的垂直位置
            height_error = desired_height - current_distance * 0.5  # 简化
            vz = -self.Kp_z * ey * 0.01 - self.Kp_z * height_error * 0.1
            
            # yaw_rate: 保持机头指向目标
            yaw_rate = -self.Kp_yaw * ex * 0.01
            
        # 限制速度
        vx = np.clip(vx, -8.0, 8.0)
        vy = np.clip(vy, -5.0, 5.0)
        vz = np.clip(vz, -3.0, 3.0)
        yaw_rate = np.clip(yaw_rate, -1.0, 1.0)
        
        # 更新历史
        self.prev_ex = ex
        self.prev_ez = ez
        
        return TrackingCommand(
            vx=vx,
            vy=vy,
            vz=vz,
            yaw_rate=yaw_rate,
            distance_estimate=current_distance,
            tracking_quality=tracking_quality
        )
        
    def _estimate_distance(self, bbox: BoundingBox) -> float:
        """
        基于单目视觉的距离估计
        
        使用已知目标尺寸和相机内参
        distance = (focal_length * real_height) / pixel_height
        """
        pixel_height = bbox.height
        
        if pixel_height < 10:  # 避免除零
            return 100.0  # 默认远距离
            
        distance = (self.camera.fy * self.target_real_height) / pixel_height
        
        return distance
        
    def reset(self):
        """重置控制器"""
        self.integral_x = 0
        self.integral_y = 0
        self.prev_ex = 0
        self.prev_ez = 0


# API定义

"""
POST /api/v1/tracking/detections
    提交检测结果
    Body: {detections: List[Detection], frame: Image}
    Response: {tracks: List[Track]}

POST /api/v1/tracking/select-target
    选择跟踪目标
    Body: {track_id: int}
    Response: {success: bool}

POST /api/v1/tracking/compute-control
    计算跟踪控制
    Body: {track_id: int, desired_distance: float, desired_height: float}
    Response: TrackingCommand

GET /api/v1/tracking/status/{track_id}
    获取跟踪状态
    Response: {track: Track, quality: float, distance: float}
"""
