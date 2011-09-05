/**
 * \file kalman_filter.hpp
 * 
 * This library provides a number of functions and classes to do state estimation 
 * using the (Extended) Kalman Filter. This filtering technique applies to a 
 * gaussian belief state. The Kalman filter is an optimal filter for a linear 
 * state-space system where all sources of noise or disturbances are Gaussian 
 * (normally distributed). This Kalman filter implementation only requires
 * that the system be at least a linearized system, in which case it becomes 
 * an Extended Kalman Filter (EKF), but if applied to an LTI or LTV system, than 
 * it is the usual Kalman Filter (minimum mean square estimator, MMSE).
 * 
 * \author Sven Mikael Persson <mikael.s.persson@gmail.com>
 * \date May 2011
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

#ifndef REAK_KALMAN_FILTER_HPP
#define REAK_KALMAN_FILTER_HPP

#include "belief_state_concept.hpp"
#include "discrete_linear_sss_concept.hpp"
#include <boost/utility/enable_if.hpp>
#include <lin_alg/vect_concepts.hpp>
#include <lin_alg/mat_alg.hpp>
#include <lin_alg/mat_cholesky.hpp>

#include <boost/static_assert.hpp>
#include "covariance_concept.hpp"

#include "gaussian_belief_state.hpp"
#include "covariance_matrix.hpp"

namespace ReaK {

namespace ctrl {

/**
 * This function template performs one prediction step using the (Extended) Kalman Filter method.
 * \tparam LinearSystem A discrete state-space system modeling the DiscreteLinearSSSConcept 
 *         at least as a DiscreteLinearizedSystemType.
 * \tparam BeliefState A belief state type modeling the ContinuousBeliefStateConcept with
 *         a unimodular gaussian representation.
 * \tparam SystemNoiseCovariance A covariance matrix type modeling the CovarianceMatrixConcept.
 * \param sys The discrete state-space system used in the state estimation.
 * \param b As input, it stores the belief-state before the prediction step. As output, it stores
 *        the belief-state after the prediction step.
 * \param u The input vector to apply to the state-space system to make the transition of the 
 *        mean-state, i.e., the current input vector.
 * \param Q The input noise covariance matrix. This is the level of uncertainty on the input 
 *        vector components (not the noise on the state transition). This was chosen as the most
 *        common way application (usually system disturbance comes on the input, not on the state).
 * \param t The current time (before the prediction).
 * 
 */
template <typename LinearSystem, 
          typename BeliefState, 
	  typename SystemNoiseCovariance>
typename boost::enable_if_c< is_continuous_belief_state<BeliefState>::value &&
                             (belief_state_traits<BeliefState>::representation == belief_representation::gaussian) &&
                             (belief_state_traits<BeliefState>::distribution == belief_distribution::unimodal),
void >::type kalman_predict(const LinearSystem& sys,
			    BeliefState& b,
			    const typename discrete_sss_traits<LinearSystem>::input_type& u,
			    const SystemNoiseCovariance& Q,
			    typename discrete_sss_traits<LinearSystem>::time_type t = 0) {
  //here the requirement is that the system models a linear system which is at worse a linearized system
  // - if the system is LTI or LTV, then this will result in a basic Kalman Filter (KF) prediction
  // - if the system is linearized, then this will result in an Extended Kalman Filter (EKF) prediction
  boost::function_requires< DiscreteLinearSSSConcept< LinearSystem, DiscreteLinearizedSystemType > >();
  boost::function_requires< ContinuousBeliefStateConcept<BeliefState> >();
  
  typedef typename discrete_sss_traits<LinearSystem>::point_type StateType;
  typedef typename continuous_belief_state_traits<BeliefState>::covariance_type CovType;
  typedef typename covariance_mat_traits< CovType >::matrix_type MatType;
  
  typename discrete_linear_sss_traits<LinearSystem>::matrixA_type A;
  typename discrete_linear_sss_traits<LinearSystem>::matrixB_type B;
  typename discrete_linear_sss_traits<LinearSystem>::matrixC_type C;
  typename discrete_linear_sss_traits<LinearSystem>::matrixD_type D;
  StateType x = b.get_mean_state();
  sys.get_linear_blocks(A, B, C, D, t, x, u);
  
  b.set_mean_state( sys.get_next_state(x, u, t) );
  b.set_covariance( CovType( ( A * b.get_covariance().get_matrix() * transpose(A) ) + B * Q.get_matrix() * transpose(B) ) );
};


