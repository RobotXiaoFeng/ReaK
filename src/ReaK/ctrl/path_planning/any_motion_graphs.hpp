/**
 * \file any_motion_graphs.hpp
 *
 * This library provides a type-erased classes for motion graphs.
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date July 2013
 */

/*
 *    Copyright 2013 Sven Mikael Persson
 *
 *    THIS SOFTWARE IS DISTRIBUTED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE v3 (GPLv3).
 *
 *    This file is part of ReaK.
 *
 *    ReaK is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    ReaK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with ReaK (as LICENSE in the root folder).  
 *    If not, sMOTION_ee <http://www.gnu.org/licenses/>.
 */

#ifndef RK_ANY_MOTION_GRAPHS_HPP
#define RK_ANY_MOTION_GRAPHS_HPP

#include "graph_alg/any_graph.hpp"

#include "metric_space_concepts.hpp"
#include "steerable_space_concept.hpp"

namespace ReaK {

namespace pp {


namespace detail {
  
  template <typename Topology, typename Graph>
  typename boost::enable_if< is_steerable_space<Topology>,
  steerable_space_traits<Topology> >::type::steer_record_type& 
    try_get_steer_record(Graph& g, typename boost::graph_traits< Graph >::edge_descriptor e) {
    return g[e].steer_record;
  };
  
  template <typename Topology, typename Graph>
  typename boost::disable_if< is_steerable_space<Topology>,
  int >::type try_get_steer_record(Graph&, typename boost::graph_traits< Graph >::edge_descriptor) {
    throw std::invalid_argument("Required property 'edge_steer_record' on a non-steerable space!");
  };
  
  
  template <typename Topology, bool IsSteerable>
  struct mg_edge_data_base { };
  
  template <typename Topology>
  struct mg_edge_data_base<Topology,true> { 
    typename steerable_space_traits<Topology>::steer_record_type steer_record;
  };
  
  
};




/**
 * This struct contains the data required on a per-vertex basis for any basic path-planning algorithm.
 * \tparam Topology The topology type on which the planning is performed.
 */
template <typename Topology>
struct mg_vertex_data {
  /// The position associated to the vertex.
  typename topology_traits<Topology>::point_type position;
};

/**
 * This struct contains the data required on a per-edge basis for any basic path-planning algorithm.
 * \tparam Topology The topology type on which the planning is performed.
 */
template <typename Topology>
struct mg_edge_data : detail::mg_edge_data_base<Topology, is_steerable_space<Topology>::value > { };



/**
 * This class template can be used as a type-erased encapsulation of a graph used by any basic path-planning algorithm.
 * \tparam Topology The topology type on which the planning is performed.
 * \tparam Graph The graph type used by the path-planning algorithm.
 */
template <typename Topology, typename Graph>
class any_motion_graph : public graph::type_erased_graph<Graph> {
  protected:
    typedef any_motion_graph<Topology, Graph> self;
    typedef graph::type_erased_graph<Graph> base_type;
    typedef typename boost::graph_traits<Graph>::vertex_descriptor real_vertex_desc;
    typedef typename boost::graph_traits<Graph>::edge_descriptor real_edge_desc;
    
    virtual void* get_property_by_ptr(const std::string& aProperty, const boost::any& aElement) const {
      
      if(aProperty == "vertex_position")
        return static_cast<void*>(&((*(this->p_graph))[boost::any_cast<real_vertex_desc>(aElement)].position));
      if(aProperty == "edge_steer_record")
        return static_cast<void*>( &( detail::try_get_steer_record<Topology>(*(this->p_graph), boost::any_cast<real_edge_desc>(aElement)) ) );
      
      return base_type::get_property_by_ptr(aProperty, aElement);
    };
    
    virtual boost::any get_property_by_any(const std::string& aProperty, const boost::any& aElement) const {
      
      if(aProperty == "vertex_position")
        return boost::any((*(this->p_graph))[boost::any_cast<real_vertex_desc>(aElement)].position);
      if(aProperty == "edge_steer_record")
        return boost::any( detail::try_get_steer_record<Topology>(*(this->p_graph), boost::any_cast<real_edge_desc>(aElement)) );
      
      return base_type::get_property_by_any(aProperty, aElement);
    };
    
  public:
    
