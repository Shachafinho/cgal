// Copyright (c) 2017 Tel-Aviv University (Israel).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
//
//
// Author(s)     : Shachaf Ben Jakov <shachafinho@gmail.com>
//                 Efi Fogel <efif@post.tau.ac.il>

#ifndef CGAL_ARR_POLYHEDRAL_SGM_REFLECT_H
#define CGAL_ARR_POLYHEDRAL_SGM_REFLECT_H

#include <CGAL/license/Arrangement_on_surface_2.h>


/*! \file
 * Definition of the global reflect() function for arr_polyhedral_sgm.
 */

#include <boost/optional/optional.hpp>
#include <boost/type_traits.hpp>

#include <CGAL/Arr_spherical_gaussian_map_3/Arr_polyhedral_sgm.h>
#include <CGAL/Arr_reflect.h>

#include <CGAL/assertions.h>

namespace CGAL {

/*!
 * The following overload takes place if GeomeTraitsRes support reflection.
 * It reflects the arrangement using the traits' Reflect_2 functor.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          template <class T> typename Dcel_T, template <class T> typename DcelRes_T>
typename boost::enable_if_c<has_Reflect_2<GeomeTraitsRes>::value, void>::type
reflect_sgm(const Arr_polyhedral_sgm<GeomeTraits, Dcel_T>& sgm,
            Arr_polyhedral_sgm<GeomeTraitsRes, DcelRes_T>& sgm_res,
            bool should_merge)
{
  // Copy the initial sgm and reflect it in-place

  typedef Arr_polyhedral_sgm<typename GeomeTraits, typename Dcel_T>           Sgm;
  typedef Arr_polyhedral_sgm<typename GeomeTraitsRes, typename DcelRes_T>     Sgm_res;

  // Geometry related typedefs
  typedef typename Sgm::Point_3                                               Point_3;
  typedef typename Sgm::Vector_3                                              Vector_3;

  // Dcel related typedefs
  typedef typename Sgm::Face_const_iterator                                   DFace_const_iterator;
  typedef typename Sgm::Edge_const_iterator                                   DEdge_const_iterator;
  typedef typename Sgm_res::Face_iterator                                     DFace_iterator_res;
  typedef typename Sgm_res::Edge_iterator                                     DEdge_iterator_res;

  // Some type assertions (not all, but better then nothing).
  CGAL_static_assertion
  ((boost::is_convertible<typename GeomeTraits::Point_2, \
                          typename GeomeTraitsRes::Point_2>::value));
  CGAL_static_assertion
  ((boost::is_convertible<typename GeomeTraits::X_monotone_curve_2, \
                          typename GeomeTraitsRes::X_monotone_curve_2>::value));

  // The result sgm cannot be the input sgm.
  CGAL_precondition((void *)(&sgm_res) != (void *)(&sgm));


  // Reflect the arrangement component of the sgm.
  reflect_arrangement(sgm, sgm_res, should_merge);

  // Set the points of the faces with the reflected points of the original sgm.
  DFace_const_iterator fit = sgm.faces_begin();
  for (DFace_iterator_res fit_res = sgm_res.faces_begin(); fit_res != sgm_res.faces_end();
    ++fit_res)
  {
    if (fit->is_set()) {
      const Point_3& p = fit->point();
      Vector_3 v = ORIGIN - p;
      fit_res->set_point(p + v + v);
    }
    ++fit;
  }

  // Set the arr_mask of the edges
  DEdge_const_iterator eit = sgm.edges_begin();
  for (DEdge_iterator_res eit_res = sgm_res.edges_begin(); eit_res != sgm_res.edges_end();
    ++eit_res)
  {
    eit_res->set_arr(eit->arr_mask());
    eit_res->twin()->set_arr(eit->arr_mask());
    ++eit;
  }

  // Reflect the center of the sgm.
  const Point_3& center = sgm.center();
  Vector_3 v = ORIGIN - center;
  sgm_res.set_center(center + v + v);
}

/*!
 * The following overload takes place if GeomeTraitsRes do not support reflection.
 * Reports an appropriate error message.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          template <class T> typename Dcel_T, template <class T> typename DcelRes_T>
typename boost::enable_if_c<!has_Reflect_2<GeomeTraitsRes>::value, void>::type
reflect_sgm(const Arr_polyhedral_sgm<GeomeTraits, Dcel_T>& sgm,
            Arr_polyhedral_sgm<GeomeTraitsRes, DcelRes_T>& sgm_res,
            bool should_merge)
{
  CGAL_error_msg("The result sgm traits do not support reflection!");
}

/*!
 * Computes the reflection of a polyhedral sgm through the origin.
 * \param sgm The polyhedral spherical gaussian map to reflect.
 * \param sgm_res Output: The resulting polyhedral spherical gaussian map.
 * \param should_merge Should merge curves previously divided by boundary.
 */
template <typename GeomeTraits, typename GeomeTraitsRes,
          template <class T> typename Dcel_T, template <class T> typename DcelRes_T>
void reflect(const Arr_polyhedral_sgm<GeomeTraits, Dcel_T>& sgm,
             Arr_polyhedral_sgm<GeomeTraitsRes, DcelRes_T>& sgm_res,
             bool should_merge = true)
{
  reflect_sgm(sgm, sgm_res, should_merge);
}
} //namespace CGAL

#endif
