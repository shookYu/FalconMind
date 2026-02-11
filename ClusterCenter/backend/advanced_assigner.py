"""
Advanced Mission Assigner - 高级任务分配算法
遗传算法、粒子群优化等
"""

import random
import math
from typing import Dict, List, Tuple, Optional
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


@dataclass
class Assignment:
    """任务分配方案"""
    uav_id: str
    area: Area
    cost: float  # 分配成本（越低越好）


class GeneticAlgorithmAssigner:
    """基于遗传算法的任务分配器"""
    
    def __init__(
        self,
        population_size: int = 50,
        generations: int = 100,
        mutation_rate: float = 0.1,
        crossover_rate: float = 0.8,
        elite_size: int = 5
    ):
        self.population_size = population_size
        self.generations = generations
        self.mutation_rate = mutation_rate
        self.crossover_rate = crossover_rate
        self.elite_size = elite_size
    
    def assign(
        self,
        mission_id: str,
        area: Area,
        num_uavs: int,
        available_uavs: List[UavCapability]
    ) -> List[str]:
        """
        使用遗传算法分配任务
        
        Args:
            mission_id: 任务 ID
            area: 搜索区域
            num_uavs: 需要的 UAV 数量
            available_uavs: 可用 UAV 列表
        
        Returns:
            分配的 UAV ID 列表
        """
        if len(available_uavs) < num_uavs:
            return [uav.uav_id for uav in available_uavs[:num_uavs]]
        
        # 初始化种群
        population = self._initialize_population(available_uavs, num_uavs)
        
        # 进化
        for generation in range(self.generations):
            # 评估适应度
            fitness_scores = [self._fitness(individual, area) for individual in population]
            
            # 选择精英
            elite_indices = sorted(
                range(len(fitness_scores)),
                key=lambda i: fitness_scores[i],
                reverse=True
            )[:self.elite_size]
            elite = [population[i] for i in elite_indices]
            
            # 生成新种群
            new_population = elite.copy()
            
            while len(new_population) < self.population_size:
                # 选择父代
                parent1 = self._tournament_selection(population, fitness_scores)
                parent2 = self._tournament_selection(population, fitness_scores)
                
                # 交叉
                if random.random() < self.crossover_rate:
                    child = self._crossover(parent1, parent2, available_uavs)
                else:
                    child = parent1.copy()
                
                # 变异
                if random.random() < self.mutation_rate:
                    child = self._mutate(child, available_uavs)
                
                new_population.append(child)
            
            population = new_population
        
        # 选择最佳个体
        final_fitness = [self._fitness(individual, area) for individual in population]
        best_index = max(range(len(final_fitness)), key=lambda i: final_fitness[i])
        best_individual = population[best_index]
        
        return [available_uavs[i].uav_id for i in best_individual]
    
    def _initialize_population(
        self,
        available_uavs: List[UavCapability],
        num_uavs: int
    ) -> List[List[int]]:
        """初始化种群（每个个体是一个 UAV 索引列表）"""
        population = []
        for _ in range(self.population_size):
            individual = random.sample(range(len(available_uavs)), num_uavs)
            population.append(individual)
        return population
    
    def _fitness(self, individual: List[int], area: Area) -> float:
        """计算适应度（越高越好）"""
        # 简化适应度：基于 UAV 能力和位置
        # 实际应该考虑任务完成时间、能耗等
        score = 0.0
        for idx in individual:
            # 这里需要访问 available_uavs，简化处理
            score += 1.0
        return score
    
    def _tournament_selection(
        self,
        population: List[List[int]],
        fitness_scores: List[float],
        tournament_size: int = 3
    ) -> List[int]:
        """锦标赛选择"""
        tournament_indices = random.sample(range(len(population)), tournament_size)
        tournament_fitness = [fitness_scores[i] for i in tournament_indices]
        winner_index = tournament_indices[max(
            range(len(tournament_fitness)),
            key=lambda i: tournament_fitness[i]
        )]
        return population[winner_index].copy()
    
    def _crossover(
        self,
        parent1: List[int],
        parent2: List[int],
        available_uavs: List[UavCapability]
    ) -> List[int]:
        """交叉操作"""
        # 单点交叉
        if len(parent1) < 2:
            return parent1.copy()
        
        point = random.randint(1, len(parent1) - 1)
        child = parent1[:point] + parent2[point:]
        
        # 去重并补充
        child = list(dict.fromkeys(child))  # 保持顺序去重
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
        available_uavs: List[UavCapability]
    ) -> List[int]:
        """变异操作"""
        if len(individual) == 0:
            return individual
        
        # 随机替换一个 UAV
        idx = random.randint(0, len(individual) - 1)
        remaining = [i for i in range(len(available_uavs)) if i not in individual]
        if remaining:
            individual[idx] = random.choice(remaining)
        
        return individual


