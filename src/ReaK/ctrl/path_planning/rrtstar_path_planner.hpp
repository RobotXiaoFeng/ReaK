/**
 * \file rrtstar_path_planner.hpp
 * 
 * This library defines a class
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date August 2012
 */

/*
 *    Copyright 2012 Sven Mikael Persson
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
 *    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REAK_RRTSTAR_PATH_PLANNER_HPP
#define REAK_RRTSTAR_PATH_PLANNER_HPP

#include "base/defs.hpp"
#include "base/named_object.hpp"

#include "motion_planner_base.hpp"
#include "sbmp_reporter_concept.hpp"

#include "metric_space_concept.hpp"
#include "path_base.hpp"
#include "interpolation/point_to_point_path.hpp"
#include "basic_sbmp_reporters.hpp"

#include "graph_alg/rrt_star.hpp"

#include "graph_alg/d_ary_bf_tree.hpp"
#include "graph_alg/d_ary_cob_tree.hpp"
#include "graph_alg/bgl_tree_adaptor.hpp"
#include "graph_alg/bgl_more_property_maps.hpp"
#include "dvp_layout_adjacency_list.hpp"
#include "metric_space_search.hpp"
#include "topological_search.hpp"
#include "path_planner_options.hpp"
#include "rrt_path_planner.hpp"
#include "lin_alg/arithmetic_tuple.hpp"

namespace ReaK {
  
namespace pp {
  


template <typename FreeSpaceType>
struct rrtstar_vertex_data {
  typename topology_traits<FreeSpaceType>::point_type position;
  double distance_accum;
  std::size_t predecessor;
  
  rrtstar_vertex_data() : position(typename topology_traits<FreeSpaceType>::point_type()),
                          distance_accum(0.0), predecessor(0) { };
};

template <typename FreeSpaceType>
struct rrtstar_edge_data {
  double weight;
  
  rrtstar_edge_data() : weight(0) { };
};





/**
 * This class is a RRT-based path-planner over the given topology.
 * \tparam FreeSpaceType The topology type on which to perform the planning, should be the C-free sub-space of a larger configuration space.
 * \tparam SBPPReporter The reporter type to use to report the progress of the path-planning.
 */
template <typename FreeSpaceType, 
          typename SBPPReporter = no_sbmp_report>
class rrtstar_path_planner : public sample_based_planner< path_planner_base<FreeSpaceType> > {
  public:
    typedef sample_based_planner< path_planner_base<FreeSpaceType> > base_type;
    typedef rrtstar_path_planner<FreeSpaceType, SBPPReporter> self;
    
    typedef FreeSpaceType space_type;
    typedef typename subspace_traits<FreeSpaceType>::super_space_type super_space_type;
    
    BOOST_CONCEPT_ASSERT((SubSpaceConcept<FreeSpaceType>));
    
    typedef typename topology_traits< super_space_type >::point_type point_type;
    typedef typename topology_traits< super_space_type >::point_difference_type point_difference_type;
    
  protected:
    SBPPReporter m_reporter;
    point_type m_start_pos;
    point_type m_goal_pos;
    std::size_t max_num_results;
    bool has_reached_max_vertices;
    std::size_t m_bidir_flag;
    std::size_t m_graph_kind_flag;
    std::size_t m_knn_flag;
    
    std::map<double, shared_ptr< path_base< super_space_type > > > m_solutions;
    
  public:
    
    template <typename Vertex, typename Graph>
    void check_goal_connection(const point_type& p_u, Vertex u, Graph& g) {
      point_type result_p = this->m_space->move_position_toward(p_u, 1.0, m_goal_pos);
      double best_case_dist = get(distance_metric, this->m_space->get_super_space())(p_u, m_goal_pos, this->m_space->get_super_space());
      double actual_dist = get(distance_metric, this->m_space->get_super_space())(p_u, result_p, this->m_space->get_super_space());
      
      if(actual_dist < 0.99 * best_case_dist)
        return;
      
      double solutions_total_dist = actual_dist + g[u].distance_accum;
//       if(solutions_total_dist >= m_solutions.begin()->first)
//         return;
      
      shared_ptr< super_space_type > sup_space_ptr(&(this->m_space->get_super_space()),null_deleter());
      shared_ptr< path_wrapper< point_to_point_path<super_space_type> > > new_sol(new path_wrapper< point_to_point_path<super_space_type> >("rrt_solution", point_to_point_path<super_space_type>(sup_space_ptr,get(distance_metric, this->m_space->get_super_space()))));
      point_to_point_path<super_space_type>& waypoints = new_sol->get_underlying_path();
      
      waypoints.push_front(m_goal_pos);
      waypoints.push_front(p_u);
      
      while(g[u].predecessor != Graph::null_vertex()) {
        u = g[u].predecessor;
        waypoints.push_front(g[u].position);
      };
      
      m_solutions[solutions_total_dist] = new_sol;
      m_reporter.draw_solution(*(this->m_space), m_solutions[solutions_total_dist]);
    };
    
    template <typename Vertex, typename Graph>
    void joining_vertex_found(Vertex u1, Vertex u2, Graph& g1, Graph& g2) {
      double total_dist = g1[u1].distance_accum + g2[u2].distance_accum
        + get(distance_metric, this->m_space->get_super_space())(g1[u1].position, g2[u2].position, this->m_space->get_super_space());
      
//       if(total_dist >= m_solutions.begin()->first)
//         return;
      
      shared_ptr< super_space_type > sup_space_ptr(&(this->m_space->get_super_space()),null_deleter());
      shared_ptr< path_wrapper< point_to_point_path<super_space_type> > > new_sol(new path_wrapper< point_to_point_path<super_space_type> >("birrt_solution", point_to_point_path<super_space_type>(sup_space_ptr,get(distance_metric, this->m_space->get_super_space()))));
      point_to_point_path<super_space_type>& waypoints = new_sol->get_underlying_path();
      
      waypoints.push_front(g1[u1].position);
      while(g1[u1].predecessor != Graph::null_vertex()) {
        u1 = g1[u1].predecessor;
        waypoints.push_front(g1[u1].position);
      };
      
      waypoints.push_back(g2[u2].position);
      while(g2[u2].predecessor != Graph::null_vertex()) {
        u2 = g2[u2].predecessor;
        waypoints.push_back(g2[u2].position);
      };
      
      m_solutions[total_dist] = new_sol;
      m_reporter.draw_solution(*(this->m_space), m_solutions[total_dist]);
    };
    