/**
 * This function template performs one measurement update step using the (Extended) Kalman Filter method.
 * \tparam LinearSystem A discrete state-space system modeling the DiscreteLinearSSSConcept 
 *         at least as a DiscreteLinearizedSystemType.
 * \tparam BeliefState A belief state type modeling the ContinuousBeliefStateConcept with
 *         a unimodular gaussian representation.
 * \tparam MeasurementNoiseCovariance A covariance matrix type modeling the CovarianceMatrixConcept.
 * \param sys The discrete state-space system used in the state estimation.
 * \param b As input, it stores the belief-state before the update step. As output, it stores
 *        the belief-state after the update step.
 * \param u The input vector to apply to the state-space system to make the transition of the 
 *        mean-state, i.e., the current input vector.
 * \param z The output vector to that was measured.
 * \param R The output noise covariance matrix. This is the level of uncertainty on the output 
 *        vector components coming from the measurements.
 * \param t The current time.
 * 
 */
template <typename LinearSystem, 
          typename BeliefState, 
	  typename MeasurementNoiseCovariance>
typename boost::enable_if_c< is_continuous_belief_state<BeliefState>::value &&
                             (belief_state_traits<BeliefState>::representation == belief_representation::gaussian) &&
                             (belief_state_traits<BeliefState>::distribution == belief_distribution::unimodal),
void >::type kalman_update(const LinearSystem& sys,
			   BeliefState& b,
			   const typename discrete_sss_traits<LinearSystem>::input_type& u,
			   const typename discrete_sss_traits<LinearSystem>::output_type& z,
			   const MeasurementNoiseCovariance& R,
			   typename discrete_sss_traits<LinearSystem>::time_type t = 0) {
  //here the requirement is that the system models a linear system which is at worse a linearized system
  // - if the system is LTI or LTV, then this will result in a basic Kalman Filter (KF) update
  // - if the system is linearized, then this will result in an Extended Kalman Filter (EKF) update
  boost::function_requires< DiscreteLinearSSSConcept< LinearSystem, DiscreteLinearizedSystemType > >();
  boost::function_requires< ContinuousBeliefStateConcept<BeliefState> >();
  
  typedef typename discrete_sss_traits<LinearSystem>::point_type StateType;
  typedef typename discrete_sss_traits<LinearSystem>::output_type OutputType;
  typedef typename continuous_belief_state_traits<BeliefState>::covariance_type CovType;
  typedef typename covariance_mat_traits< CovType >::matrix_type MatType;
  typedef typename mat_traits<MatType>::value_type ValueType;
  
  typename discrete_linear_sss_traits<LinearSystem>::matrixA_type A;
  typename discrete_linear_sss_traits<LinearSystem>::matrixB_type B;
  typename discrete_linear_sss_traits<LinearSystem>::matrixC_type C;
  typename discrete_linear_sss_traits<LinearSystem>::matrixD_type D;
  StateType x = b.get_mean_state();
  const MatType& P = b.get_covariance().get_matrix();
  sys.get_linear_blocks(A, B, C, D, t, x, u);
  
  OutputType y = z - C * x - D * u;
  mat< ValueType, mat_structure::rectangular, mat_alignment::column_major > CP = C * P;
  mat< ValueType, mat_structure::symmetric > S( CP * transpose(C) + R.get_matrix() );
  linsolve_Cholesky(S,CP);
  mat< ValueType, mat_structure::rectangular, mat_alignment::row_major > K = transpose_move(CP);
   
  b.set_mean_state( x + K * y );
  b.set_covariance( CovType( MatType( (mat< ValueType, mat_structure::identity>(K.get_row_count()) - K * C) * P ) ) );
};