class ParticleSwarmOptimizer:
    """基于粒子群优化的任务分配器"""
    
    def __init__(
        self,
        swarm_size: int = 30,
        iterations: int = 100,
        w: float = 0.7,  # 惯性权重
        c1: float = 1.5,  # 个体学习因子
        c2: float = 1.5   # 社会学习因子
    ):
        self.swarm_size = swarm_size
        self.iterations = iterations
        self.w = w
        self.c1 = c1
        self.c2 = c2
    
    def assign(
        self,
        mission_id: str,
        area: Area,
        num_uavs: int,
        available_uavs: List[UavCapability]
    ) -> List[str]:
        """
        使用粒子群优化分配任务
        
        Args:
            mission_id: 任务 ID
            area: 搜索区域
            num_uavs: 需要的 UAV 数量
            available_uavs: 可用 UAV 列表
        
        Returns:
            分配的 UAV ID 列表
        """
        if len(available_uavs) < num_uavs:
            return [uav.uav_id for uav in available_uavs[:num_uavs]]
        
        # 初始化粒子群
        particles = []
        global_best = None
        global_best_fitness = float('-inf')
        
        for _ in range(self.swarm_size):
            # 随机初始化位置（UAV 索引列表）
            position = random.sample(range(len(available_uavs)), num_uavs)
            velocity = [random.uniform(-1, 1) for _ in range(num_uavs)]
            fitness = self._fitness(position, area, available_uavs)
            
            particle = {
                'position': position,
                'velocity': velocity,
                'best_position': position.copy(),
                'best_fitness': fitness
            }
            particles.append(particle)
            
            if fitness > global_best_fitness:
                global_best_fitness = fitness
                global_best = position.copy()
        
        # 迭代优化
        for iteration in range(self.iterations):
            for particle in particles:
                # 更新速度
                for i in range(len(particle['position'])):
                    r1 = random.random()
                    r2 = random.random()
                    
                    # 速度更新（简化：基于索引差异）
                    pos_diff_personal = self._position_diff(
                        particle['position'][i],
                        particle['best_position'][i] if i < len(particle['best_position']) else particle['position'][i],
                        len(available_uavs)
                    )
                    pos_diff_global = self._position_diff(
                        particle['position'][i],
                        global_best[i] if i < len(global_best) else particle['position'][i],
                        len(available_uavs)
                    )
                    
                    particle['velocity'][i] = (
                        self.w * particle['velocity'][i] +
                        self.c1 * r1 * pos_diff_personal +
                        self.c2 * r2 * pos_diff_global
                    )
                    
                    # 更新位置（简化：基于速度选择 UAV）
                    if abs(particle['velocity'][i]) > 0.5:
                        # 随机替换
                        remaining = [j for j in range(len(available_uavs)) if j not in particle['position']]
                        if remaining:
                            particle['position'][i] = random.choice(remaining)
                
                # 评估适应度
                fitness = self._fitness(particle['position'], area, available_uavs)
                
                # 更新个体最优
                if fitness > particle['best_fitness']:
                    particle['best_fitness'] = fitness
                    particle['best_position'] = particle['position'].copy()
                
                # 更新全局最优
                if fitness > global_best_fitness:
                    global_best_fitness = fitness
                    global_best = particle['position'].copy()
        
        return [available_uavs[i].uav_id for i in global_best]
    
    def _fitness(
        self,
        position: List[int],
        area: Area,
        available_uavs: List[UavCapability]
    ) -> float:
        """计算适应度"""
        # 简化适应度：基于 UAV 能力
        score = 0.0
        for idx in position:
            uav = available_uavs[idx]
            # 电池得分
            battery_score = uav.current_battery / uav.battery_capacity
            # 高度能力得分
            altitude_score = min(1.0, uav.max_altitude / area.max_altitude) if area.max_altitude > 0 else 1.0
            score += battery_score * 0.6 + altitude_score * 0.4
        
        return score / len(position) if position else 0.0
    
    def _position_diff(self, pos1: int, pos2: int, max_pos: int) -> float:
        """计算位置差异（归一化）"""
        if max_pos == 0:
            return 0.0
        return (pos2 - pos1) / max_pos