    bool keep_going() const {
      return (max_num_results > m_solutions.size()) && !has_reached_max_vertices;
    };
    
    template <typename Graph>
    void report_progress(Graph& g) {
      if(num_vertices(g) % this->m_progress_interval == 0)
        m_reporter.draw_motion_graph(*(this->m_space), g, get(&rrtstar_vertex_data<FreeSpaceType>::position,g));
      has_reached_max_vertices = (num_vertices(g) >= this->m_max_vertex_count);
    };
    
    template <typename Graph>
    void report_progress(Graph& g1, Graph& g2) {
      if((num_vertices(g1) + num_vertices(g2)) % this->m_progress_interval == 0) {
        m_reporter.draw_motion_graph(*(this->m_space), g1, get(&rrtstar_vertex_data<FreeSpaceType>::position,g1));
        m_reporter.draw_motion_graph(*(this->m_space), g2, get(&rrtstar_vertex_data<FreeSpaceType>::position,g2));
      };
      has_reached_max_vertices = (num_vertices(g1) + num_vertices(g2) >= this->m_max_vertex_count);
    };
    
    /**
     * This function computes a valid path in the C-free. If it cannot 
     * achieve a valid path, an exception will be thrown. This algorithmic
     * path solver class is such that any settings that ought to be set for the 
     * path planning algorithm should be set before calling this function, otherwise
     * the function is likely to fail.
     * \return The path object that can be used to map out the path.
     */
    virtual shared_ptr< path_base< super_space_type > > solve_path();
    
    const SBPPReporter& get_reporter() const { return m_reporter; };
    void set_reporter(const SBPPReporter& aNewReporter) { m_reporter = aNewReporter; };
    
    const point_type& get_start_pos() const { return m_start_pos; };
    void set_start_pos(const point_type& aStartPos) { m_start_pos = aStartPos; };
    
    const point_type& get_goal_pos() const { return m_goal_pos; };
    void set_goal_pos(const point_type& aGoalPos) { m_goal_pos = aGoalPos; };
    
    std::size_t get_max_result_count() const { return max_num_results; };
    void set_max_result_count(std::size_t aMaxResultCount) { max_num_results = aMaxResultCount; };
    
    std::size_t get_bidir_flag() const { return m_bidir_flag; };
    void set_bidir_flag(std::size_t aBidirFlag) { m_bidir_flag = aBidirFlag; };
    
    std::size_t get_graph_kind_flag() const { return m_graph_kind_flag; };
    void set_graph_kind_flag(std::size_t aGraphKindFlag) { m_graph_kind_flag = aGraphKindFlag; };
    
    std::size_t get_knn_flag() const { return m_knn_flag; };
    void set_knn_flag(std::size_t aKNNMethodFlag) { m_knn_flag = aKNNMethodFlag; };
    
    
    /**
     * Parametrized constructor.
     * \param aWorld A topology which represents the C-free (obstacle-free configuration space).
     * \param aStartPos The position value of the starting location.
     * \param aGoalPos The position value of the goal location.
     * \param aMaxVertexCount The maximum number of samples to generate during the motion planning.
     * \param aProgressInterval The number of new samples between each "progress report".
     * \param aBiDirFlag An integer flag representing the directionality of the RRT algorithm 
     *                   used (either UNIDIRECTIONAL_RRT or BIDIRECTIONAL_RRT).
     * \param aGraphKindFlag An integer flag representing the kind of motion graph to use in the 
     *                       RRT algorithm. Can be ADJ_LIST_MOTION_GRAPH or DVP_ADJ_LIST_MOTION_GRAPH.
     * \param aKNNMethodFlag An integer flag representing the kind of KNN method to use for nearest
     *                       neighbor queries in the graph. Can be LINEAR_SEARCH_KNN, DVP_BF2_TREE_KNN,
     *                       DVP_BF4_TREE_KNN, DVP_COB2_TREE_KNN, or DVP_COB4_TREE_KNN when the 
     *                       motion graph is of kind ADJ_LIST_MOTION_GRAPH. Can be DVP_ALT_BF2_TREE_KNN,
     *                       DVP_ALT_BF4_TREE_KNN, DVP_ALT_COB2_TREE_KNN, or DVP_ALT_COB4_TREE_KNN when 
     *                       the motion graph is of kind DVP_ADJ_LIST_MOTION_GRAPH.
     * \param aReporter The SBPP reporter object to use to report results and progress.
     * \param aMaxResultCount The maximum number of successful start-goal connections to make before 
     *                        stopping the path planner (the higher the number the more likely that a 
     *                        good path will be found, however, running time can become much longer).
     */
    rrtstar_path_planner(const shared_ptr< space_type >& aWorld = shared_ptr< space_type >(), 
                         const point_type& aStartPos = point_type(),
                         const point_type& aGoalPos = point_type(),
                         std::size_t aMaxVertexCount = 5000, 
                         std::size_t aProgressInterval = 100,
                         std::size_t aBiDirFlag = BIDIRECTIONAL_RRT,
                         std::size_t aGraphKindFlag = ADJ_LIST_MOTION_GRAPH,
                         std::size_t aKNNMethodFlag = DVP_BF2_TREE_KNN,
                         SBPPReporter aReporter = SBPPReporter(),
                         std::size_t aMaxResultCount = 50) :
                         base_type("rrtstar_planner", aWorld, aMaxVertexCount, aProgressInterval),
                         m_reporter(aReporter),
                         m_start_pos(aStartPos),
                         m_goal_pos(aGoalPos),
                         max_num_results(aMaxResultCount),
                         has_reached_max_vertices(false),
                         m_bidir_flag(aBiDirFlag),
                         m_graph_kind_flag(aGraphKindFlag),
                         m_knn_flag(aKNNMethodFlag),
                         m_solutions() { };
    
