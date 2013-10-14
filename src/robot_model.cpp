#include <limits>

#include <boost/bind.hpp>

#include <ros/ros.h>

#include <dynamic-graph/command-getter.h>
#include <dynamic-graph/factory.h>
#include <dynamic-graph/null-ptr.hh>
#include <jrl/dynamics/urdf/parser.hh>

#include "dynamic_graph_bridge/ros_init.hh"
#include "robot_model.hh"

namespace dynamicgraph
{
namespace dg = dynamicgraph;
using ::dynamicgraph::command::Getter;

namespace command
{
LoadFromParameterServer::LoadFromParameterServer
(RosRobotModel& entity,
 const std::string& docstring)
    : Command (entity, std::vector<Value::Type>(), docstring)
{}

Value
LoadFromParameterServer::doExecute()
{
    RosRobotModel& entity =
            static_cast<RosRobotModel&>(owner ());
    entity.loadFromParameterServer ();
    return Value ();
}

LoadUrdf::LoadUrdf(RosRobotModel& entity, const std::string& docstring)
    : Command (entity, std::vector<Value::Type>(), docstring)
{}

Value
LoadUrdf::doExecute()
{
    RosRobotModel& entity =
            static_cast<RosRobotModel&>(owner ());

    const std::vector<Value>& values = getParameterValues ();
    std::string resourceName = values[0].value ();

    entity.loadUrdf (resourceName);
    return Value ();
}
} // end of namespace command.

RosRobotModel::RosRobotModel (const std::string& name)
    : Dynamic(name,false),
      parameterName_ ("jrl_map"),
      lastComputation_ (std::numeric_limits<int>::min())
{

    std::string docstring;

    docstring =
            "\n"
            "  Load the robot model from the parameter server.\n"
            "\n"
            "  This is the recommended method.\n"
            "\n";
    addCommand ("loadFromParameterServer",
                new command::LoadFromParameterServer (*this, docstring));
    docstring =
            "\n"
            "  Load the robot model from an URDF file.\n"
            "\n";
    addCommand ("loadUrdf", new command::LoadUrdf (*this, docstring));

    docstring =
            "\n"
            "  Get current configuration of the robot.\n"
            "\n";
    addCommand ("curConf", new command::Getter<RosRobotModel,Vector> (*this,&RosRobotModel::curConf,docstring));
}

RosRobotModel::~RosRobotModel ()
{}

void
RosRobotModel::loadUrdf (const std::string& filename)
{
    jrl::dynamics::urdf::Parser parser;
    m_HDR = parser.parse(filename);

    ros::NodeHandle nh;
    nh.setParam(parameterName_, parser.JointsNamesByRank_);
}

void
RosRobotModel::loadFromParameterServer ()
{
    jrl::dynamics::urdf::Parser parser;

    rosInit (false);
    std::string robotDescription;
    ros::param::param<std::string>
            ("robot_description", robotDescription, "");
    if (robotDescription.empty ())
        throw std::runtime_error
            ("No model available as ROS parameter. Fail.");
    m_HDR = parser.parseStream (robotDescription);

    ros::NodeHandle nh;
    nh.setParam(parameterName_, parser.JointsNamesByRank_);

}

namespace
{

vectorN convertVector (const ml::Vector& v)
{
    vectorN res (v.size());
    for (unsigned i = 0; i < v.size(); ++i)
        res[i] = v(i);
    return res;
}

ml::Vector convertVector (const vectorN& v)
{
    ml::Vector res;
    res.resize(v.size());
    for (unsigned i = 0; i < v.size(); ++i)
        res(i) = v[i];
    return res;
}

} // end of anonymous namespace.

Vector RosRobotModel::curConf() const
{
    if (!m_HDR )
        throw std::runtime_error ("no robot loaded");
    else {
        vectorN currConf = m_HDR->currentConfiguration();
        Vector res;
        res = convertVector(currConf);
        return res;
    }
}

DYNAMICGRAPH_FACTORY_ENTITY_PLUGIN(RosRobotModel, "RosRobotModel");
} // end of namespace dynamicgraph.
