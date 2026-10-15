# 机器人建模总结

本工作空间完成了对于一个差速轮控制移动的粗糙小车的建模。对应的模型文件是 [urdf/fishbot_base.urdf](urdf/fishbot_base.urdf)，用 Rviz2 可以把它显示出来并让轮子转起来。

## 什么是机器人建模

- 机器人建模 = 用文件把机器人的**物理结构**描述出来，让 ROS2 知道它长什么样、关节在哪里、各部件之间是什么关系。
- ROS2 中用URDF来描述机器人，本质是一个`XML`文件。
- 建模的核心是把机器人拆成两部分：
  - Link（连杆/刚体）：机器人上不会变形的部件，比如车身、轮子、雷达。
  - Joint（关节）：连接两个 Link 的"关节"，决定了它们之间怎么相对运动。
- 建模的意义：描述文件写好后，Rviz2 能显示模型、robot_state_publisher 能发布各个部件的坐标变换（TF），后续导航、仿真都依赖它。

## 建模整体流程

1. **明确机器人的结构和坐标系**：先想清楚机器人有哪些部件、谁连着谁、坐标系怎么定。
2. **创建功能包**（描述类的功能包）：
   ```bash
   ros2 pkg create fishbot_description --build-type ament_python
   ```
   描述包一般不含逻辑代码，只放 urdf/launch/config 等文件。
3. **编写 URDF**：先写所有 link，再写所有 joint，把它们串成一棵"连杆树"。
4. **可视化验证**：用 Rviz2 + robot_state_publisher 把模型显示出来，检查各部件位置、朝向对不对。
5. **让关节动起来**：发布 `/joint_states` 话题，给关节提供角度/速度数据。
6. **构建并运行**：`colcon build` → `source` → `ros2 launch`。

## URDF 核心语法

### Link 部分

一个 link 主要描述它的**外观（visual）**，完整的还应有 collision（碰撞体积）和 inertial（质量惯量），本项目只写了 visual：

```xml
<link name="base_link">
    <visual>
        <origin xyz="0 0 0.0" rpy="0 0 0"/>        <!-- 相对该 link 的位姿 -->
        <geometry>
            <cylinder length="0.12" radius="0.10"/> <!-- 形状和尺寸。此处是圆柱 -->
        </geometry>
        <material name="base_blue">                 <!-- 颜色 -->
            <color rgba="0.0 0.0 1.0 0.5" />
        </material>
    </visual>
</link>
```

三种常用几何体：

| 形状 | 标签 | 参数 | 注意 |
|------|------|------|------|
| 长方体 | `<box>` | `size="x y z"` | 三个方向长度 |
| 圆柱体 | `<cylinder>` | `length` `radius` | **轴线默认沿 z 轴**， 想要修改则前面 `origin rpy = "1.57079 0 0"` |
| 球体 | `<sphere>` | `radius` | |

颜色 `rgba` 是 r/g/b/a 四个 **0~1** 的值(和OpenCV不一样)，a 是透明度，本项目全用的 0.5（半透明）。

### Joint 部分

joint 把两个 link 连起来，**一个关节只能连一个 parent 和一个 child**，构成父子关系：

```xml
<joint name="right_wheel_joint" type="continuous">
    <parent link="base_link" />
    <child link="right_wheel_link" />
    <origin xyz="-0.02 -0.10 -0.06" />   <!-- child 相对 parent 的位置 -->
    <axis xyz="0 1 0" />                  <!-- 旋转轴，只对可动关节有意义 -->
</joint>
```

关节类型：

| 类型 | 含义 | 本项目用在哪 |
|------|------|--------------|
| `fixed` | 固定连接，不能动 | 激光雷达、IMU、万向轮、base_link |
| `continuous` | 无限旋转，绕 axis 转 | 左右车轮 |
| `revolute` | 有限角度旋转（带上下限） | 机械臂关节 |
| `prismatic` | 直线滑动关节（带行程限制） | 滑台 |

**连杆树**：所有 link 通过 joint 串成一棵树，有且只有一个"根"。本项目的树是：

```
base_footprint（根，底盘在地面的投影）
    └── base_link（车身）
        ├── laser_link（激光雷达）
        ├── imu_link（IMU）
        ├── left_wheel_link / right_wheel_link（左右轮）
        └── caster_link（万向轮）
```

## 本项目建模拆解

### 各 Link 一览

| link | 形状 | 尺寸(m) | 颜色 | 说明 |
|------|------|---------|------|------|
| base_link | 圆柱 | 长0.12 半径0.10 | 蓝色 | 车身 |
| laser_link | 圆柱 | 长0.02 半径0.02 | 绿色 | 激光雷达 |
| imu_link | 方块 | 0.02³ | 紫色 | IMU |
| left/right_wheel_link | 圆柱 | 长0.04 半径0.032 | 黑色 | 车轮 |
| caster_link | 球 | 半径0.016 | 黑色 | 万向轮 |
| base_footprint | 无 | 无 | — | 底盘投影（纯坐标点，无几何体） |

### 各 Joint 一览

