"""
ML Load Predictor - 机器学习负载预测
使用 LSTM 和 Transformer 模型进行负载预测
"""

import numpy as np
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass
from datetime import datetime, timedelta
from collections import deque
import logging

logger = logging.getLogger(__name__)

# 尝试导入机器学习库
try:
    import torch
    import torch.nn as nn
    TORCH_AVAILABLE = True
except ImportError:
    TORCH_AVAILABLE = False
    logger.warning("PyTorch not available, ML models disabled")

try:
    from sklearn.preprocessing import MinMaxScaler
    SKLEARN_AVAILABLE = True
except ImportError:
    SKLEARN_AVAILABLE = False
    logger.warning("scikit-learn not available, scaling disabled")


@dataclass
class LoadSequence:
    """负载序列"""
    timestamp: datetime
    mission_count: int
    battery_usage: float
    cpu_usage: float
    memory_usage: float


class LSTMLoadPredictor:
    """LSTM 负载预测器"""
    
    def __init__(
        self,
        input_size: int = 4,  # mission_count, battery, cpu, memory
        hidden_size: int = 64,
        num_layers: int = 2,
        sequence_length: int = 10,
        prediction_horizon: int = 1
    ):
        if not TORCH_AVAILABLE:
            raise RuntimeError("PyTorch not available")
        
        self.input_size = input_size
        self.hidden_size = hidden_size
        self.num_layers = num_layers
        self.sequence_length = sequence_length
        self.prediction_horizon = prediction_horizon
        
        # LSTM 模型
        self.model = nn.LSTM(
            input_size=input_size,
            hidden_size=hidden_size,
            num_layers=num_layers,
            batch_first=True
        )
        self.fc = nn.Linear(hidden_size, input_size * prediction_horizon)
        
        # 数据归一化
        self.scaler = MinMaxScaler() if SKLEARN_AVAILABLE else None
        
        # 训练状态
        self.trained = False
        self.history: deque = deque(maxlen=1000)
    
    def prepare_sequences(self, data: List[LoadSequence]) -> Tuple[np.ndarray, np.ndarray]:
        """准备训练序列"""
        if len(data) < self.sequence_length + self.prediction_horizon:
            return None, None
        
        # 提取特征
        features = np.array([
            [seq.mission_count, seq.battery_usage, seq.cpu_usage, seq.memory_usage]
            for seq in data
        ])
        
        # 归一化
        if self.scaler:
            features = self.scaler.fit_transform(features)
        
        # 创建序列
        X, y = [], []
        for i in range(len(features) - self.sequence_length - self.prediction_horizon + 1):
            X.append(features[i:i + self.sequence_length])
            y.append(features[i + self.sequence_length:i + self.sequence_length + self.prediction_horizon])
        
        return np.array(X), np.array(y)
    
    def train(self, data: List[LoadSequence], epochs: int = 100, lr: float = 0.001):
        """训练模型"""
        if not TORCH_AVAILABLE:
            logger.error("PyTorch not available for training")
            return
        
        X, y = self.prepare_sequences(data)
        if X is None or len(X) == 0:
            logger.error("Insufficient data for training")
            return
        
        # 转换为 PyTorch 张量
        X_tensor = torch.FloatTensor(X)
        y_tensor = torch.FloatTensor(y).reshape(len(y), -1)
        
        # 优化器
        optimizer = torch.optim.Adam(self.model.parameters(), lr=lr)
        criterion = nn.MSELoss()
        
        # 训练
        self.model.train()
        for epoch in range(epochs):
            optimizer.zero_grad()
            output, _ = self.model(X_tensor)
            output = self.fc(output[:, -1, :])
            loss = criterion(output, y_tensor)
            loss.backward()
            optimizer.step()
            
            if epoch % 10 == 0:
                logger.info(f"Epoch {epoch}, Loss: {loss.item():.4f}")
        
        self.trained = True
        logger.info("LSTM model trained successfully")
    
    def predict(self, history: List[LoadSequence]) -> Optional[np.ndarray]:
        """预测未来负载"""
        if not self.trained:
            logger.warning("Model not trained")
            return None
        
        if len(history) < self.sequence_length:
            logger.warning("Insufficient history for prediction")
            return None
        
        # 准备输入
        features = np.array([
            [seq.mission_count, seq.battery_usage, seq.cpu_usage, seq.memory_usage]
            for seq in history[-self.sequence_length:]
        ])
        
        # 归一化
        if self.scaler:
            features = self.scaler.transform(features)
        
        # 预测
        self.model.eval()
        with torch.no_grad():
            X_tensor = torch.FloatTensor(features).unsqueeze(0)
            output, _ = self.model(X_tensor)
            prediction = self.fc(output[:, -1, :])
            prediction = prediction.numpy().reshape(self.prediction_horizon, self.input_size)
        
        # 反归一化
        if self.scaler:
            prediction = self.scaler.inverse_transform(prediction)
        
        return prediction
    
    def add_data_point(self, sequence: LoadSequence):
        """添加数据点"""
        self.history.append(sequence)
    
    def update_model(self, epochs: int = 10):
        """更新模型（增量学习）"""
        if len(self.history) < self.sequence_length + self.prediction_horizon:
            return
        
        data = list(self.history)
        self.train(data, epochs=epochs)


