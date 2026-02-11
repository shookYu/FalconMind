"""
Multi-Objective Mission Assigner - 多目标优化任务分配
支持多目标优化和约束优化
"""

import random
import math
from typing import Dict, List, Tuple, Optional, Callable
from dataclasses import dataclass
import logging

logger = logging.getLogger(__name__)


@dataclass
class Point:
    """地理坐标点"""
    lat: float
    lon: float
    alt: float = 0.0


@dataclass
class Area:
    """区域定义"""
    polygon: List[Point]
    min_altitude: float = 0.0
    max_altitude: float = 100.0


@dataclass
class UavCapability:
    """UAV 能力信息"""
    uav_id: str
    max_altitude: float = 100.0
    max_speed: float = 15.0
    battery_capacity: float = 100.0
    current_battery: float = 100.0
    position: Optional[Point] = None
    current_mission_id: Optional[str] = None
    max_payload: float = 0.0  # 最大载重（kg）


@dataclass
class Constraint:
    """约束条件"""
    constraint_type: str  # "altitude", "battery", "distance", "payload", "time"
    min_value: Optional[float] = None
    max_value: Optional[float] = None
    target_value: Optional[float] = None


@dataclass
class Objective:
    """优化目标"""
    objective_type: str  # "minimize_cost", "maximize_battery", "minimize_time", "maximize_coverage"
    weight: float = 1.0  # 权重


