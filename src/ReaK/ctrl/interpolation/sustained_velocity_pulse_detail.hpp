/**
 * \file sustained_velocity_pulse_detail.hpp
 * 
 * This library contains the implementation details of the rate-limited sustained velocity 
 * pulse (SVP) interpolation.
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date November 2011
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

#ifndef REAK_SUSTAINED_VELOCITY_PULSE_DETAIL_HPP
#define REAK_SUSTAINED_VELOCITY_PULSE_DETAIL_HPP

#include "lin_alg/arithmetic_tuple.hpp"

#include "lin_alg/mat_num_exceptions.hpp"
#include "lin_alg/vect_alg.hpp"
#include "path_planning/bounded_space_concept.hpp"
#include "path_planning/tangent_bundle_concept.hpp"

#include "root_finders/bisection_method.hpp"

#include <boost/bind.hpp>

#include <cmath>

namespace ReaK {

namespace pp {
  
  
namespace detail {
  
  template <typename Idx, typename PointType, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline 
  typename boost::enable_if< 
    boost::mpl::less< 
      Idx, 
      boost::mpl::size_t<2> 
    >,
  void >::type svp_interpolate_HOT_impl(PointType& result, const PointDiff1& delta_second_order,
                                        const DiffSpace& space, const TimeSpace& t_space,
				 	double t_factor) {
    /* nothing to do. */
  };
  
  template <typename Idx, typename PointType, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline 
  typename boost::enable_if< 
    boost::mpl::equal_to< 
      Idx, 
      boost::mpl::size_t<2> 
    >,
  void >::type svp_interpolate_HOT_impl(PointType& result, const PointDiff1& delta_second_order,
                                        const DiffSpace& space, const TimeSpace& t_space,
					double t_factor) {
    get<2>(result) = lift_to_space<2>(delta_second_order, t_factor, space, t_space);
  };
  
  
  
  
  
  template <typename Idx, typename PointType, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline
  typename boost::enable_if<
    boost::mpl::less<
      Idx,
      boost::mpl::size_t<2>
    >,
  void >::type svp_constant_accel_motion_HOT_impl(PointType&, const PointDiff1&, 
                                                  const DiffSpace&, const TimeSpace&) {
    /* Nothing to do. */ 
  };
  
  template <typename Idx, typename PointType, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline
  typename boost::enable_if<
    boost::mpl::equal_to<
      Idx,
      boost::mpl::size_t<2>
    >,
  void >::type svp_constant_accel_motion_HOT_impl(PointType& result, const PointDiff1& descended_accel, 
                                                  const DiffSpace& space, const TimeSpace& t_space) {
    get<2>(result) = lift_to_space<2>(descended_accel,1.0,space,t_space);
  };
  
  template <typename Idx, typename PointType, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline
  typename boost::enable_if<
    boost::mpl::greater<
      Idx,
      boost::mpl::size_t<2>
    >,
  void >::type svp_constant_accel_motion_HOT_impl(PointType& result, const PointDiff1& descended_accel, 
                                                  const DiffSpace& space, const TimeSpace& t_space) {
    svp_constant_accel_motion_HOT_impl< typename boost::mpl::prior<Idx>::type >(result, descended_accel, space, t_space);
    
    get< Idx::type::value >(result) = get_space< Idx::type::value >(space,t_space).origin();
  };
  
  template <typename Idx, typename PointType, typename PointDiff1, typename DiffSpace, typename TimeSpace>
  inline
  void svp_constant_accel_motion_impl(PointType& result, const PointDiff1& descended_accel,
                                      const DiffSpace& space, const TimeSpace& t_space, double dt) {
    
    get<0>(result) = 
      get_space<0>(space,t_space).adjust( get<0>(result),
        descend_to_space<0>( get_space<1>(space,t_space).adjust( get<1>(result), 
          0.5 * dt * descended_accel), dt, space, t_space) );
      
    get<1>(result) = 
      get_space<1>(space,t_space).adjust( get<1>(result), 
        dt * descended_accel );

    if(Idx::type::value > 1) 
      svp_constant_accel_motion_HOT_impl<Idx>(result,descended_accel,space,t_space);
    
    return;
  };
  
  
  
  template <typename Idx, typename PointType, typename PointDiff0, typename DiffSpace, typename TimeSpace>
  inline
  typename boost::enable_if<
    boost::mpl::less<
      Idx,
      boost::mpl::size_t<1>
    >,
  void >::type svp_constant_vel_motion_HOT_impl(PointType&, const PointDiff0&, 
                                                const DiffSpace&, const TimeSpace&) {
    /* Nothing to do. */ 
  };
  
  template <typename Idx, typename PointType, typename PointDiff0, typename DiffSpace, typename TimeSpace>
  inline
  typename boost::enable_if<
    boost::mpl::equal_to<
      Idx,
      boost::mpl::size_t<1>
    >,
  void >::type svp_constant_vel_motion_HOT_impl(PointType& result, const PointDiff0& descended_vel, 
                                                const DiffSpace& space, const TimeSpace& t_space) {
    get<1>(result) = lift_to_space<1>(descended_vel,1.0,space,t_space);
  };
  
  template <typename Idx, typename PointType, typename PointDiff0, typename DiffSpace, typename TimeSpace>
  inline
  typename boost::enable_if<
    boost::mpl::greater<
      Idx,
      boost::mpl::size_t<1>
    >,
  void >::type svp_constant_vel_motion_HOT_impl(PointType& result, const PointDiff0& descended_vel, 
                                                const DiffSpace& space, const TimeSpace& t_space) {
    svp_constant_vel_motion_HOT_impl< typename boost::mpl::prior<Idx>::type >(result, descended_vel, space, t_space);
    
    get< Idx::type::value >(result) = get_space< Idx::type::value >(space,t_space).origin();
  };
  
  template <typename Idx, typename PointType, typename PointDiff0, typename DiffSpace, typename TimeSpace>
  inline
  void svp_constant_vel_motion_impl(PointType& result, const PointDiff0& descended_vel,
                                    const DiffSpace& space, const TimeSpace& t_space, double dt) {
      
    get<0>(result) = 
      get_space<0>(space,t_space).adjust( get<0>(result), dt * descended_vel);
      
    if(Idx::type::value > 0) 
      svp_constant_vel_motion_HOT_impl<Idx>(result,descended_vel,space,t_space);
    
    return;
  };
  
  
  
  
  
  
  
  
  template <typename Idx, typename PointType, typename PointDiff0, typename PointType1, typename DiffSpace, typename TimeSpace>
  inline 
  typename boost::enable_if< 
    boost::mpl::less< 
      Idx, 
      boost::mpl::size_t<3> 
    >,
  void >::type svp_interpolate_impl(PointType& result, const PointType& start_point, const PointType& end_point, 
				    const PointDiff0& delta_first_order, const PointType1& peak_velocity,
                                    const DiffSpace& space, const TimeSpace& t_space,
				    double dt, double dt_total) {
    
    double dt1 = get(distance_metric, get_space<1>(space,t_space))(get<1>(start_point), peak_velocity, get_space<1>(space,t_space));
    double dt2 = get(distance_metric, get_space<1>(space,t_space))(peak_velocity, get<1>(end_point), get_space<1>(space,t_space));
    dt_total -= dt1 + dt2;
    
    result = start_point;
    
    //Phase 1: constant acceleration to the peak-velocity:
    
    if(dt1 > std::numeric_limits<double>::epsilon()) {
      svp_constant_accel_motion_impl<Idx>(
	result,
	(1.0 / dt1) * get_space<1>(space,t_space).difference(peak_velocity,get<1>(start_point)),
        space, t_space,
	(dt > dt1 ? dt1 : dt)
      );
    };
    dt -= dt1;
    if(dt < 0.0)
      return;
    
    //Phase 2: constant velocity (or cruise phase):
    
    if(dt_total > std::numeric_limits<double>::epsilon()) {
      svp_constant_vel_motion_impl<Idx>(
	result,
	descend_to_space<0>(peak_velocity, 1.0, space, t_space),
        space, t_space,
	(dt > dt_total ? dt_total : dt)
      );
    };
    dt -= dt_total;
    if(dt < 0.0)
      return;
    
    //Phase 3: constant acceleration to end-velocity:
    
    if(dt2 > std::numeric_limits<double>::epsilon()) {
      svp_constant_accel_motion_impl<Idx>(
	result,
	(1.0 / dt2) * get_space<1>(space,t_space).difference(get<1>(end_point),peak_velocity),
	space, t_space,
	(dt > dt2 ? dt2 : dt)
      );
    };
    
  };
  
  template <typename Idx, typename PointType, typename PointDiff0, typename PointType1, typename DiffSpace, typename TimeSpace>
  inline 
  typename boost::enable_if< 
    boost::mpl::greater< 
      Idx, 
      boost::mpl::size_t<2> 
    >,
  void >::type svp_interpolate_impl(PointType& result, const PointType& start_point, const PointType& end_point, 
				    const PointDiff0& delta_first_order, const PointType1& peak_velocity, 
				    const DiffSpace& space, const TimeSpace& t_space,
				    double dt, double dt_total) {
    svp_interpolate_impl< typename boost::mpl::prior<Idx>::type >(result,start_point,end_point,delta_first_order,peak_velocity,space,t_space,dt,dt_total);
    
    get< Idx::type::value >(result) = get_space< Idx::type::value >(space,t_space).origin();
  };
  
  
  
  
  
  
  inline
  double svp_compute_slack_time(double beta, double dt, double beta_0, double norm_delta, const vect<double,5>& coefs) {
    using std::sqrt;
    using std::fabs;
    
    double c = (norm_delta + coefs[4] * (beta_0 * beta_0 - beta * beta) );
    
    return dt - sqrt(coefs[0] * coefs[0] - beta * coefs[4] * (2.0 * coefs[0] * coefs[1] - beta * coefs[4])) 
              - sqrt(coefs[2] * coefs[2] - beta * coefs[4] * (2.0 * coefs[2] * coefs[3] - beta * coefs[4]))
           - fabs(c) / beta;
  };
    
  inline
  double svp_compute_derivative_slack_time(double beta, double dt, double beta_0, double norm_delta, const vect<double,5>& coefs) {
    using std::sqrt;
    using std::fabs;
    
    double c = (norm_delta + coefs[4] * (beta_0 * beta_0 - beta * beta) );
    
    double term1 = sqrt(coefs[0] * coefs[0] - beta * coefs[4] * (2.0 * coefs[0] * coefs[1] - beta * coefs[4]));
    double term2 = sqrt(coefs[2] * coefs[2] - beta * coefs[4] * (2.0 * coefs[2] * coefs[3] - beta * coefs[4]));
    return fabs(c) / (beta * beta) - ( c > 0.0 ? -2.0 : 2.0) * coefs[4]
           - coefs[4] * ((coefs[4] * beta - coefs[0] * coefs[1]) / term1 + (coefs[4] * beta - coefs[2] * coefs[3]) / term2);
  };
  
  
  inline
  double svp_compute_travel_time(double beta, double beta_0, double norm_delta, const vect<double,5>& coefs) {
    using std::sqrt;
    using std::fabs;
    
    double c = (norm_delta + coefs[4] * (beta_0 * beta_0 - beta * beta) );
    
    return sqrt(coefs[0] * coefs[0] - beta * coefs[4] * (2.0 * coefs[0] * coefs[1] - beta * coefs[4])) 
           + sqrt(coefs[2] * coefs[2] - beta * coefs[4] * (2.0 * coefs[2] * coefs[3] - beta * coefs[4])) 
           + fabs(c) / beta;
  };
    
  inline
  double svp_compute_derivative_travel_time(double beta, double beta_0, double norm_delta, const vect<double,5>& coefs) {
    using std::sqrt;
    using std::fabs;
    
    double c = (norm_delta + coefs[4] * (beta_0 * beta_0 - beta * beta) );
    
    double term1 = sqrt(coefs[0] * coefs[0] - beta * coefs[4] * (2.0 * coefs[0] * coefs[1] - beta * coefs[4]));
    double term2 = sqrt(coefs[2] * coefs[2] - beta * coefs[4] * (2.0 * coefs[2] * coefs[3] - beta * coefs[4]));
    return coefs[4] * ((coefs[4] * beta - coefs[0] * coefs[1]) / term1 + (coefs[4] * beta - coefs[2] * coefs[3]) / term2) 
           - fabs(c) / (beta * beta) + ( c > 0.0 ? -2.0 : 2.0) * coefs[4];
  };
  
  struct svp_update_delta_first_order {
    template <typename PointType, typename PointDiff0, typename PointType1, typename DiffSpace, typename TimeSpace>
    void operator()(const PointType& start_point, const PointType& end_point,
		    PointDiff0& delta_first_order, PointType1& peak_velocity,
		    double& norm_delta, const double& beta,
		    const DiffSpace& space, const TimeSpace& t_space,
		    double num_tol = 1E-6) {
    
      PointDiff0 descended_peak_velocity = descend_to_space<0>(peak_velocity,1.0,space,t_space);
      delta_first_order = get_space<0>(space,t_space).difference(
        get_space<0>(space,t_space).adjust(get<0>(end_point), (-0.5 * get(distance_metric, get_space<1>(space,t_space))(peak_velocity, get<1>(end_point), get_space<1>(space,t_space))) * ( descended_peak_velocity
                                                                                                                                                 + descend_to_space<0>(get<1>(end_point),1.0,space,t_space) )),
        get_space<0>(space,t_space).adjust(get<0>(start_point), (0.5 * get(distance_metric, get_space<1>(space,t_space))(peak_velocity, get<1>(start_point), get_space<1>(space,t_space))) * (descended_peak_velocity
                                                                                                                                                    + descend_to_space<0>(get<1>(start_point),1.0,space,t_space) ))
      );
      norm_delta = get(distance_metric, get_space<0>(space,t_space))(delta_first_order, get_space<0>(space,t_space));
      if(norm_delta > num_tol)
        peak_velocity = lift_to_space<1>(delta_first_order * beta, norm_delta, space, t_space);
      if( get(distance_metric, get_space<0>(space,t_space))(descended_peak_velocity - delta_first_order, get_space<0>(space,t_space)) > get(distance_metric, get_space<0>(space,t_space))(descended_peak_velocity, get_space<0>(space,t_space)) )
        norm_delta = -norm_delta;
    };
  };
  
  double svp_solve_for_min_dt_beta(double beta, double norm_delta, const vect<double,5>& coefs, double num_tol) {
    if(svp_compute_derivative_travel_time(1.0,beta,norm_delta,coefs) > 0.0) {
      double upper = 1.0;
      double lower = 0.5;
      while((lower < 0.99) && (svp_compute_derivative_travel_time(lower,beta,norm_delta,coefs) > 0.0)) {
        lower += 0.5 * (upper - lower);
      };
      if(lower < 0.99) {
        bisection_method(lower, upper, boost::bind(svp_compute_derivative_travel_time,_1,boost::cref(beta),boost::cref(norm_delta),boost::cref(coefs)), num_tol);
      } else {
        upper = 1.0;
        lower = 0.5;
        while(svp_compute_derivative_travel_time(lower,beta,norm_delta,coefs) > 0.0) {
          upper = lower;
          lower *= 0.5;
        };
        bisection_method(lower, upper, boost::bind(svp_compute_derivative_travel_time,_1,boost::cref(beta),boost::cref(norm_delta),boost::cref(coefs)), num_tol);
      };
      //make sure that the second root does not cause a reversal of the travel direction:
      if( (norm_delta > 0.0) && ( norm_delta + coefs[4] * (beta * beta - upper * upper) < 0.0 )) {
        upper = std::sqrt(beta * beta + norm_delta / coefs[4]);
      };
      beta = upper;
    } else {
      beta = 1.0;
    };
    return beta;
  };
  
  bool svp_min_dt_predicate(double beta, double norm_delta, const vect<double,5>& coefs, double num_tol, double& result) {
    result = svp_compute_travel_time(beta,beta,norm_delta,coefs);
    RK_NOTICE(1,"   gives min-dt = " << result);
    return true;
  };
  
  double svp_solve_for_no_slack_beta(double beta, double norm_delta, const vect<double,5>& coefs, double num_tol, double delta_time) {
    double beta_peak1 = 1.0;
    double beta_peak2 = 5.0;
      
    if(svp_compute_slack_time(1.0,delta_time,beta,norm_delta,coefs) > 0.0) {
      //means I have a single root in the interval, so I can solve for it:
      double beta_low = 0.5;
      while(svp_compute_slack_time(beta_low,delta_time,beta,norm_delta,coefs) > 0.0) {
        beta_peak1 = beta_low;
        beta_low *= 0.5;
      };
      bisection_method(beta_low, beta_peak1, boost::bind(svp_compute_slack_time,_1,boost::cref(delta_time),boost::cref(beta),boost::cref(norm_delta),boost::cref(coefs)), num_tol);
    } else {
      //This means that I must have either a parabola-looking curve, or it never goes positive.
      // so, find the maximum in the interval, by finding the zero of the derivative:
      double beta_low = 0.5;
      while(svp_compute_derivative_slack_time(beta_low,delta_time,beta,norm_delta,coefs) < 0.0) {
        beta_peak1 = beta_low;
        beta_low *= 0.5;
      };
      bisection_method(beta_low,beta_peak1, boost::bind(svp_compute_derivative_slack_time,_1,boost::cref(delta_time),boost::cref(beta),boost::cref(norm_delta),boost::cref(coefs)), num_tol);
      if( svp_compute_slack_time(beta_peak1,delta_time,beta,norm_delta,coefs) > 0.0 ) {
        //this means the maximum slack-time is actually positive, meaning there must be a root on either side.
        beta_peak2 = beta_peak1;
        beta_low = 0.5 * beta_peak1;
        while(svp_compute_slack_time(beta_low,delta_time,beta,delta_time,coefs) > 0.0) {
          beta_peak1 = beta_low;
          beta_low *= 0.5;
        };
        bisection_method(beta_low, beta_peak1, boost::bind(svp_compute_slack_time,_1,boost::cref(delta_time),boost::cref(beta),boost::cref(norm_delta),boost::cref(coefs)), num_tol);
        beta_low = beta_peak2;
        beta_peak2 = 1.0;
        bisection_method(beta_low, beta_peak2, boost::bind(svp_compute_slack_time,_1,boost::cref(delta_time),boost::cref(beta),boost::cref(norm_delta),boost::cref(coefs)), num_tol);
          
        //make sure that the second root does not cause a reversal of the travel direction:
        if( norm_delta + coefs[4] * (beta * beta - beta_peak2 * beta_peak2) < 0.0 ) {
          beta_peak2 = 5.0;
        };
      };
    };
      
    RK_NOTICE(1,"   delta-time = " << delta_time);
    RK_NOTICE(1,"   beta-peaks = (" << beta_peak1 << " " << beta_peak2 << ")");
    
    if( std::fabs(beta - beta_peak1) < std::fabs(beta - beta_peak2) ) {
      beta = beta_peak1;
    } else {
      beta = beta_peak2;
    };
    if(beta <= num_tol)
      beta = num_tol;
    return beta;
  };
  
  bool svp_no_slack_predicate(double beta, double norm_delta, const vect<double,5>& coefs, double num_tol, double& slack, double delta_time) {
    slack = svp_compute_slack_time(beta,delta_time,beta,norm_delta,coefs);
    RK_NOTICE(1,"   gives slack-time = " << slack);
    return (std::fabs(slack) < 100.0 * num_tol * delta_time );
  };
  
  
  template <typename PointType, typename PointDiff0, typename PointType1, typename DiffSpace, typename TimeSpace,
            typename UpdateDeltaPFunctor, typename SolveForBetaFunctor, typename AddedPredicate>
  void svp_peak_velocity_iteration(const PointType& start_point, const PointType& end_point,
                                   PointDiff0& delta_first_order, PointType1& peak_velocity,
				   double& norm_delta, double& beta,
				   const DiffSpace& space, const TimeSpace& t_space,
				   UpdateDeltaPFunctor update_delta_p, SolveForBetaFunctor get_next_beta,
				   AddedPredicate pred,
				   double num_tol = 1E-6, unsigned int max_iter = 20) {
    
    typedef typename derived_N_order_space< DiffSpace, TimeSpace, 1>::type Space1;
    
    const Space1& space_1 = get_space<1>(space,t_space);
    
    vect<double,5> coefs( get(distance_metric, space_1)( get<1>(start_point), space_1.origin(), space_1 ),
	                  0.0,
		          get(distance_metric, space_1)( get<1>(end_point), space_1.origin(), space_1 ),
	                  0.0,
		          space_1.get_radius());
    
    update_delta_p(start_point,end_point,delta_first_order,peak_velocity,norm_delta,beta,space,t_space,num_tol);
    //svp_update_delta_first_order(start_point,end_point,delta_first_order,peak_velocity,norm_delta,beta,space,t_space,num_tol);
    
    for(unsigned int i = 0; i < max_iter; ++i) {
      PointType1 prev_peak_velocity = peak_velocity;
      
      RK_NOTICE(1,"*********                       iter = " << i << " *****************");
      RK_NOTICE(1,"   with peak-vel = " << peak_velocity << " and delta-p = " << delta_first_order);
      
      peak_velocity = space_1.adjust(peak_velocity, space_1.get_diff_to_boundary(peak_velocity));
      
      double temp = get(distance_metric, space_1)(get<1>(start_point), peak_velocity, space_1);
      coefs[1] = (coefs[0] * coefs[0] + coefs[4] * coefs[4] - temp * temp) / (2.0 * coefs[0] * coefs[4]);
      temp = get(distance_metric, space_1)(get<1>(end_point), peak_velocity, space_1);
      coefs[3] = (coefs[2] * coefs[2] + coefs[4] * coefs[4] - temp * temp) / (2.0 * coefs[2] * coefs[4]);
      
      RK_NOTICE(1,"   coefs = " << coefs);
      
      beta = get_next_beta(beta,norm_delta,coefs,num_tol);
      
      RK_NOTICE(1,"   found beta = " << beta);
      
      peak_velocity = space_1.adjust(space_1.origin(), beta * space_1.difference(peak_velocity, space_1.origin()));
      
      update_delta_p(start_point,end_point,delta_first_order,peak_velocity,norm_delta,beta,space,t_space,num_tol);
      //svp_update_delta_first_order(start_point,end_point,delta_first_order,peak_velocity,norm_delta,beta,space,t_space,num_tol);
      
      RK_NOTICE(1,"   gives new peak-vel = " << peak_velocity);
      RK_NOTICE(1,"   gives new delta-p = " << delta_first_order);
      
      if( pred(beta,norm_delta,coefs,num_tol)
	 && ( beta > num_tol ) && ( beta <= 1.0)
	 && ( get(distance_metric, space_1)(prev_peak_velocity, peak_velocity, space_1) < 1e-6 * space_1.get_radius() ) ) {
        break;
      };
	  
    };
  };
  
  
  
  
  template <typename PointType, typename PointDiff0, typename PointType1, typename DiffSpace, typename TimeSpace>
  double svp_compute_min_delta_time(const PointType& start_point, const PointType& end_point,
                                    PointDiff0& delta_first_order, PointType1& peak_velocity,
				    double& norm_delta, double& beta,
				    const DiffSpace& space, const TimeSpace& t_space,
				    double num_tol = 1E-6, unsigned int max_iter = 20) {
    
    double result = 0.0;
    RK_NOTICE(1,"*********  Starting Min-dt Iterations    *****************");
    
    svp_peak_velocity_iteration(
      start_point, end_point,
      delta_first_order, peak_velocity,
      norm_delta, beta,
      space, t_space,
      svp_update_delta_first_order(),
      svp_solve_for_min_dt_beta,
      boost::bind(svp_min_dt_predicate,_1,_2,_3,_4,boost::ref(result)),
      num_tol, max_iter
    );
    
    return result;
  };
  
  template <typename PointType, typename PointDiff0, typename PointType1, typename DiffSpace, typename TimeSpace>
  double svp_compute_peak_velocity(const PointType& start_point, const PointType& end_point,
                                   PointDiff0& delta_first_order, PointType1& peak_velocity,
				   double& norm_delta, double& beta, double delta_time,
				   const DiffSpace& space, const TimeSpace& t_space,
				   double num_tol = 1E-6, unsigned int max_iter = 20) {
    
    double slack = 0.0;
    RK_NOTICE(1,"*********  Starting No-slack Iterations  *****************");
    
    svp_peak_velocity_iteration(
      start_point, end_point,
      delta_first_order, peak_velocity,
      norm_delta, beta,
      space, t_space,
      svp_update_delta_first_order(),
      boost::bind(svp_solve_for_no_slack_beta,_1,_2,_3,_4,boost::cref(delta_time)),
      boost::bind(svp_no_slack_predicate,_1,_2,_3,_4,boost::ref(slack),boost::cref(delta_time)),
      num_tol, max_iter
    );
    
    return slack;
  };
  
  
  template <typename PointType, typename DiffSpace, typename TimeSpace>
  double svp_compute_interpolation_data_impl(const PointType& start_point, const PointType& end_point,
                                             typename topology_traits< typename derived_N_order_space< DiffSpace, TimeSpace,0>::type >::point_difference_type& delta_first_order,
					     typename topology_traits< typename derived_N_order_space< DiffSpace, TimeSpace,1>::type >::point_type& peak_velocity,
					     const DiffSpace& space, const TimeSpace& t_space,
					     double delta_time = 0.0,
					     typename topology_traits< typename derived_N_order_space< DiffSpace, TimeSpace,1>::type >::point_type* best_peak_velocity = NULL, 
					     double num_tol = 1e-6, unsigned int max_iter = 20) {
    delta_first_order = get_space<0>(space,t_space).difference( get<0>(end_point), get<0>(start_point) );
    double norm_delta = get(distance_metric, get_space<0>(space,t_space))( delta_first_order, get_space<0>(space,t_space) );
    double beta = 0.0;
    peak_velocity = get_space<1>(space,t_space).origin();
    
    double min_delta_time = svp_compute_min_delta_time(start_point, end_point, 
                                                       delta_first_order, peak_velocity,
					               norm_delta, beta, space, t_space, 1e-6, 20);
    if(best_peak_velocity)
      *best_peak_velocity = peak_velocity;
    
    if(min_delta_time > delta_time)
      return min_delta_time;
    
    beta = beta * min_delta_time / delta_time;
    peak_velocity = get_space<1>(space,t_space).adjust(
      get_space<1>(space,t_space).origin(),
      (min_delta_time / delta_time) *
      get_space<1>(space,t_space).difference(
        peak_velocity,
        get_space<1>(space,t_space).origin()
      )
    );
    
    svp_compute_peak_velocity(start_point, end_point, 
                              delta_first_order, peak_velocity,
			      norm_delta, beta, delta_time,
			      space, t_space, 1e-6, 100);
    
    return min_delta_time;
  };
  
};


};

};

#endif









