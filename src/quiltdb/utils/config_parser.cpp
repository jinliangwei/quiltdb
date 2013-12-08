#include "config_parser.hpp"
#include <glog/logging.h>
#include <sstream>

namespace quiltdb {

ConfigParser::ConfigParser(){}
ConfigParser::~ConfigParser(){}

int ConfigParser::LoadConfigFile(std::string _filename){
  try{
    YAML::Node yaml_config = YAML::LoadFile(_filename);
    if(!yaml_config["NodeConfig"])
      LOG(FATAL) << "Didn't found NodeConfig";
    int num_nodes = yaml_config["NodeConfig"].size(); 
    int idx;
    for(idx = 0; idx < num_nodes; ++idx){
      VLOG(0) << "node " << idx;
      YAML::Node yaml_node = yaml_config["NodeConfig"][idx];
      NodeConfig node_config;
      {
	std::stringstream str_parser; 
	str_parser << yaml_node["id"].as<std::string>();
	str_parser >> node_config.node_info_.node_id_;
      }
      node_config.node_info_.recv_pull_ip_ 
	= yaml_node["recv-pull-ip"].as<std::string>();
      node_config.node_info_.recv_pull_port_ 
	= yaml_node["recv-pull-port"].as<std::string>();

      node_config.node_info_.recv_push_ip_ 
	= yaml_node["recv-push-ip"].as<std::string>();
      node_config.node_info_.recv_push_port_ 
	= yaml_node["recv-push-port"].as<std::string>();
      
      VLOG(0) << "num-props :" << yaml_node["num-props"].as<std::string>();

      {
	std::stringstream str_parser;
	str_parser << yaml_node["num-props"].as<std::string>();
	str_parser >> node_config.num_expected_props_;
      }
      
      {
	std::stringstream str_parser;
	str_parser << yaml_node["downstream-id"].as<std::string>();
	str_parser >> node_config.downstream_recv_;
      }

      VLOG(0) << "node id [" << node_config.node_info_.node_id_
	      << "] recv-pull-ip [" << node_config.node_info_.recv_pull_ip_
	      << "] recv-pull-port [" << node_config.node_info_.recv_pull_port_
	      << "] recv-push-ip [" << node_config.node_info_.recv_push_ip_
	      << "] recv-push-port [" << node_config.node_info_.recv_push_port_
	      << "] num-props [" << node_config.num_expected_props_
	      << "] downstream-recv [" << node_config.downstream_recv_ << "]";
      
      node_dir_[node_config.node_info_.node_id_] = node_config;

    }
    /*
    if(yaml_config["QuiltConfig"]){
      VLOG(0) << "Found NodeConfig";
      int num_params = yaml_config["QuiltConfig"].size(); 
      int idx;
      for(idx = 0; idx < num_params; ++idx){
	VLOG(0) << yaml_config["QuiltConfig"][idx].as<std::string>();
      }
      
    }
    */
  }catch(...){
    LOG(ERROR) << "parsing config failed!";
    return -1;
  }
  return 0;
}

NodeConfig ConfigParser::GetNodeInfo(int32_t _node_id){
  NodeConfig node;
  node.node_info_.node_id_ = -1;
  
  boost::unordered_map<int32_t, NodeConfig>::iterator node_iter
    = node_dir_.find(_node_id);
  if(node_iter != node_dir_.end()){
    node = node_iter->second;
  }
  return node;
}

bool ConfigParser::GetParam(std::string _param, int32_t &_val){
  
  return false;
}

}