class MultiObjectiveAssigner:
    """多目标优化任务分配器"""
    
    def __init__(
        self,
        objectives: List[Objective],
        constraints: List[Constraint] = None,
        population_size: int = 100,
        generations: int = 200
    ):
        self.objectives = objectives
        self.constraints = constraints or []
        self.population_size = population_size
        self.generations = generations
    
    def assign(
        self,
        mission_id: str,
        area: Area,
        num_uavs: int,
        available_uavs: List[UavCapability],
        mission_payload: float = 0.0
    ) -> List[str]:
        """
        多目标优化任务分配
        
        Args:
            mission_id: 任务 ID
            area: 搜索区域
            num_uavs: 需要的 UAV 数量
            available_uavs: 可用 UAV 列表
            mission_payload: 任务载荷（kg）
        
        Returns:
            分配的 UAV ID 列表
        """
        if len(available_uavs) < num_uavs:
            return [uav.uav_id for uav in available_uavs[:num_uavs]]
        
        # 使用 NSGA-II 算法（非支配排序遗传算法）
        population = self._initialize_population(available_uavs, num_uavs)
        
        for generation in range(self.generations):
            # 评估适应度（多目标）
            fitness_scores = [
                self._evaluate_multi_objective(individual, area, available_uavs, mission_payload)
                for individual in population
            ]
            
            # 非支配排序
            fronts = self._non_dominated_sort(population, fitness_scores)
            
            # 选择下一代（基于拥挤距离）
            new_population = []
            for front in fronts:
                if len(new_population) + len(front) <= self.population_size:
                    new_population.extend(front)
                else:
                    # 使用拥挤距离选择
                    remaining = self.population_size - len(new_population)
                    sorted_front = self._sort_by_crowding_distance(front, fitness_scores)
                    new_population.extend(sorted_front[:remaining])
                    break
            
            # 交叉和变异
            offspring = []
            while len(offspring) < self.population_size:
                parent1 = self._tournament_selection(new_population, fitness_scores)
                parent2 = self._tournament_selection(new_population, fitness_scores)
                child = self._crossover(parent1, parent2, available_uavs)
                child = self._mutate(child, available_uavs)
                offspring.append(child)
            
            population = offspring
        
        # 选择最优解（第一个非支配前沿的第一个个体）
        final_fitness = [
            self._evaluate_multi_objective(individual, area, available_uavs, mission_payload)
            for individual in population
        ]
        fronts = self._non_dominated_sort(population, final_fitness)
        if fronts and fronts[0]:
            best_individual = fronts[0][0]
            return [available_uavs[i].uav_id for i in best_individual]
        
        return [available_uavs[i].uav_id for i in population[0]]
    
    def _initialize_population(
        self,
        available_uavs: List[UavCapability],
        num_uavs: int
    ) -> List[List[int]]:
        """初始化种群"""
        population = []
        for _ in range(self.population_size):
            individual = random.sample(range(len(available_uavs)), num_uavs)
            population.append(individual)
        return population
    
    def _evaluate_multi_objective(
        self,
        individual: List[int],
        area: Area,
        available_uavs: List[UavCapability],
        mission_payload: float
    ) -> List[float]:
        """评估多目标适应度"""
        scores = []
        
        for objective in self.objectives:
            if objective.objective_type == "minimize_cost":
                # 最小化成本（基于电池使用）
                cost = sum(
                    (1.0 - available_uavs[i].current_battery / available_uavs[i].battery_capacity)
                    for i in individual
                )
                scores.append(cost * objective.weight)
            
            elif objective.objective_type == "maximize_battery":
                # 最大化电池剩余
                battery = sum(
                    available_uavs[i].current_battery / available_uavs[i].battery_capacity
                    for i in individual
                )
                scores.append(-battery * objective.weight)  # 负号因为要最小化
            
            elif objective.objective_type == "minimize_time":
                # 最小化时间（基于距离和速度）
                total_time = sum(
                    self._estimate_mission_time(available_uavs[i], area)
                    for i in individual
                )
                scores.append(total_time * objective.weight)
            
            elif objective.objective_type == "maximize_coverage":
                # 最大化覆盖（简化：基于 UAV 数量）
                coverage = len(individual)
                scores.append(-coverage * objective.weight)
        
        return scores
    
    def _estimate_mission_time(self, uav: UavCapability, area: Area) -> float:
        """估算任务时间（简化）"""
        # 简化：基于区域大小和 UAV 速度
        if not area.polygon:
            return 0.0
        
        # 计算区域大小（简化）
        min_lat = min(p.lat for p in area.polygon)
        max_lat = max(p.lat for p in area.polygon)
        min_lon = min(p.lon for p in area.polygon)
        max_lon = max(p.lon for p in area.polygon)
        
        # 估算距离（米）
        lat_diff = (max_lat - min_lat) * 111000  # 1度约111km
        lon_diff = (max_lon - min_lon) * 111000 * math.cos(math.radians((min_lat + max_lat) / 2))
        distance = math.sqrt(lat_diff ** 2 + lon_diff ** 2)
        
        # 估算时间（秒）
        if uav.max_speed > 0:
            time_seconds = distance / uav.max_speed
        else:
            time_seconds = 0.0
        
        return time_seconds
    
    def _check_constraints(
        self,
        individual: List[int],
        area: Area,
        available_uavs: List[UavCapability],
        mission_payload: float
    ) -> bool:
        """检查约束条件"""
        for constraint in self.constraints:
            if constraint.constraint_type == "altitude":
                for i in individual:
                    uav = available_uavs[i]
                    if constraint.max_value and uav.max_altitude > constraint.max_value:
                        return False
                    if constraint.min_value and uav.max_altitude < constraint.min_value:
                        return False
            
            elif constraint.constraint_type == "battery":
                for i in individual:
                    uav = available_uavs[i]
                    battery_percent = uav.current_battery / uav.battery_capacity
                    if constraint.min_value and battery_percent < constraint.min_value:
                        return False
            
            elif constraint.constraint_type == "payload":
                total_payload = sum(available_uavs[i].max_payload for i in individual)
                if constraint.min_value and total_payload < constraint.min_value:
                    return False
                if mission_payload > total_payload:
                    return False
        
        return True
    
    def _non_dominated_sort(
        self,
        population: List[List[int]],
        fitness_scores: List[List[float]]
    ) -> List[List[List[int]]]:
        """非支配排序"""
        fronts = []
        remaining = list(range(len(population)))
        
        while remaining:
            front = []
            for i in remaining:
                is_dominated = False
                for j in remaining:
                    if i == j:
                        continue
                    if self._dominates(fitness_scores[j], fitness_scores[i]):
                        is_dominated = True
                        break
                if not is_dominated:
                    front.append(population[i])
                    remaining.remove(i)
            
            if front:
                fronts.append(front)
            else:
                break
        
        return fronts
    
    def _dominates(self, fitness1: List[float], fitness2: List[float]) -> bool:
        """判断 fitness1 是否支配 fitness2"""
        # 所有目标都不差，且至少有一个更好
        all_better_or_equal = all(f1 <= f2 for f1, f2 in zip(fitness1, fitness2))
        at_least_one_better = any(f1 < f2 for f1, f2 in zip(fitness1, fitness2))
        return all_better_or_equal and at_least_one_better
    
    def _sort_by_crowding_distance(
        self,
        front: List[List[int]],
        fitness_scores: List[List[float]]
    ) -> List[List[int]]:
        """按拥挤距离排序"""
        if len(front) <= 2:
            return front
        
        # 计算拥挤距离（简化实现）
        distances = [0.0] * len(front)
        
        for obj_idx in range(len(fitness_scores[0])):
            # 按当前目标排序
            sorted_indices = sorted(
                range(len(front)),
                key=lambda i: fitness_scores[i][obj_idx]
            )
            
            # 边界个体距离为无穷
            distances[sorted_indices[0]] = float('inf')
            distances[sorted_indices[-1]] = float('inf')
            
            # 计算中间个体的距离
            obj_min = fitness_scores[sorted_indices[0]][obj_idx]
            obj_max = fitness_scores[sorted_indices[-1]][obj_idx]
            obj_range = obj_max - obj_min if obj_max != obj_min else 1.0
            
            for i in range(1, len(sorted_indices) - 1):
                idx = sorted_indices[i]
                prev_idx = sorted_indices[i - 1]
                next_idx = sorted_indices[i + 1]
                distance = (
                    (fitness_scores[next_idx][obj_idx] - fitness_scores[prev_idx][obj_idx]) / obj_range
                )
                distances[idx] += distance
        
        # 按距离降序排序
        sorted_front = sorted(
            zip(front, distances),
            key=lambda x: x[1],
            reverse=True
        )
        
        return [individual for individual, _ in sorted_front]
    
    def _tournament_selection(
        self,
        population: List[List[int]],
        fitness_scores: List[List[float]],
        tournament_size: int = 2
    ) -> List[int]:
        """锦标赛选择"""
        tournament_indices = random.sample(range(len(population)), tournament_size)
        # 简化：选择第一个非支配的
        return population[tournament_indices[0]].copy()
    
    def _crossover(
        self,
        parent1: List[int],
        parent2: List[int],
        available_uavs: List[UavCapability]
    ) -> List[int]:
        """交叉操作"""
        if len(parent1) < 2:
            return parent1.copy()
        
        point = random.randint(1, len(parent1) - 1)
        child = parent1[:point] + parent2[point:]
        
        # 去重并补充
        child = list(dict.fromkeys(child))
        while len(child) < len(parent1):
            remaining = [i for i in range(len(available_uavs)) if i not in child]
            if remaining:
                child.append(random.choice(remaining))
            else:
                break
        
        return child[:len(parent1)]
    
    def _mutate(
        self,
        individual: List[int],
        available_uavs: List[UavCapability],
        mutation_rate: float = 0.1
    ) -> List[int]:
        """变异操作"""
        if random.random() > mutation_rate:
            return individual
        
        if len(individual) == 0:
            return individual
        
        # 随机替换一个 UAV
        idx = random.randint(0, len(individual) - 1)
        remaining = [i for i in range(len(available_uavs)) if i not in individual]
        if remaining:
            individual[idx] = random.choice(remaining)
        
        return individual