    any_motion_graph(Graph* aPGraph) : base_type(aPGraph) { };
    
};





/**
 * This struct contains the data required on a per-vertex basis for any optimal path-planning algorithm.
 * \tparam Topology The topology type on which the planning is performed.
 */
template <typename Topology>
struct optimal_mg_vertex : mg_vertex_data<Topology> {
  /// The travel-distance accumulated in the vertex, i.e., the travel-distance from the root vertex to this vertex.
  double distance_accum;
  /// The predecessor associated to the vertex, e.g., following the predecessor links starting at the goal node yields a backward trace of the optimal path.
  std::size_t predecessor;
};

/**
 * This struct contains the data required on a per-edge basis for any optimal path-planning algorithm.
 * \tparam Topology The topology type on which the planning is performed.
 */
template <typename Topology>
struct optimal_mg_edge : mg_edge_data<Topology> {
  /// The travel-distance associated to the edge (from source to target).
  double weight;
};



/**
 * This class template can be used as a type-erased encapsulation of a graph used by any optimal path-planning algorithm.
 * \tparam Topology The topology type on which the planning is performed.
 * \tparam Graph The graph type used by the path-planning algorithm.
 */
template <typename Topology, typename Graph>
class any_optimal_motion_graph : public any_motion_graph<Topology, Graph> {
  protected:
    typedef any_optimal_motion_graph<Topology, Graph> self;
    typedef any_motion_graph<Topology, Graph> base_type;
    typedef typename boost::graph_traits<Graph>::vertex_descriptor real_vertex_desc;
    typedef typename boost::graph_traits<Graph>::edge_descriptor real_edge_desc;
    
    virtual void* get_property_by_ptr(const std::string& aProperty, const boost::any& aElement) const {
      
      if(aProperty == "vertex_distance_accum")
        return static_cast<void*>(&((*(this->p_graph))[boost::any_cast<real_vertex_desc>(aElement)].distance_accum));
      if(aProperty == "vertex_predecessor")
        return static_cast<void*>(&((*(this->p_graph))[boost::any_cast<real_vertex_desc>(aElement)].predecessor));
      if(aProperty == "edge_weight")
        return static_cast<void*>(&((*(this->p_graph))[boost::any_cast<real_edge_desc>(aElement)].weight));
      
      return base_type::get_property_by_ptr(aProperty, aElement);
    };
    
    virtual boost::any get_property_by_any(const std::string& aProperty, const boost::any& aElement) const {
      
      if(aProperty == "vertex_distance_accum")
        return boost::any((*(this->p_graph))[boost::any_cast<real_vertex_desc>(aElement)].distance_accum);
      if(aProperty == "vertex_predecessor")
        return boost::any((*(this->p_graph))[boost::any_cast<real_vertex_desc>(aElement)].predecessor);
      if(aProperty == "edge_weight")
        return boost::any((*(this->p_graph))[boost::any_cast<real_edge_desc>(aElement)].weight);
      
      return base_type::get_property_by_any(aProperty, aElement);
    };
    
  public:
    
    any_optimal_motion_graph(Graph* aPGraph) : base_type(aPGraph) { };
    
};






/**
 * This struct contains the data required on a per-vertex basis for any A*-like path-planning algorithm (heuristically driven).
 * \tparam Topology The topology type on which the planning is performed.
 */
template <typename Topology>
struct astar_mg_vertex : optimal_mg_vertex<Topology> {
  /// The heuristic-value associated to the vertex, i.e., the bird-flight distance to the goal.
  double heuristic_value;
  /// The key-value associated to the vertex, computed by the algorithm (usually a combination of accumulated and heuristic distances).
  double key_value;
};

/**
 * This struct contains the data required on a per-edge basis for any A*-like path-planning algorithm (heuristically driven).
 * \tparam Topology The topology type on which the planning is performed.
 */
template <typename Topology>
struct astar_mg_edge : optimal_mg_edge<Topology> { };


/**
 * This class template can be used as a type-erased encapsulation of a graph used by any A*-like path-planning algorithm.
 * \tparam Topology The topology type on which the planning is performed.
 * \tparam Graph The graph type used by the path-planning algorithm.
 */
template <typename Topology, typename Graph>
class any_astar_motion_graph : public any_optimal_motion_graph<Topology, Graph> {
  protected:
    typedef any_astar_motion_graph<Topology, Graph> self;
    typedef any_optimal_motion_graph<Topology, Graph> base_type;
    typedef typename boost::graph_traits<Graph>::vertex_descriptor real_vertex_desc;
    typedef typename boost::graph_traits<Graph>::edge_descriptor real_edge_desc;
    
    virtual void* get_property_by_ptr(const std::string& aProperty, const boost::any& aElement) const {
      
      if(aProperty == "vertex_heuristic_value")
        return static_cast<void*>(&((*(this->p_graph))[boost::any_cast<real_vertex_desc>(aElement)].heuristic_value));
      if(aProperty == "vertex_key_value")
        return static_cast<void*>(&((*(this->p_graph))[boost::any_cast<real_vertex_desc>(aElement)].key_value));
      
      return base_type::get_property_by_ptr(aProperty, aElement);
    };
    
    virtual boost::any get_property_by_any(const std::string& aProperty, const boost::any& aElement) const {
      
      if(aProperty == "vertex_heuristic_value")
        return boost::any((*(this->p_graph))[boost::any_cast<real_vertex_desc>(aElement)].heuristic_value);
      if(aProperty == "vertex_key_value")
        return boost::any((*(this->p_graph))[boost::any_cast<real_vertex_desc>(aElement)].key_value);
      
      return base_type::get_property_by_any(aProperty, aElement);
    };
    
  public:
    
    any_astar_motion_graph(Graph* aPGraph) : base_type(aPGraph) { };
    
};




};

};


#endif