/**
 * This function template performs one complete estimation step using the (Extended) Kalman 
 * Filter method, which includes a prediction and measurement update step. This function is, 
 * in general, more efficient than applying the prediction and update separately.
 * \tparam LinearSystem A discrete state-space system modeling the DiscreteLinearSSSConcept 
 *         at least as a DiscreteLinearizedSystemType.
 * \tparam BeliefState A belief state type modeling the ContinuousBeliefStateConcept with
 *         a unimodular gaussian representation.
 * \tparam SystemNoiseCovariance A covariance matrix type modeling the CovarianceMatrixConcept.
 * \tparam MeasurementNoiseCovariance A covariance matrix type modeling the CovarianceMatrixConcept.
 * \param sys The discrete state-space system used in the state estimation.
 * \param b As input, it stores the belief-state before the estimation step. As output, it stores
 *        the belief-state after the estimation step.
 * \param u The input vector to apply to the state-space system to make the transition of the 
 *        mean-state, i.e., the current input vector.
 * \param z The output vector to that was measured.
 * \param Q The input noise covariance matrix. This is the level of uncertainty on the input 
 *        vector components (not the noise on the state transition). This was chosen as the most
 *        common way application (usually system disturbance comes on the input, not on the state).
 * \param R The output noise covariance matrix. This is the level of uncertainty on the output 
 *        vector components coming from the measurements.
 * \param t The current time (before the prediction).
 * 
 */
template <typename LinearSystem, 
          typename BeliefState, 
	  typename SystemNoiseCovariance,
	  typename MeasurementNoiseCovariance>
typename boost::enable_if_c< is_continuous_belief_state<BeliefState>::value &&
                             (belief_state_traits<BeliefState>::representation == belief_representation::gaussian) &&
                             (belief_state_traits<BeliefState>::distribution == belief_distribution::unimodal),
void >::type kalman_filter_step(const LinearSystem& sys,
			        BeliefState& b,
				const typename discrete_sss_traits<LinearSystem>::input_type& u,
				const typename discrete_sss_traits<LinearSystem>::output_type& z,
				const SystemNoiseCovariance& Q,
				const MeasurementNoiseCovariance& R,
				typename discrete_sss_traits<LinearSystem>::time_type t = 0) {
  //here the requirement is that the system models a linear system which is at worse a linearized system
  // - if the system is LTI or LTV, then this will result in a basic Kalman Filter (KF) update
  // - if the system is linearized, then this will result in an Extended Kalman Filter (EKF) update
  boost::function_requires< DiscreteLinearSSSConcept< LinearSystem, DiscreteLinearizedSystemType > >();
  boost::function_requires< ContinuousBeliefStateConcept<BeliefState> >();

  typedef typename discrete_sss_traits<LinearSystem>::point_type StateType;
  typedef typename discrete_sss_traits<LinearSystem>::output_type OutputType;
  typedef typename continuous_belief_state_traits<BeliefState>::covariance_type CovType;
  typedef typename covariance_mat_traits< CovType >::matrix_type MatType;
  typedef typename mat_traits<MatType>::value_type ValueType;
  
  typename discrete_linear_sss_traits<LinearSystem>::matrixA_type A;
  typename discrete_linear_sss_traits<LinearSystem>::matrixB_type B;
  typename discrete_linear_sss_traits<LinearSystem>::matrixC_type C;
  typename discrete_linear_sss_traits<LinearSystem>::matrixD_type D;
  StateType x = b.get_mean_state();
  MatType P = b.get_covariance().get_matrix();
  sys.get_linear_blocks(A, B, C, D, t, x, u);

  x = sys.get_next_state(x, u, t);
  P = ( A * P * transpose(A)) + B * Q.get_matrix() * transpose(B);
  
  OutputType y = z - C * x - D * u;
  mat< ValueType, mat_structure::rectangular, mat_alignment::column_major > CP = C * P;
  mat< ValueType, mat_structure::symmetric > S(CP * transpose(C) + R.get_matrix());  
  linsolve_Cholesky(S,CP);
  mat< ValueType, mat_structure::rectangular, mat_alignment::row_major > K = transpose_move(CP);
   
  b.set_mean_state( x + K * y );
  b.set_covariance( CovType( MatType( (mat< ValueType, mat_structure::identity>(K.get_row_count()) - K * C) * P ) ) );
};




/**
 * This class template can be used as a belief-state predictor (and transfer) that uses the 
 * (Extended) Kalman Filter method. This class template models the BeliefTransferConcept and 
 * the BeliefPredictorConcept.
 * \tparam LinearSystem A discrete state-space system modeling the DiscreteLinearSSSConcept 
 *         at least as a DiscreteLinearizedSystemType.
 * \tparam BeliefState A belief state type modeling the ContinuousBeliefStateConcept with
 *         a unimodular gaussian representation.
 * \tparam SystemNoiseCovar A covariance matrix type modeling the CovarianceMatrixConcept.
 * \tparam MeasurementCovar A covariance matrix type modeling the CovarianceMatrixConcept.
 */