class TransformerLoadPredictor:
    """Transformer 负载预测器（简化实现）"""
    
    def __init__(
        self,
        input_size: int = 4,
        d_model: int = 64,
        nhead: int = 4,
        num_layers: int = 2,
        sequence_length: int = 10,
        prediction_horizon: int = 1
    ):
        if not TORCH_AVAILABLE:
            raise RuntimeError("PyTorch not available")
        
        self.input_size = input_size
        self.d_model = d_model
        self.sequence_length = sequence_length
        self.prediction_horizon = prediction_horizon
        
        # Transformer 编码器
        self.input_projection = nn.Linear(input_size, d_model)
        encoder_layer = nn.TransformerEncoderLayer(
            d_model=d_model,
            nhead=nhead,
            dim_feedforward=d_model * 4,
            batch_first=True
        )
        self.transformer = nn.TransformerEncoder(encoder_layer, num_layers=num_layers)
        self.output_projection = nn.Linear(d_model, input_size * prediction_horizon)
        
        # 数据归一化
        self.scaler = MinMaxScaler() if SKLEARN_AVAILABLE else None
        
        # 训练状态
        self.trained = False
        self.history: deque = deque(maxlen=1000)
    
    def prepare_sequences(self, data: List[LoadSequence]) -> Tuple[np.ndarray, np.ndarray]:
        """准备训练序列"""
        if len(data) < self.sequence_length + self.prediction_horizon:
            return None, None
        
        # 提取特征
        features = np.array([
            [seq.mission_count, seq.battery_usage, seq.cpu_usage, seq.memory_usage]
            for seq in data
        ])
        
        # 归一化
        if self.scaler:
            features = self.scaler.fit_transform(features)
        
        # 创建序列
        X, y = [], []
        for i in range(len(features) - self.sequence_length - self.prediction_horizon + 1):
            X.append(features[i:i + self.sequence_length])
            y.append(features[i + self.sequence_length:i + self.sequence_length + self.prediction_horizon])
        
        return np.array(X), np.array(y)
    
    def train(self, data: List[LoadSequence], epochs: int = 100, lr: float = 0.001):
        """训练模型"""
        if not TORCH_AVAILABLE:
            logger.error("PyTorch not available for training")
            return
        
        X, y = self.prepare_sequences(data)
        if X is None or len(X) == 0:
            logger.error("Insufficient data for training")
            return
        
        # 转换为 PyTorch 张量
        X_tensor = torch.FloatTensor(X)
        y_tensor = torch.FloatTensor(y).reshape(len(y), -1)
        
        # 优化器
        optimizer = torch.optim.Adam(
            list(self.input_projection.parameters()) +
            list(self.transformer.parameters()) +
            list(self.output_projection.parameters()),
            lr=lr
        )
        criterion = nn.MSELoss()
        
        # 训练
        self.input_projection.train()
        self.transformer.train()
        self.output_projection.train()
        
        for epoch in range(epochs):
            optimizer.zero_grad()
            
            # 投影输入
            x_proj = self.input_projection(X_tensor)
            
            # Transformer 编码
            encoded = self.transformer(x_proj)
            
            # 使用最后一个时间步的输出
            output = self.output_projection(encoded[:, -1, :])
            
            loss = criterion(output, y_tensor)
            loss.backward()
            optimizer.step()
            
            if epoch % 10 == 0:
                logger.info(f"Epoch {epoch}, Loss: {loss.item():.4f}")
        
        self.trained = True
        logger.info("Transformer model trained successfully")
    
    def predict(self, history: List[LoadSequence]) -> Optional[np.ndarray]:
        """预测未来负载"""
        if not self.trained:
            logger.warning("Model not trained")
            return None
        
        if len(history) < self.sequence_length:
            logger.warning("Insufficient history for prediction")
            return None
        
        # 准备输入
        features = np.array([
            [seq.mission_count, seq.battery_usage, seq.cpu_usage, seq.memory_usage]
            for seq in history[-self.sequence_length:]
        ])
        
        # 归一化
        if self.scaler:
            features = self.scaler.transform(features)
        
        # 预测
        self.input_projection.eval()
        self.transformer.eval()
        self.output_projection.eval()
        
        with torch.no_grad():
            X_tensor = torch.FloatTensor(features).unsqueeze(0)
            x_proj = self.input_projection(X_tensor)
            encoded = self.transformer(x_proj)
            prediction = self.output_projection(encoded[:, -1, :])
            prediction = prediction.numpy().reshape(self.prediction_horizon, self.input_size)
        
        # 反归一化
        if self.scaler:
            prediction = self.scaler.inverse_transform(prediction)
        
        return prediction
    
    def add_data_point(self, sequence: LoadSequence):
        """添加数据点"""
        self.history.append(sequence)
    
    def update_model(self, epochs: int = 10):
        """更新模型（增量学习）"""
        if len(self.history) < self.sequence_length + self.prediction_horizon:
            return
        
        data = list(self.history)
        self.train(data, epochs=epochs)