    virtual ~rrtstar_path_planner() { };
    
/*******************************************************************************
                   ReaK's RTTI and Serialization interfaces
*******************************************************************************/

    virtual void RK_CALL save(serialization::oarchive& A, unsigned int) const {
      base_type::save(A,base_type::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_SAVE_WITH_NAME(m_reporter)
        & RK_SERIAL_SAVE_WITH_NAME(m_start_pos)
        & RK_SERIAL_SAVE_WITH_NAME(m_goal_pos)
        & RK_SERIAL_SAVE_WITH_NAME(max_num_results)
        & RK_SERIAL_SAVE_WITH_NAME(m_bidir_flag)
        & RK_SERIAL_SAVE_WITH_NAME(m_graph_kind_flag)
        & RK_SERIAL_SAVE_WITH_NAME(m_knn_flag);
    };

    virtual void RK_CALL load(serialization::iarchive& A, unsigned int) {
      base_type::load(A,base_type::getStaticObjectType()->TypeVersion());
      A & RK_SERIAL_LOAD_WITH_NAME(m_reporter)
        & RK_SERIAL_LOAD_WITH_NAME(m_start_pos)
        & RK_SERIAL_LOAD_WITH_NAME(m_goal_pos)
        & RK_SERIAL_LOAD_WITH_NAME(max_num_results)
        & RK_SERIAL_LOAD_WITH_NAME(m_bidir_flag)
        & RK_SERIAL_LOAD_WITH_NAME(m_graph_kind_flag)
        & RK_SERIAL_LOAD_WITH_NAME(m_knn_flag);
      has_reached_max_vertices = false;
      m_solutions.clear();
    };

    RK_RTTI_MAKE_CONCRETE_1BASE(self,0xC2460009,1,"rrtstar_path_planner",base_type)
};




template <typename FreeSpaceType, typename NNFinderSynchro, typename SBPPReporter = no_sbmp_report>
struct rrtstar_planner_visitor {
  shared_ptr< FreeSpaceType > m_space;
  rrtstar_path_planner<FreeSpaceType,SBPPReporter>* m_planner;
  NNFinderSynchro m_nn_synchro;
  
  rrtstar_planner_visitor(const shared_ptr< FreeSpaceType >& aSpace, 
                          rrtstar_path_planner<FreeSpaceType,SBPPReporter>* aPlanner,
                          NNFinderSynchro aNNSynchro) : 
                          m_space(aSpace), m_planner(aPlanner), m_nn_synchro(aNNSynchro) { };
  
  typedef typename topology_traits<FreeSpaceType>::point_type PointType;
  
  template <typename Vertex, typename Graph>
  void vertex_added(Vertex u, Graph& g) const {
    m_nn_synchro.added_vertex(u,g);
    //std::cout << "reached this point." << std::endl;
    std::cout << "\r" << std::setw(10) << num_vertices(g) << std::flush;
  };
  
  template <typename Vertex, typename Graph>
  void vertex_added(Vertex u, Graph& g, Graph&) const {
    m_nn_synchro.added_vertex(u,g);
  };
  
  template <typename EdgeType, typename Graph>
  void edge_added(EdgeType e, Graph& g) const {
    typedef typename boost::graph_traits<Graph>::vertex_descriptor VertexType;
    VertexType v = target(e,g);
    
    // Call progress reporter...
    m_planner->report_progress(g);
    
    // Check if a straight path to goal is possible...
    m_planner->check_goal_connection(g[v].position,v,g);
    
  };
  
  template <typename EdgeType, typename Graph>
  void edge_added(EdgeType e, Graph& g1, Graph& g2) const {
    
    // Call progress reporter...
    m_planner->report_progress(g1,g2);
  };
  
  bool is_position_free(PointType p) const {
    return m_space->is_free(p);
  };
  
  bool keep_going() const {
    return m_planner->keep_going();
  };
  
  template <typename Vertex, typename Graph>
  void joining_vertex_found(Vertex u1, Vertex u2, Graph& g1, Graph& g2) const {
    m_planner->joining_vertex_found(u1, u2, g1, g2);
  };
  
  template <typename Vertex, typename Graph>
  std::pair<PointType, bool> steer_towards_position(const PointType& p, Vertex u, Graph& g) const {
    PointType result_p = m_space->move_position_toward(g[u].position, 1.0, p);
    double best_case_dist = get(distance_metric, m_space->get_super_space())(g[u].position, p, m_space->get_super_space());
    double actual_dist = get(distance_metric, m_space->get_super_space())(g[u].position, result_p, m_space->get_super_space());
    if(actual_dist > 0.1 * best_case_dist)
      return std::make_pair(result_p, true);
    else
      return std::make_pair(result_p, false);
  };
  
