
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
 *    If not, see <http://www.gnu.org/licenses/>.
 */


#include "base/defs.hpp"

#if (defined(RK_ENABLE_CXX11_FEATURES) && defined(RK_ENABLE_EXTERN_TEMPLATES))

#include "path_planning/rrtstar_manip_planners.hpp"

#include "path_planning/rrtstar_path_planner.tpp"

namespace ReaK {

namespace pp {


#define RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(NDOF) \
template class rrtstar_planner< \
  manip_quasi_static_env< typename Ndof_rl_space<double, NDOF, 0>::type, linear_interpolation_tag> >; \
template class rrtstar_planner< \
  manip_quasi_static_env< typename Ndof_rl_space<double, NDOF, 1>::type, linear_interpolation_tag> >; \
template class rrtstar_planner< \
  manip_quasi_static_env< typename Ndof_rl_space<double, NDOF, 2>::type, linear_interpolation_tag> >; \
 \
template class rrtstar_planner< \
  manip_quasi_static_env< typename Ndof_rl_space<double, NDOF, 1>::type, cubic_hermite_interpolation_tag> >; \
template class rrtstar_planner< \
  manip_quasi_static_env< typename Ndof_rl_space<double, NDOF, 2>::type, cubic_hermite_interpolation_tag> >; \
 \
template class rrtstar_planner< \
  manip_quasi_static_env< typename Ndof_rl_space<double, NDOF, 2>::type, quintic_hermite_interpolation_tag> >; \
 \
template class rrtstar_planner< \
  manip_quasi_static_env< typename Ndof_rl_space<double, NDOF, 1>::type, svp_Ndof_interpolation_tag> >; \
template class rrtstar_planner< \
  manip_quasi_static_env< typename Ndof_rl_space<double, NDOF, 2>::type, svp_Ndof_interpolation_tag> >; \
 \
template class rrtstar_planner< \
  manip_quasi_static_env< typename Ndof_rl_space<double, NDOF, 2>::type, sap_Ndof_interpolation_tag> >;


RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(RK_PLANNERS_DIM)

// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(0)
// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(1)
// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(2)
// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(3)
// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(4)
// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(5)
// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(6)
// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(7)
// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(8)
// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(9)
// RK_RRTSTAR_MANIP_PLANNERS_MAKE_STATIC_MANIP_EXTERN_INSTANCES(10)


};

};

#else

namespace ReaK {

namespace pp {

#define CONCAT_NAMES(X,Y) X##Y

void CONCAT_NAMES(dummy_rrtstar_manip_planners_static_externs_symbol_, RK_PLANNERS_DIM)() { };

};

};

#endif

