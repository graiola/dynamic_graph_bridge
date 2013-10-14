#include "dynamic_graph_bridge/ros_interpreter.hh"

namespace dynamicgraph
{
  static const int queueSize = 5;

  Interpreter::Interpreter (ros::NodeHandle& nodeHandle)
    : interpreter_ (),
      nodeHandle_ (nodeHandle),
      runCommandSrv_ (),
      runPythonFileSrv_ ()
  {
  }

  void Interpreter::startRosService ()
  {
    runCommandCallback_t runCommandCb =
      boost::bind (&Interpreter::runCommandCallback, this, _1, _2);
    runCommandSrv_ =
      nodeHandle_.advertiseService ("run_command", runCommandCb);

    runPythonFileCallback_t runPythonFileCb =
      boost::bind (&Interpreter::runPythonFileCallback, this, _1, _2);
    runPythonFileSrv_ =
      nodeHandle_.advertiseService ("run_script", runPythonFileCb);
  }

  bool
  Interpreter::runCommandCallback
  (dynamic_graph_bridge::RunCommand::Request& req,
   dynamic_graph_bridge::RunCommand::Response& res)
  {
    interpreter_.python(req.input, res.result, res.stdout, res.stderr);
    return true;
  }

  bool
  Interpreter::runPythonFileCallback (dynamic_graph_bridge::RunPythonFile::Request& req,
                                      dynamic_graph_bridge::RunPythonFile::Response& res)
  {
    interpreter_.runPythonFile(req.input);
    res.result = "File parsed"; // FIX: It is just an echo, is there a way to have a feedback?
    return true;
  }

  std::string
  Interpreter::runCommand
  (const std::string& command)
  {
    return interpreter_.python(command);
  }

  void Interpreter::runCommand
  (const std::string & command, 
   std::string &result,
   std::string &out, 
   std::string &err)
  {
    interpreter_.python(command, result, out, err);
  }

  void Interpreter::runPythonFile( std::string ifilename ){
      interpreter_.runPythonFile(ifilename);
  }

} // end of namespace dynamicgraph.

