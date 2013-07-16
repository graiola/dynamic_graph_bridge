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

  namespace
  {
    void convert (ml::Vector& dst, const vector3d& src)
    {
      dst(0) = src[0];
      dst(1) = src[1];
      dst(2) = src[2];
    }
  } // end of anonymous namespace.

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
        topicName_ ("jrl_map"),
        lastComputation_ (std::numeric_limits<int>::min())/*,
        q_ (dynamicgraph::nullptr,
            "RosRobotModel(" + name + ")::input(vector)::position"),
        dq_ (dynamicgraph::nullptr,
             "RosRobotModel(" + name + ")::input(vector)::velocity"),
        ddq_ (dynamicgraph::nullptr,
              "RosRobotModel(" + name + ")::input(vector)::acceleration"),
        zmp_ (boost::bind (&RosRobotModel::computeZmp, this, _1, _2),
              0,
              "RosRobotModel(" + name + ")::output(vector)::zmp"),
        com_ (boost::bind (&RosRobotModel::computeCom, this, _1, _2),
              0,
              "RosRobotModel(" + name + ")::output(vector)::com"),
        jcom_ (boost::bind (&RosRobotModel::computeJCom, this, _1, _2),
               0,
               "RosRobotModel(" + name + ")::output(vector)::Jcom"),
        lowerJointLimits_
        (boost::bind
         (&RosRobotModel::computeLowerJointLimits, this, _1, _2),
         0,
         "RosRobotModel(" + name + ")::output(vector)::lowerJl"),
        upperJointLimits_
        (boost::bind
         (&RosRobotModel::computeUpperJointLimits, this, _1, _2),
         0,
         "RosRobotModel(" + name + ")::output(vector)::upperJl")
    */
  {
      /*
      signalRegistration(q_);
      signalRegistration(dq_);
      signalRegistration(ddq_);
      signalRegistration(zmp_);
      signalRegistration(com_);
      signalRegistration(jcom_);
      signalRegistration(lowerJointLimits_);
      signalRegistration(upperJointLimits_);

      zmp_.addDependency (q_);
      zmp_.addDependency (dq_);
      zmp_.addDependency (ddq_);
      com_.addDependency (q_);
      com_.addDependency (dq_);
      com_.addDependency (ddq_);
      jcom_.addDependency (q_);
      jcom_.addDependency (dq_);
      jcom_.addDependency (ddq_);
      */
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
      /*
      docstring = "    \n"
              "    Get the dimension of the robot configuration.\n"
              "    \n"
              "      Return:\n"
              "        an unsigned int: the dimension.\n"
              "    \n";
      addCommand ("getDimension",
                  new Getter<RosRobotModel, unsigned>
                  (*this,
                   &RosRobotModel::getDimension, docstring));
      */
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
    nh.setParam(topicName_, parser.JointsNamesByRank_);

    //buildSignals ();
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
    nh.setParam(topicName_, parser.JointsNamesByRank_);

    //buildSignals ();
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

  void RosRobotModel::buildSignals ()
  {
      // iterate on tree nodes and add signals
      typedef dg::SignalTimeDependent<MatrixHomogeneous, int> signal_t;
      ::std::vector<CjrlJoint*> jointsVect = m_HDR->jointVector();

      for (uint i=0; i<jointsVect.size(); ++i)
      {
          CjrlJoint* currentJoint = jointsVect[i];
          std::string signame = currentJoint->getName();

          signal_t* sig
                  = new signal_t
                  (boost::bind
                   (&RosRobotModel::computeBodyPosition, this, currentJoint, _1, _2),
                   0,
                   "sotDynamic(" + getName () + ")::output(matrix)::" + signame);
          sig->addDependency (jointPositionSIN);
          sig->addDependency (jointVelocitySIN);
          sig->addDependency (jointAccelerationSIN);
          genericSignalRefs_.push_back (sig);
          signalRegistration (*sig);
      }
      // force recomputation as the model has changed.
      lastComputation_ = std::numeric_limits<int>::min();
  }

  RosRobotModel::MatrixHomogeneous& RosRobotModel::computeBodyPosition (CjrlJoint* joint, MatrixHomogeneous& position, int t)
  {
      update (t);
      for(unsigned i = 0; i < position.nbRows(); ++i)
          for(unsigned j = 0; j < position.nbCols(); ++j){
              position.elementAt(i,j) =
                      joint->currentTransformation().m[i * position.nbCols() + j];
          }
      return position;
  }

  void RosRobotModel::update (int t)
  {

      if (t <= lastComputation_)
          return;

      vectorN q = convertVector (jointPositionSIN (t));
      vectorN dq = convertVector (jointVelocitySIN (t));
      vectorN ddq = convertVector (jointAccelerationSIN (t));

      if (!m_HDR->currentConfiguration(q))
          throw std::runtime_error ("failed to update current configuration");
      if (!m_HDR->currentVelocity(dq))
          throw std::runtime_error ("failed to update current velocity");
      if (!m_HDR->currentAcceleration(ddq))
          throw std::runtime_error ("failed to update current acceleration");
      m_HDR->computeForwardKinematics();
      lastComputation_ = t;
  }

  ml::Vector& RosRobotModel::computeZmp (ml::Vector& zmp, int t)
  {
      zmp.resize(3);
      if (!m_HDR)
          throw std::runtime_error ("no robot");
      update (t);
      convert (zmp, m_HDR->zeroMomentumPoint());
      return zmp;
  }

  ml::Vector& RosRobotModel::computeCom (ml::Vector& com, int t)
  {
      com.resize(3);
      if (!m_HDR)
          throw std::runtime_error ("no robot");
      update (t);
      convert (com, m_HDR->positionCenterOfMass());
      return com;
  }

  ml::Matrix& RosRobotModel::computeJCom (ml::Matrix& jcom, int t)
  {
      jcom.resize(3, getDimension ());
      if (!m_HDR)
          throw std::runtime_error ("no robot");
      update (t);
      CjrlJoint* rootJoint = m_HDR->rootJoint();
      if (!rootJoint)
          throw std::runtime_error ("no root joint");
      matrixNxP jacobian;
      m_HDR->getJacobianCenterOfMass (*rootJoint, jacobian);
      jcom.initFromMotherLib (jacobian);
      return jcom;
  }

  ml::Vector& RosRobotModel::computeLowerJointLimits (ml::Vector& lowerJointLimits, int t)
  {
      if (!m_HDR)
          throw std::runtime_error ("no robot");

      const unsigned int& ndofs = m_HDR->numberDof ();
      lowerJointLimits.resize (ndofs);

      for (unsigned int i = 0; i< ndofs; ++i)
          lowerJointLimits (i) = m_HDR->lowerBoundDof (i);
      return lowerJointLimits;
  }

  ml::Vector& RosRobotModel::computeUpperJointLimits (ml::Vector& upperJointLimits, int t)
  {
      if (!m_HDR)
          throw std::runtime_error ("no robot");

      const unsigned int& ndofs = m_HDR->numberDof ();
      upperJointLimits.resize (ndofs);

      for (unsigned int i = 0; i< ndofs; ++i)
          upperJointLimits (i) = m_HDR->upperBoundDof (i);
      return upperJointLimits;
  }

  DYNAMICGRAPH_FACTORY_ENTITY_PLUGIN(RosRobotModel, "RosRobotModel");
  } // end of namespace dynamicgraph.