  template <typename Vertex, typename Graph>
  bool can_be_connected(Vertex u, Vertex v, const Graph& g) {
    double dist = get(distance_metric, *m_space)(g[u].position, g[v].position, *m_space);
    return (dist < std::numeric_limits<double>::infinity());
  };
  
};






template <typename FreeSpaceType, 
          typename SBPPReporter>
shared_ptr< path_base< typename rrtstar_path_planner<FreeSpaceType,SBPPReporter>::super_space_type > > 
  rrtstar_path_planner<FreeSpaceType,SBPPReporter>::solve_path() {
  using ReaK::to_vect;
  
  typedef typename subspace_traits<FreeSpaceType>::super_space_type SuperSpace;
  typedef typename topology_traits<SuperSpace>::point_type PointType;
  typedef boost::data_member_property_map<PointType, rrtstar_vertex_data<FreeSpaceType> > PositionMap;
  PositionMap pos_map = PositionMap(&rrtstar_vertex_data<FreeSpaceType>::position);
  typedef boost::data_member_property_map<double, rrtstar_vertex_data<FreeSpaceType> > CostMap;
  CostMap cost_map = CostMap(&rrtstar_vertex_data<FreeSpaceType>::distance_accum);
  typedef boost::data_member_property_map<std::size_t, rrtstar_vertex_data<FreeSpaceType> > PredMap;
  PredMap pred_map = PredMap(&rrtstar_vertex_data<FreeSpaceType>::predecessor);
  typedef boost::data_member_property_map<double, rrtstar_edge_data<FreeSpaceType> > WeightMap;
  WeightMap weight_map = WeightMap(&rrtstar_edge_data<FreeSpaceType>::weight);
  
  double space_dim = double((to_vect<double>(this->m_space->get_super_space().difference(this->m_goal_pos,this->m_start_pos))).size()); 
  double space_Lc = get(distance_metric,this->m_space->get_super_space())(this->m_start_pos, this->m_goal_pos, this->m_space->get_super_space());
  
  
  if(m_bidir_flag == UNIDIRECTIONAL_RRT) {
    
    if(m_graph_kind_flag == ADJ_LIST_MOTION_GRAPH) {
      
      typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::undirectedS,
                             rrtstar_vertex_data<FreeSpaceType>,
                             rrtstar_edge_data<FreeSpaceType>,
                             boost::vecS> MotionGraphType;
      typedef typename boost::graph_traits<MotionGraphType>::vertex_descriptor Vertex;
      typedef typename MotionGraphType::vertex_property_type VertexProp;
      typedef boost::composite_property_map< 
        PositionMap, boost::whole_bundle_property_map< MotionGraphType, boost::vertex_bundle_t > > GraphPositionMap;
      
      MotionGraphType motion_graph;
      GraphPositionMap g_pos_map = GraphPositionMap(pos_map, boost::whole_bundle_property_map< MotionGraphType, boost::vertex_bundle_t >(&motion_graph));
      
      rrtstar_vertex_data<FreeSpaceType> v_p;
      v_p.position = this->m_start_pos;
      v_p.distance_accum = 0.0;
      v_p.predecessor = MotionGraphType::null_vertex();
      add_vertex(v_p, motion_graph);
      
      if(m_knn_flag == LINEAR_SEARCH_KNN) {
        rrtstar_planner_visitor<FreeSpaceType, no_NNfinder_synchro, SBPPReporter> vis(this->m_space, this, no_NNfinder_synchro());
        
        ReaK::graph::generate_rrt_star(
          motion_graph, 
          this->m_space->get_super_space(),
          vis, 
          pos_map, 
          cost_map,
          pred_map,
          weight_map,
          get(random_sampler, this->m_space->get_super_space()), 
          get(distance_metric, this->m_space->get_super_space()),
          ReaK::graph::star_neighborhood< linear_neighbor_search<> >(
            linear_neighbor_search<>(), 
            space_dim, 3.0 * space_Lc), 
          this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_BF2_TREE_KNN) {
        
        typedef dvp_tree<Vertex, SuperSpace, GraphPositionMap, 2, 
                         random_vp_chooser, ReaK::graph::d_ary_bf_tree_storage<2> > SpacePartType;
        SpacePartType space_part(motion_graph, ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), g_pos_map);
        
        multi_dvp_tree_search<MotionGraphType, SpacePartType> nn_finder;
        nn_finder.graph_tree_map[&motion_graph] = &space_part;
        
        rrtstar_planner_visitor<FreeSpaceType, multi_dvp_tree_search<MotionGraphType, SpacePartType>, SBPPReporter> vis(this->m_space, this, nn_finder);
        
        ReaK::graph::generate_rrt_star(
          motion_graph, 
          this->m_space->get_super_space(),
          vis, 
          pos_map, 
          cost_map,
          pred_map,
          weight_map,
          get(random_sampler, this->m_space->get_super_space()), 
          get(distance_metric, this->m_space->get_super_space()),
          ReaK::graph::star_neighborhood< multi_dvp_tree_search<MotionGraphType, SpacePartType> >(
            nn_finder, 
            space_dim, 3.0 * space_Lc), 
          this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_BF4_TREE_KNN) {
        
        typedef dvp_tree<Vertex, SuperSpace, GraphPositionMap, 4, 
                         random_vp_chooser, ReaK::graph::d_ary_bf_tree_storage<4> > SpacePartType;
        SpacePartType space_part(motion_graph, ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), g_pos_map);
        
        multi_dvp_tree_search<MotionGraphType, SpacePartType> nn_finder;
        nn_finder.graph_tree_map[&motion_graph] = &space_part;
        
        rrtstar_planner_visitor<FreeSpaceType, multi_dvp_tree_search<MotionGraphType, SpacePartType>, SBPPReporter> vis(this->m_space, this, nn_finder);
        
        ReaK::graph::generate_rrt_star(
          motion_graph, 
          this->m_space->get_super_space(),
          vis, 
          pos_map, 
          cost_map,
          pred_map,
          weight_map,
          get(random_sampler, this->m_space->get_super_space()), 
          get(distance_metric, this->m_space->get_super_space()),
          ReaK::graph::star_neighborhood< multi_dvp_tree_search<MotionGraphType, SpacePartType> >(
            nn_finder, 
            space_dim, 3.0 * space_Lc), 
          this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_COB2_TREE_KNN) {
        
        typedef dvp_tree<Vertex, SuperSpace, GraphPositionMap, 2, 
                         random_vp_chooser, ReaK::graph::d_ary_cob_tree_storage<2> > SpacePartType;
        SpacePartType space_part(motion_graph, ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), g_pos_map);
        
        multi_dvp_tree_search<MotionGraphType, SpacePartType> nn_finder;
        nn_finder.graph_tree_map[&motion_graph] = &space_part;
        
        rrtstar_planner_visitor<FreeSpaceType, multi_dvp_tree_search<MotionGraphType, SpacePartType>, SBPPReporter> vis(this->m_space, this, nn_finder);
        
        ReaK::graph::generate_rrt_star(
          motion_graph, 
          this->m_space->get_super_space(),
          vis, 
          pos_map, 
          cost_map,
          pred_map,
          weight_map,
          get(random_sampler, this->m_space->get_super_space()), 
          get(distance_metric, this->m_space->get_super_space()),
          ReaK::graph::star_neighborhood< multi_dvp_tree_search<MotionGraphType, SpacePartType> >(
            nn_finder, 
            space_dim, 3.0 * space_Lc), 
          this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_COB4_TREE_KNN) {
        
        typedef dvp_tree<Vertex, SuperSpace, GraphPositionMap, 4, 
                         random_vp_chooser, ReaK::graph::d_ary_cob_tree_storage<4> > SpacePartType;
        SpacePartType space_part(motion_graph, ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), g_pos_map);
        
        multi_dvp_tree_search<MotionGraphType, SpacePartType> nn_finder;
        nn_finder.graph_tree_map[&motion_graph] = &space_part;
        
        rrtstar_planner_visitor<FreeSpaceType, multi_dvp_tree_search<MotionGraphType, SpacePartType>, SBPPReporter> vis(this->m_space, this, nn_finder);
        
        ReaK::graph::generate_rrt_star(
          motion_graph, 
          this->m_space->get_super_space(),
          vis, 
          pos_map, 
          cost_map,
          pred_map,
          weight_map,
          get(random_sampler, this->m_space->get_super_space()), 
          get(distance_metric, this->m_space->get_super_space()),
          ReaK::graph::star_neighborhood< multi_dvp_tree_search<MotionGraphType, SpacePartType> >(
            nn_finder, 
            space_dim, 3.0 * space_Lc), 
          this->m_max_vertex_count);
        
      };
      
    } else if(m_graph_kind_flag == DVP_ADJ_LIST_MOTION_GRAPH) {
      
      if(m_knn_flag == DVP_ALT_BF2_KNN) {
        
        typedef dvp_adjacency_list<
          rrtstar_vertex_data<FreeSpaceType>,
          rrtstar_edge_data<FreeSpaceType>,
          SuperSpace,
          PositionMap,
          2, random_vp_chooser, ReaK::graph::d_ary_bf_tree_storage<2>,
          boost::vecS, boost::undirectedS, boost::listS > ALTGraph;
        
        ALTGraph space_part(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), pos_map);
        
        typedef typename ALTGraph::adj_list_type MotionGraph;
        
        MotionGraph motion_graph = space_part.get_adjacency_list();
        
        rrtstar_vertex_data<FreeSpaceType> v_p;
        v_p.position = this->m_start_pos;
        v_p.distance_accum = 0.0;
        v_p.predecessor = MotionGraph::null_vertex();
        add_vertex(v_p, motion_graph);
        
        multi_dvp_tree_search<MotionGraph, ALTGraph> nn_finder;
        nn_finder.graph_tree_map[&motion_graph] = &space_part;
        
        rrtstar_planner_visitor<FreeSpaceType, no_NNfinder_synchro, SBPPReporter> vis(this->m_space, this, no_NNfinder_synchro());
        
        ReaK::graph::generate_rrt_star(
          motion_graph, 
          this->m_space->get_super_space(),
          vis, 
          pos_map, 
          cost_map,
          pred_map,
          weight_map,
          get(random_sampler, this->m_space->get_super_space()), 
          get(distance_metric, this->m_space->get_super_space()),
          ReaK::graph::star_neighborhood< multi_dvp_tree_search<MotionGraph, ALTGraph> >(
            nn_finder, 
            space_dim, 3.0 * space_Lc), 
          this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_ALT_BF4_KNN) {
        
        typedef dvp_adjacency_list<
          rrtstar_vertex_data<FreeSpaceType>,
          rrtstar_edge_data<FreeSpaceType>,
          SuperSpace,
          PositionMap,
          4, random_vp_chooser, ReaK::graph::d_ary_bf_tree_storage<4>,
          boost::vecS, boost::undirectedS, boost::listS > ALTGraph;
        
        ALTGraph space_part(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), pos_map);
        
        typedef typename ALTGraph::adj_list_type MotionGraph;
        
        MotionGraph motion_graph = space_part.get_adjacency_list();
        
        rrtstar_vertex_data<FreeSpaceType> v_p;
        v_p.position = this->m_start_pos;
        v_p.distance_accum = 0.0;
        v_p.predecessor = MotionGraph::null_vertex();
        add_vertex(v_p, motion_graph);
        
        multi_dvp_tree_search<MotionGraph, ALTGraph> nn_finder;
        nn_finder.graph_tree_map[&motion_graph] = &space_part;
        
        rrtstar_planner_visitor<FreeSpaceType, no_NNfinder_synchro, SBPPReporter> vis(this->m_space, this, no_NNfinder_synchro());
        
        ReaK::graph::generate_rrt_star(
          motion_graph, 
          this->m_space->get_super_space(),
          vis, 
          pos_map, 
          cost_map,
          pred_map,
          weight_map,
          get(random_sampler, this->m_space->get_super_space()), 
          get(distance_metric, this->m_space->get_super_space()),
          ReaK::graph::star_neighborhood< multi_dvp_tree_search<MotionGraph, ALTGraph> >(
            nn_finder, 
            space_dim, 3.0 * space_Lc), 
          this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_ALT_COB2_KNN) {
        
        typedef dvp_adjacency_list<
          rrtstar_vertex_data<FreeSpaceType>,
          rrtstar_edge_data<FreeSpaceType>,
          SuperSpace,
          PositionMap,
          2, random_vp_chooser, ReaK::graph::d_ary_cob_tree_storage<2>,
          boost::vecS, boost::undirectedS, boost::listS > ALTGraph;
        
        ALTGraph space_part(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), pos_map);
        
        typedef typename ALTGraph::adj_list_type MotionGraph;
        
        MotionGraph motion_graph = space_part.get_adjacency_list();
        
        rrtstar_vertex_data<FreeSpaceType> v_p;
        v_p.position = this->m_start_pos;
        v_p.distance_accum = 0.0;
        v_p.predecessor = MotionGraph::null_vertex();
        add_vertex(v_p, motion_graph);
        
        multi_dvp_tree_search<MotionGraph, ALTGraph> nn_finder;
        nn_finder.graph_tree_map[&motion_graph] = &space_part;
        
        rrtstar_planner_visitor<FreeSpaceType, no_NNfinder_synchro, SBPPReporter> vis(this->m_space, this, no_NNfinder_synchro());
        
        ReaK::graph::generate_rrt_star(
          motion_graph, 
          this->m_space->get_super_space(),
          vis, 
          pos_map, 
          cost_map,
          pred_map,
          weight_map,
          get(random_sampler, this->m_space->get_super_space()), 
          get(distance_metric, this->m_space->get_super_space()),
          ReaK::graph::star_neighborhood< multi_dvp_tree_search<MotionGraph, ALTGraph> >(
            nn_finder, 
            space_dim, 3.0 * space_Lc), 
          this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_ALT_COB4_KNN) {
        
        typedef dvp_adjacency_list<
          rrtstar_vertex_data<FreeSpaceType>,
          rrtstar_edge_data<FreeSpaceType>,
          SuperSpace,
          PositionMap,
          4, random_vp_chooser, ReaK::graph::d_ary_cob_tree_storage<4>,
          boost::vecS, boost::undirectedS, boost::listS > ALTGraph;
        
        ALTGraph space_part(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), pos_map);
        
        typedef typename ALTGraph::adj_list_type MotionGraph;
        
        MotionGraph motion_graph = space_part.get_adjacency_list();
        
        rrtstar_vertex_data<FreeSpaceType> v_p;
        v_p.position = this->m_start_pos;
        v_p.distance_accum = 0.0;
        v_p.predecessor = MotionGraph::null_vertex();
        add_vertex(v_p, motion_graph);
        
        multi_dvp_tree_search<MotionGraph, ALTGraph> nn_finder;
        nn_finder.graph_tree_map[&motion_graph] = &space_part;
        
        rrtstar_planner_visitor<FreeSpaceType, no_NNfinder_synchro, SBPPReporter> vis(this->m_space, this, no_NNfinder_synchro());
        
        ReaK::graph::generate_rrt_star(
          motion_graph, 
          this->m_space->get_super_space(),
          vis, 
          pos_map, 
          cost_map,
          pred_map,
          weight_map,
          get(random_sampler, this->m_space->get_super_space()), 
          get(distance_metric, this->m_space->get_super_space()),
          ReaK::graph::star_neighborhood< multi_dvp_tree_search<MotionGraph, ALTGraph> >(
            nn_finder, 
            space_dim, 3.0 * space_Lc), 
          this->m_max_vertex_count);
        
      };
      
    };
    
  } else {
#if 0    
    if(m_graph_kind_flag == ADJ_LIST_MOTION_GRAPH) {
      
      typedef boost::adjacency_list< boost::vecS, boost::listS, boost::bidirectionalS,
                             rrtstar_vertex_data<FreeSpaceType>,
                             rrtstar_edge_data<FreeSpaceType>,
                             boost::vecS> MotionGraphType;
      typedef typename boost::graph_traits<MotionGraphType>::vertex_descriptor Vertex;
      
      typedef boost::composite_property_map< 
        PositionMap, boost::whole_bundle_property_map< MotionGraphType, boost::vertex_bundle_t > > GraphPositionMap;
      
      MotionGraphType motion_graph1;
      GraphPositionMap g1_pos_map = GraphPositionMap(pos_map, boost::whole_bundle_property_map< MotionGraphType, boost::vertex_bundle_t >(&motion_graph1));
      Vertex v1 = add_vertex(motion_graph1);
      motion_graph1[v1].position = this->m_start_pos;
      motion_graph1[v1].distance_accum = 0.0;
      
      MotionGraphType motion_graph2;
      GraphPositionMap g2_pos_map = GraphPositionMap(pos_map, boost::whole_bundle_property_map< MotionGraphType, boost::vertex_bundle_t >(&motion_graph2));
      Vertex v2 = add_vertex(motion_graph2);
      motion_graph2[v2].position = this->m_goal_pos;
      motion_graph2[v2].distance_accum = 0.0;
      
      if(m_knn_flag == LINEAR_SEARCH_KNN) {
        rrtstar_planner_visitor<FreeSpaceType, no_NNfinder_synchro, SBPPReporter> vis(this->m_space, this, no_NNfinder_synchro());
        
        ReaK::graph::generate_bidirectional_rrt(
          motion_graph1, motion_graph2, this->m_space->get_super_space(),
          vis, pos_map, get(random_sampler, this->m_space->get_super_space()), 
          linear_neighbor_search<>(), this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_BF2_TREE_KNN) {
        
        typedef dvp_tree<Vertex, SuperSpace, GraphPositionMap, 2, 
                         random_vp_chooser, ReaK::graph::d_ary_bf_tree_storage<2> > SpacePartType;
        SpacePartType space_part1(motion_graph1, 
                                  ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), 
                                  g1_pos_map);
        SpacePartType space_part2(motion_graph2, 
                                  ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), 
                                  g2_pos_map);
        
        multi_dvp_tree_search<MotionGraphType, SpacePartType> nn_finder;
        nn_finder.graph_tree_map[&motion_graph1] = &space_part1;
        nn_finder.graph_tree_map[&motion_graph2] = &space_part2;
        
        rrtstar_planner_visitor<FreeSpaceType, multi_dvp_tree_search<MotionGraphType, SpacePartType>, SBPPReporter> vis(this->m_space, this, nn_finder);
        
        ReaK::graph::generate_bidirectional_rrt(
          motion_graph1, motion_graph2, this->m_space->get_super_space(),
          vis, pos_map, get(random_sampler, this->m_space->get_super_space()), 
          nn_finder, this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_BF4_TREE_KNN) {
        
        typedef dvp_tree<Vertex, SuperSpace, GraphPositionMap, 4, 
                         random_vp_chooser, ReaK::graph::d_ary_bf_tree_storage<4> > SpacePartType;
        SpacePartType space_part1(motion_graph1, 
                                  ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), 
                                  g1_pos_map);
        SpacePartType space_part2(motion_graph2, 
                                  ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), 
                                  g2_pos_map);
        
        multi_dvp_tree_search<MotionGraphType, SpacePartType> nn_finder;
        nn_finder.graph_tree_map[&motion_graph1] = &space_part1;
        nn_finder.graph_tree_map[&motion_graph2] = &space_part2;
        
        rrtstar_planner_visitor<FreeSpaceType, multi_dvp_tree_search<MotionGraphType, SpacePartType>, SBPPReporter> vis(this->m_space, this, nn_finder);
        
        ReaK::graph::generate_bidirectional_rrt(
          motion_graph1, motion_graph2, this->m_space->get_super_space(),
          vis, pos_map, get(random_sampler, this->m_space->get_super_space()), 
          nn_finder, this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_COB2_TREE_KNN) {
        
        typedef dvp_tree<Vertex, SuperSpace, GraphPositionMap, 2, 
                         random_vp_chooser, ReaK::graph::d_ary_cob_tree_storage<2> > SpacePartType;
        SpacePartType space_part1(motion_graph1, 
                                  ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), 
                                  g1_pos_map);
        SpacePartType space_part2(motion_graph2, 
                                  ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), 
                                  g2_pos_map);
        
