#ifndef __QUILTDB_CONFIG_PARSER_HPP__
#define __QUILTDB_CONFIG_PARSER_HPP__

#include <yaml-cpp/yaml.h>
#include <string>
#include <boost/unordered_map.hpp>

#include <quiltdb/include/common.hpp>

namespace quiltdb{

struct NodeConfig{
  NodeInfo node_info_;
  int32_t downstream_recv_;
  int32_t num_expected_props_;
};

class ConfigParser {
public:
  ConfigParser();
  ~ConfigParser();
  int LoadConfigFile(std::string _filename);
  NodeConfig GetNodeInfo(int32_t _node_id);
  bool GetParam(std::string _param, int32_t &_val);
  
private:
  boost::unordered_map<int32_t, NodeConfig> node_dir_;
  boost::unordered_map<std::string, int32_t> param_map_;
};
}

#endif
