#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

class TopicPublisher01 : public rclcpp::Node
{
public:
    // 构造函数,有一个参数为节点名称
    TopicPublisher01(std::string name) : Node(name)
    {
        RCLCPP_INFO(this->get_logger(), "%s节点已经启动.", name.c_str());
        command_publisher = this->create_publisher<std_msgs::msg::String>("command", 10);
        timer = this->create_wall_timer(
            std::chrono::milliseconds(500), 
            std::bind(&TopicPublisher01::timer_callback, this)
        );
    }

private:
    // 声明节点
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr command_publisher;

    void timer_callback()
    {
        std_msgs::msg::String message;
        message.data = "forward";

        RCLCPP_INFO(this->get_logger(), "Publishing: %s", message.data.c_str());
        
        command_publisher->publish(message);
    }
    rclcpp::TimerBase::SharedPtr timer;

};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    /*创建对应节点的共享指针对象*/
    auto node = std::make_shared<TopicPublisher01>("topic_publisher_01");
    /* 运行节点，并检测退出信号*/
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
