/**
 * \file metric_space_concept.hpp
 * 
 * This library defines the traits and concepts that pertain to what can be considered 
 * a metric-space, as used in ReaK::pp. Metric-spaces are based on the Topology concept 
 * from the Boost.Graph library, but with additional requirements which are needed 
 * in algorithms tailored for a metric-space (see metric_space_search.hpp). Basically,
 * the concept of a metric-space in ReaK::pp corresponds to the mathematical concept of 
 * a metric-space (see wikipedia or any decent math book).
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date March 2011
 */

/*
 *    Copyright 2011 Sven Mikael Persson
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

#ifndef REAK_METRIC_SPACE_CONCEPT_HPP
#define REAK_METRIC_SPACE_CONCEPT_HPP


#include <boost/config.hpp>
#include <cmath>
#include <boost/concept_check.hpp>

/** Main namespace for ReaK */
namespace ReaK {

/** Main namespace for ReaK.Path-Planning */
namespace pp {
  
  
/**
 * This traits class defines the types and constants associated to a metric-space.
 * \tparam Topology The topology type for which the metric-space traits are sought.
 */
template <typename Topology>
struct metric_topology_traits {
  /** The type that describes a point in the space. */
  typedef typename Topology::point_type point_type;
  /** The type that describes a difference between points in the space. */
  typedef typename Topology::point_difference_type point_difference_type;
  
  /** The dimensions of the space (0 if unknown at compile-time). */
  BOOST_STATIC_CONSTANT(std::size_t, dimensions = point_type::dimensions);
  
};

/**
 * This concept defines the requirements to fulfill in order to model a distance-metric 
 * as used in ReaK::pp. A distance-metric is essentially a callable type that can compute 
 * both the distance between two points and the corresponding norm of a difference between 
 * two points.
 * 
 * Required concepts:
 * 
 * Topology should model the Topology concept of the BGL.
 * 
 * Valid expressions:
 * 
 * dist = d(p1, p2, s);  The distance (dist) can be obtained by calling the distance metric (d) on two points (p1,p2) and providing a const-ref to the topology (or space) (s).
 * 
 * dist = d(pd, s);  The distance (dist) can be obtained by calling the distance metric (d) on a point-difference (pd) and providing a const-ref to the topology (or space) (s).
 * 
 * \tparam DistanceMetric The distance metric type to be checked for this concept.
 * \tparam Topology The topology to which the distance metric should apply.
 */
template <typename DistanceMetric, typename Topology>
struct DistanceMetricConcept {
  DistanceMetric d;
  Topology s;
  Topology::point_type p1, p2;
  Topology::point_difference_type pd;
  double dist;
  void constraints() {
    dist = d(p1, p2, s);
    dist = d(pd, s);
  };
  
};

/**
 * This concept defines the requirements to fulfill in order to model a metric-space 
 * as used in ReaK::pp. A metric-space is a special kind of topology which has a 
 * distance metric (in theory, satisfying triangular inequality).
 * 
 * Valid expressions:
 * 
 * d  = space.distance(p1, p2);  The distance between two points (p1,p2) can be obtained as a double (d).
 * 
 * d  = space.norm(pd);  The norm of the difference (pd) between two points can be obtained as a double (d).
 * 
 * p1 = space.random_point();  A random-point in the metric-space can be obtained.
 * 
 * pd = space.difference(p1,p2);  The difference (pd) between two points (p1,p2) can be obtained.
 * 
 * p1 = space.move_position_toward(p1,d,p2);  A point can be obtained by moving a fraction (d) away from one point (p1) to another (p2).
 * 
 * p1 = space.origin();  The origin of the space can be obtained.
 * 
 * p1 = space.adjust(p1,d * pd);  A point-difference can be scaled (d * pd) and added to a point (p1) to obtain an adjusted point.
 * 
 * pd = -pd;  A point-difference can be negated (reversed).
 * 
 * \tparam Topology The topology type to be checked for this concept.
 */
template <typename Topology>
struct MetricSpaceConcept {
  typename metric_topology_traits<Topology>::point_type p1, p2;
  typename metric_topology_traits<Topology>::point_difference_type pd;
  Topology space;
  double d;
  void constraints() {
    d  = space.distance(p1, p2);
    d  = space.norm(pd);
    p1 = space.random_point();
    pd = space.difference(p1,p2);
    p1 = space.move_position_toward(p1,d,p2);
    p1 = space.origin();
    p1 = space.adjust(p1,d * pd);
    pd = -pd;
  };
  
};

/**
 * This class is the default distance metric functor which models the DistanceMetricConcept.
 * This class will simply rely on the distance and norm functions included in the 
 * given topology (assuming it models the MetricSpaceConcept).
 */
struct default_distance_metric {
  /** 
   * This function returns the distance between two points on a topology.
   * \tparam Point The point-type.
   * \tparam Topology The topology.
   * \param a The first point.
   * \param b The second point.
   * \param s The topology or space on which the points lie.
   * \return The distance between two points on a topology.
   */
  template <typename Point, typename Topology>
  double operator()(const Point& a, const Point& b, const Topology& s) const {
    return s.distance(a, b);
  };
  /** 
   * This function returns the norm of a difference between two points on a topology.
   * \tparam PointDiff The point-difference-type.
   * \tparam Topology The topology.
   * \param a The point-difference.
   * \param s The topology or space on which the points lie.
   * \return The norm of the difference between two points on a topology.
   */
  template <typename PointDiff, typename Topology>
  double operator()(const PointDiff& a, const Topology& s) const {
    return s.norm(a);
  };
};


};

};


#endif