| joint | 类型 | parent→child | origin (xyz) | 说明 |
|-------|------|--------------|--------------|------|
| base_joint | fixed | base_footprint→base_link | 0 0 0.076 | 把车身抬离地面 |
| laser_joint | fixed | base_link→laser_link | 0 0 0.075 | 雷达装在车身顶部 |
| imu_joint | fixed | base_link→imu_link | 0 0 0.02 | IMU 装在车身上方 |
| left_wheel_joint | continuous | base_link→left_wheel_link | -0.02 **0.10** -0.06 | 左轮，axis 绕 y 轴 |
| right_wheel_joint | continuous | base_link→right_wheel_link | -0.02 **-0.10** -0.06 | 右轮，axis 绕 y 轴 |
| caster_joint | fixed | base_link→caster_link | 0.06 0 -0.076 | 前方万向轮（防倒） |

### 几个关键的坑

- **坐标系约定**：机器人坐标系遵循 REP 103——x 朝前、y 朝左、z 朝上。所以 y 负半轴是**右侧**，这就是为什么 `right_wheel` 的 y 是 -0.10 而 `left_wheel` 是 0.10，别写反了。
- **车轮的朝向**：`<cylinder>` 的轴线默认沿 z，而车轮要"立"起来绕左右方向（y 轴）转。所以车轮 visual 里写了 `rpy="1.57079 0 0"`（绕 x 转 90°），把圆柱的轴从 z 掰到 y；对应 joint 的 `<axis xyz="0 1 0"/>` 表示绕 y 轴旋转。两者要一致，否则显示和运动对不上。
- **base_footprint 是什么**：它是底盘在地面上的投影点，没有任何几何体，纯粹是一个坐标系。它位于地面（z=0），机器人所有部件都以它为根。导航、建图时机器人"在哪"指的就是它。

## 让模型显示出来、动起来

### 可视化需要的三个角色

| 节点 | 作用 |
|------|------|
| `robot_state_publisher` | 读取 URDF，发布各 link 之间的**静态 TF 变换**，并持续发布 `/robot_description` |
| `joint_state_publisher(_gui)` | 发布 `/joint_states`（各关节角度），gui 版带滑条可以手动拖 |
| `rviz2` | 显示模型，订阅 TF 把各部件摆到对应位置 |

三者配合：robot_state_publisher 发布"各部件相对关系"，joint_state_publisher 发布"关节当前角度"，rviz2 把两者合起来渲染出模型。缺了任何一个，模型要么不显示要么不会动。

### 让轮子自己转：自定义节点

本项目的 [fishbot_description/robot_state_publisher.py](fishbot_description/robot_state_publisher.py) 自己发布 `/joint_states`，让左右轮以 30Hz 持续转动，替代了 joint_state_publisher_gui。要点：

- `JointState` 消息的核心字段：
  - `name`：关节名列表，**必须和 URDF 里的 joint 名字完全一致**（这里就是 `left_wheel_joint`、`right_wheel_joint`），对不上轮子就不会动；
  - `position` / `velocity` / `effort`：角度、角速度、力矩，与 name 一一对应；
  - `header.stamp`：时间戳。
- 发布方式：开一个线程循环发布，`position += delta_time * velocity` 做数值积分，再 `create_rate(30)` 控制频率。
- 差速转向：`main` 里 `update_speed([15.0, -15.0])` 让左轮正转、右轮反转，两轮速度相反 → 小车原地打转。差速小车就是这么转弯的。

### Launch 文件

[launch/display_rviz2.launch.py](launch/display_rviz2.launch.py) 一次启动 4 个节点。要点：

- `FindPackageShare(package).find(package)` 没有不要写死绝对路径，而是找到`install`后 `share/fishbot_description` 的绝对路径，再拼出 urdf 和 rviz 配置文件的路径。
- `robot_state_publisher` 用 `arguments=[urdf_model_path]` 把 URDF 文件路径作为参数传入。
- rviz2 用 `-d` 参数加载配置文件 `config/display_robot_model.rviz`。这个文件是第一次打开 rviz2 时手动添加 RobotModel、TF 显示后 `File → Save Config As` 保存出来的，下次就不用再点了。

## 常用命令

```bash
colcon build              # 在工作空间根目录构建
source install/setup.bash # 刷新环境
ros2 launch fishbot_description display_rviz2.launch.py   # 一键启动
```

调试相关：

```bash
check_urdf urdf/fishbot_base.urdf            # 检查 URDF 语法和树结构（装 urdfdom-tools）
urdf_to_graphviz urdf/fishbot_base.urdf      # 生成连杆树图
ros2 topic echo /joint_states                # 查看关节数据
```

**注意**：urdf、launch、config 这些文件是通过 [setup.py](setup.py) 的 `data_files` 拷贝到 `install/share/...` 里的，改了它们之后**必须重新 `colcon build`**，否则运行的还是旧文件。开发时可以用 `colcon build --symlink-install` 建立软链接，改文件就不用反复编译。

## 总结与下一步

- 建模流程一句话：**想结构 → 建包 → 写 link → 写 joint → rviz2 验证 → 发布 joint_states 驱动**。
- 本项目只写了 `visual`，没有 `collision` 和 `inertial`，所以只能看、不能做物理仿真；下一步补上质量惯量后就能进 Gazebo 仿真了。
- 后续进阶：用 **xacro** 宏来消除重复代码（左右轮这种对称件）、导入真实的 mesh 网格模型替代圆柱方块、添加相机/雷达插件等。