template <typename LinearSystem,
          typename BeliefState = gaussian_belief_state< covariance_matrix< typename discrete_sss_traits<LinearSystem>::point_type > >,
          typename SystemNoiseCovar = covariance_matrix< typename discrete_sss_traits< LinearSystem >::input_type >,
          typename MeasurementCovar = covariance_matrix< typename discrete_sss_traits< LinearSystem >::output_type > >
struct KF_belief_transfer {
  typedef KF_belief_transfer<LinearSystem, BeliefState> self;
  typedef BeliefState belief_state;
  typedef LinearSystem state_space_system;
  typedef typename discrete_sss_traits< state_space_system >::time_type time_type;
  typedef typename discrete_sss_traits< state_space_system >::time_difference_type time_difference_type;

  typedef typename belief_state_traits< belief_state >::state_type state_type;
  typedef typename continuous_belief_state_traits<BeliefState>::covariance_type covariance_type;
  typedef typename covariance_mat_traits< covariance_type >::matrix_type matrix_type;

  typedef typename discrete_sss_traits< state_space_system >::input_type input_type;
  typedef typename discrete_sss_traits< state_space_system >::output_type output_type;

  const LinearSystem* sys; ///< Holds the reference to the system used for the filter.
  SystemNoiseCovar Q; ///< Holds the system's input noise covariance matrix.
  MeasurementCovar R; ///< Holds the system's output measurement's covariance matrix.

  /**
   * Parametrized constructor.
   * \param aSys The reference to the system used for the filter.
   * \param aQ The system's input noise covariance matrix.
   * \param aR The system's output measurement's covariance matrix.
   */
  KF_belief_transfer(const LinearSystem& aSys, 
                     const SystemNoiseCovar& aQ,
                     const MeasurementCovar& aR) : sys(aSys), Q(aQ), R(aR) { };
  
  /**
   * Returns the time-step of the predictor.
   * \return The time-step of the predictor.
   */
  time_difference_type get_time_step() const { return sys->get_time_step(); };

  /**
   * Returns a reference to the underlying state-space system.
   * \return A reference to the underlying state-space system.
   */
  const state_space_system& get_ss_system() const { return *sys; };

  /**
   * Returns the belief-state at the next time instant.
   * \param b The current belief-state.
   * \param t The current time.
   * \param u The current input given to the system.
   * \param y The output that was measured at the next time instant.
   * \return the belief-state at the next time instant.
   */
  belief_state get_next_belief(belief_state b, const time_type& t, const input_type& u, const input_type& y) const {
    kalman_filter_step(*sys,b,u,y,Q,R,t);
    return b;
  };
  
  /**
   * Returns the prediction belief-state at the next time instant.
   * \param b The current belief-state.
   * \param t The current time.
   * \param u The current input given to the system.
   * \return the belief-state at the next time instant, predicted by the filter.
   */
  belief_state predict_belief(belief_state b, const time_type& t, const input_type& u) const {
    kalman_predict(*sys,b,u,Q,t);
    return b;
  };
  
  /**
   * Converts a prediction belief-state into an updated belief-state which assumes the most likely measurement.
   * \param b The current prediction's belief-state.
   * \param t The current time.
   * \param u The current input given to the system.
   * \return the updated belief-state when assuming the most likely measurement.
   */
  belief_state prediction_to_ML_belief(belief_state b, const time_type& t, const input_type& u) const {
    kalman_update(*sys,b,u,sys->get_output(b.get_mean_state(),u,t),R,t);
    return b;
  };
  
  /**
   * Returns the prediction belief-state at the next time instant, assuming the upcoming measurement to be the most likely one.
   * \param b The current belief-state.
   * \param t The current time.
   * \param u The current input given to the system.
   * \return the belief-state at the next time instant, predicted by the filter.
   */
  belief_state predict_ML_belief(belief_state b, const time_type& t, const input_type& u) const {
    kalman_predict(*sys,b,u,Q,t);
    kalman_update(*sys,b,u,sys->get_output(b.get_mean_state(),u,t),R,t + sys->get_time_step());
    return b;
  };
  
};








};

};


#endif













