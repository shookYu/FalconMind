#include <iostream>
#include <vector>
#include <cmath>
struct Vec2 { double x; double y; };
static void runObstacleAvoidanceDemo(){ std::vector<Vec2> obstacles = { {4.0,0.5},{7.0,-0.6},{9.0,0.0} }; double px=0.0, py=0.0, vx=0.8, targetX=10.0; std::cout << "[ObstacleAvoidance] rk1126b 模拟" << std::endl; for(int s=0;s<100;++s){ double nx=px+vx, ny=py; bool blocked=false; for(auto &ob: obstacles){ double dx=nx-px, dy=ny-py; double t=((ob.x-px)*dx+(ob.y-py)*dy)/(dx*dx+dy*dy+1e-9); if(t<0) t=0; if(t>1) t=1; double projx=px+t*dx, projy=py+t*dy; double dist = std::hypot(ob.x-projx, ob.y-projy); if(dist<1.3){ blocked=true; break; } } if(blocked) py+=1.0; else px=nx; if(px>=targetX-0.5){ std::cout<<"目标到达附近"<<std::endl; break; } }