        multi_dvp_tree_search<MotionGraphType, SpacePartType> nn_finder;
        nn_finder.graph_tree_map[&motion_graph1] = &space_part1;
        nn_finder.graph_tree_map[&motion_graph2] = &space_part2;
        
        rrtstar_planner_visitor<FreeSpaceType, multi_dvp_tree_search<MotionGraphType, SpacePartType>, SBPPReporter> vis(this->m_space, this, nn_finder);
        
        ReaK::graph::generate_bidirectional_rrt(
          motion_graph1, motion_graph2, this->m_space->get_super_space(),
          vis, pos_map, get(random_sampler, this->m_space->get_super_space()), 
          nn_finder, this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_COB4_TREE_KNN) {
        
        typedef dvp_tree<Vertex, SuperSpace, GraphPositionMap, 4, 
                         random_vp_chooser, ReaK::graph::d_ary_cob_tree_storage<4> > SpacePartType;
        SpacePartType space_part1(motion_graph1, 
                                  ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), 
                                  g1_pos_map);
        SpacePartType space_part2(motion_graph2, 
                                  ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), 
                                  g2_pos_map);
        
        multi_dvp_tree_search<MotionGraphType, SpacePartType> nn_finder;
        nn_finder.graph_tree_map[&motion_graph1] = &space_part1;
        nn_finder.graph_tree_map[&motion_graph2] = &space_part2;
        
        rrtstar_planner_visitor<FreeSpaceType, multi_dvp_tree_search<MotionGraphType, SpacePartType>, SBPPReporter> vis(this->m_space, this, nn_finder);
        
        ReaK::graph::generate_bidirectional_rrt(
          motion_graph1, motion_graph2, this->m_space->get_super_space(),
          vis, pos_map, get(random_sampler, this->m_space->get_super_space()), 
          nn_finder, this->m_max_vertex_count);
        
      };
      
    } else if(m_graph_kind_flag == DVP_ADJ_LIST_MOTION_GRAPH) {
      
      if(m_knn_flag == DVP_ALT_BF2_KNN) {
        
        typedef dvp_adjacency_list<
          rrtstar_vertex_data<FreeSpaceType>,
          rrtstar_edge_data<FreeSpaceType>,
          SuperSpace,
          PositionMap,
          2, random_vp_chooser, ReaK::graph::d_ary_bf_tree_storage<2>,
          boost::vecS, boost::bidirectionalS, boost::listS > ALTGraph;
        
        ALTGraph space_part1(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), 
                             pos_map);
        
        ALTGraph space_part2(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), 
                             pos_map);
        
        typedef typename ALTGraph::adj_list_type MotionGraph;
        
        MotionGraph motion_graph1 = space_part1.get_adjacency_list();
        rrtstar_vertex_data<FreeSpaceType> v1_p;
        v1_p.position = this->m_start_pos;
        v1_p.distance_accum = 0.0;
        create_root(v1_p, motion_graph1);
        
        MotionGraph motion_graph2 = space_part2.get_adjacency_list();
        rrtstar_vertex_data<FreeSpaceType> v2_p;
        v2_p.position = this->m_goal_pos;
        v2_p.distance_accum = 0.0;
        create_root(v2_p, motion_graph2);
        
        multi_dvp_tree_search<MotionGraph, ALTGraph> nn_finder;
        nn_finder.graph_tree_map[&motion_graph1] = &space_part1;
        nn_finder.graph_tree_map[&motion_graph2] = &space_part2;
        
        rrtstar_planner_visitor<FreeSpaceType, no_NNfinder_synchro, SBPPReporter> vis(this->m_space, this, no_NNfinder_synchro());
        
        ReaK::graph::generate_bidirectional_rrt(
          motion_graph1, motion_graph2, this->m_space->get_super_space(),
          vis, pos_map, get(random_sampler, this->m_space->get_super_space()), 
          nn_finder, this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_ALT_BF4_KNN) {
        
        typedef dvp_adjacency_list<
          rrtstar_vertex_data<FreeSpaceType>,
          rrtstar_edge_data<FreeSpaceType>,
          SuperSpace,
          PositionMap,
          4, random_vp_chooser, ReaK::graph::d_ary_bf_tree_storage<4>,
          boost::vecS, boost::bidirectionalS, boost::listS > ALTGraph;
        
        ALTGraph space_part1(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), pos_map);
        
        ALTGraph space_part2(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), pos_map);
        
        typedef typename ALTGraph::adj_list_type MotionGraph;
        
        MotionGraph motion_graph1 = space_part1.get_adjacency_list();
        rrtstar_vertex_data<FreeSpaceType> v1_p;
        v1_p.position = this->m_start_pos;
        v1_p.distance_accum = 0.0;
        create_root(v1_p, motion_graph1);
        
        MotionGraph motion_graph2 = space_part2.get_adjacency_list();
        rrtstar_vertex_data<FreeSpaceType> v2_p;
        v2_p.position = this->m_goal_pos;
        v2_p.distance_accum = 0.0;
        create_root(v2_p, motion_graph2);
        
        multi_dvp_tree_search<MotionGraph, ALTGraph> nn_finder;
        nn_finder.graph_tree_map[&motion_graph1] = &space_part1;
        nn_finder.graph_tree_map[&motion_graph2] = &space_part2;
        
        rrtstar_planner_visitor<FreeSpaceType, no_NNfinder_synchro, SBPPReporter> vis(this->m_space, this, no_NNfinder_synchro());
        
        ReaK::graph::generate_bidirectional_rrt(
          motion_graph1, motion_graph2, this->m_space->get_super_space(),
          vis, pos_map, get(random_sampler, this->m_space->get_super_space()), 
          nn_finder, this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_ALT_COB2_KNN) {
        
        typedef dvp_adjacency_list<
          rrtstar_vertex_data<FreeSpaceType>,
          rrtstar_edge_data<FreeSpaceType>,
          SuperSpace,
          PositionMap,
          2, random_vp_chooser, ReaK::graph::d_ary_cob_tree_storage<2>,
          boost::vecS, boost::bidirectionalS, boost::listS > ALTGraph;
        
        ALTGraph space_part1(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), pos_map);
        
        ALTGraph space_part2(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), pos_map);
        
        typedef typename ALTGraph::adj_list_type MotionGraph;
        
        MotionGraph motion_graph1 = space_part1.get_adjacency_list();
        rrtstar_vertex_data<FreeSpaceType> v1_p;
        v1_p.position = this->m_start_pos;
        v1_p.distance_accum = 0.0;
        create_root(v1_p, motion_graph1);
        
        MotionGraph motion_graph2 = space_part2.get_adjacency_list();
        rrtstar_vertex_data<FreeSpaceType> v2_p;
        v2_p.position = this->m_goal_pos;
        v2_p.distance_accum = 0.0;
        create_root(v2_p, motion_graph2);
        
        multi_dvp_tree_search<MotionGraph, ALTGraph> nn_finder;
        nn_finder.graph_tree_map[&motion_graph1] = &space_part1;
        nn_finder.graph_tree_map[&motion_graph2] = &space_part2;
        
        rrtstar_planner_visitor<FreeSpaceType, no_NNfinder_synchro, SBPPReporter> vis(this->m_space, this, no_NNfinder_synchro());
        
        ReaK::graph::generate_bidirectional_rrt(
          motion_graph1, motion_graph2, this->m_space->get_super_space(),
          vis, pos_map, get(random_sampler, this->m_space->get_super_space()), 
          nn_finder, this->m_max_vertex_count);
        
      } else if(m_knn_flag == DVP_ALT_COB4_KNN) {
        
        typedef dvp_adjacency_list<
          rrtstar_vertex_data<FreeSpaceType>,
          rrtstar_edge_data<FreeSpaceType>,
          SuperSpace,
          PositionMap,
          4, random_vp_chooser, ReaK::graph::d_ary_cob_tree_storage<4>,
          boost::vecS, boost::bidirectionalS, boost::listS > ALTGraph;
        
        ALTGraph space_part1(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), pos_map);
        
        ALTGraph space_part2(ReaK::shared_ptr<const SuperSpace>(&(this->m_space->get_super_space()),null_deleter()), pos_map);
        
        typedef typename ALTGraph::adj_list_type MotionGraph;
        
        MotionGraph motion_graph1 = space_part1.get_adjacency_list();
        rrtstar_vertex_data<FreeSpaceType> v1_p;
        v1_p.position = this->m_start_pos;
        v1_p.distance_accum = 0.0;
        create_root(v1_p, motion_graph1);
        
        MotionGraph motion_graph2 = space_part2.get_adjacency_list();
        rrtstar_vertex_data<FreeSpaceType> v2_p;
        v2_p.position = this->m_goal_pos;
        v2_p.distance_accum = 0.0;
        create_root(v2_p, motion_graph2);
        
        multi_dvp_tree_search<MotionGraph, ALTGraph> nn_finder;
        nn_finder.graph_tree_map[&motion_graph1] = &space_part1;
        nn_finder.graph_tree_map[&motion_graph2] = &space_part2;
        
        rrtstar_planner_visitor<FreeSpaceType, no_NNfinder_synchro, SBPPReporter> vis(this->m_space, this, no_NNfinder_synchro());
        
        ReaK::graph::generate_bidirectional_rrt(
          motion_graph1, motion_graph2, this->m_space->get_super_space(),
          vis, pos_map, get(random_sampler, this->m_space->get_super_space()), 
          nn_finder, this->m_max_vertex_count);
        
      };
      
    };
#endif
  };
  
  if(m_solutions.size())
    return m_solutions.begin()->second;
  else
    return shared_ptr< path_base< SuperSpace > >();
};


};

};

#endif
